// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Player/YcPlayerState.h"

#include "YcAbilitySet.h"
#include "YcAbilitySystemComponent.h"
#include "YcTeamAgentInterface.h"
#include "YiChenGameplay.h"
#include "Player/YcPlayerController.h"
#include "Character/YcPawnData.h"
#include "Character/YcPawnExtensionComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "GameModes/YcExperienceManagerComponent.h"
#include "GameModes/YcGameMode.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcPlayerState)

const FName AYcPlayerState::NAME_YcAbilityReady("YcAbilitiesReady");

AYcPlayerState::AYcPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UYcAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	// ASC需要以较高的频率进行更新。
	SetNetUpdateFrequency(100.0f);
}

void AYcPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PawnData, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MyTeamID, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MySquadID, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, StatTags, SharedParams);
}

AYcPlayerController* AYcPlayerState::GetYcPlayerController() const
{
	return Cast<AYcPlayerController>(GetOwner());
}

UAbilitySystemComponent* AYcPlayerState::GetAbilitySystemComponent() const
{
	return GetYcAbilitySystemComponent();
}

void AYcPlayerState::SetPawnData(const UYcPawnData* InPawnData)
{
	check(InPawnData);

	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (PawnData)
	{
		UE_LOG(LogYcGameplay, Error, TEXT("Trying to set PawnData [%s] on player state [%s] that already has valid PawnData [%s]."), *GetNameSafe(InPawnData), *GetNameSafe(this), *GetNameSafe(PawnData));
		return;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PawnData, this);
	PawnData = InPawnData;
	
	// 遍历PawnData->AbilitySets中的技能授予ASC组件
	for (const UYcAbilitySet* AbilitySet : PawnData->AbilitySets)
	{
		if (AbilitySet)
		{
			AbilitySet->GiveToAbilitySystem(AbilitySystemComponent, nullptr);
		}
	}
	
	/** PawnData并且将其中的AbilitySet赋予了ASC组件, 发送一个可以被其他模块的游戏框架控制系统监听的事件 */
	UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(this, NAME_YcAbilityReady);

	ForceNetUpdate(); //强制更新到客户端(强制立刻同步)
}

void AYcPlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AYcPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();
	
	check(AbilitySystemComponent);
	// 需要主动设置AvatarActor为操作的Pawn, 否则默认Avatar就是PlayerState了
	AbilitySystemComponent->InitAbilityActorInfo(this, GetPawn()); 

	const UWorld* World = GetWorld();
	if (World && World->IsGameWorld() && World->GetNetMode() != NM_Client)
	{
		const AGameStateBase* GameState = GetWorld()->GetGameState();
		check(GameState);
		UYcExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UYcExperienceManagerComponent>();
		check(ExperienceComponent);
		// 向ExperienceComponent注册OnExperienceLoaded的回调, 以响应游戏体验加载完成事件
		ExperienceComponent->CallOrRegister_OnExperienceLoaded(FOnYcExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
	}
}

void AYcPlayerState::ClientInitialize(AController* C)
{
	Super::ClientInitialize(C);
	
	if (UYcPawnExtensionComponent* PawnExtComp = UYcPawnExtensionComponent::FindPawnExtensionComponent(GetPawn()))
	{
		// PlayerState现在在客户端有效了, 推进PawnExtComp的初始化进度
		PawnExtComp->CheckDefaultInitialization();
	}
}

void AYcPlayerState::OnExperienceLoaded(const UYcExperienceDefinition* CurrentExperience)
{
	check(CurrentExperience);
	
	if (const UYcPawnData* NewPawnData = CurrentExperience->DefaultPawnData)
	{
		SetPawnData(NewPawnData);
	}
	else
	{
		UE_LOG(LogYcGameplay, Error, TEXT("AYcPlayerState::OnExperienceLoaded(): Unable to find PawnData to initialize player state [%s]!"), *GetNameSafe(this));
	}
}

void AYcPlayerState::OnRep_PawnData()
{
}

void AYcPlayerState::AddStatTagStack(const FGameplayTag Tag, const int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void AYcPlayerState::RemoveStatTagStack(const FGameplayTag Tag, const int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

int32 AYcPlayerState::GetStatTagStackCount(const FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

bool AYcPlayerState::HasStatTag(const FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}

void AYcPlayerState::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	// 仅在服务器端允许设置队伍ID
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	// 如果队伍ID没有变化，直接返回
	if (MyTeamID == NewTeamID)
	{
		return;
	}

	// 保存旧的队伍ID用于广播变更事件
	const FGenericTeamId OldTeamID = MyTeamID;

	// 更新队伍ID并标记为脏数据（触发网络复制）
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MyTeamID, this);
	MyTeamID = NewTeamID;

	// 广播队伍变更事件
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

FGenericTeamId AYcPlayerState::GetGenericTeamId() const
{
	return MyTeamID;
}

FOnYcTeamIndexChangedDelegate* AYcPlayerState::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

int32 AYcPlayerState::GetTeamId() const
{
	// 将通用队伍ID转换为整数形式
	return GenericTeamIdToInteger(MyTeamID);
}

int32 AYcPlayerState::GetSquadId() const
{
	return MySquadID;
}

void AYcPlayerState::K2_SetTeamID(int32 NewTeamID)
{
	SetGenericTeamId(IntegerToGenericTeamId(NewTeamID));
}

void AYcPlayerState::SetSquadID(int32 NewSquadID)
{
	// 仅在服务器端允许设置小队ID
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	// 如果小队ID没有变化，直接返回
	if (MySquadID == NewSquadID)
	{
		return;
	}

	// 更新小队ID并标记为脏数据（触发网络复制）
	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MySquadID, this);
	MySquadID = NewSquadID;
}

void AYcPlayerState::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	// 当队伍ID通过网络复制到客户端时，广播队伍变更事件
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

void AYcPlayerState::OnRep_MySquadID(int32 OldSquadId)
{
	// 当小队ID通过网络复制到客户端时，可以在这里处理相关的客户端逻辑
	// 例如：更新UI显示、刷新小队成员列表等
}