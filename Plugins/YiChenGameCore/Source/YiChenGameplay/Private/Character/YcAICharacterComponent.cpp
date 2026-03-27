// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Character/YcAICharacterComponent.h"

#include "YcAbilitySystemComponent.h"
#include "YcGameplayTags.h"
#include "Character/YcPawnExtensionComponent.h"
#include "Components/GameFrameworkComponentManager.h"
#include "YiChenGameplay.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAICharacterComponent)

const FName UYcAICharacterComponent::NAME_ActorFeatureName("AICharacter");

UYcAICharacterComponent::UYcAICharacterComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UYcAICharacterComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState) const
{
	check(Manager);

	APawn* Pawn = GetPawn<APawn>();

	// 检查是否可以从无效状态转换到Spawned状态
	if (!CurrentState.IsValid() && DesiredState == YcGameplayTags::InitState_Spawned)
	{
		// 只要存在有效的Pawn就允许转换
		if (Pawn) return true;
	}
	// 检查是否可以从Spawned转换到DataAvailable状态
	else if (CurrentState == YcGameplayTags::InitState_Spawned && DesiredState == YcGameplayTags::InitState_DataAvailable)
	{
		// AI角色不需要PlayerState，但需要Controller（权威端）
		if (Pawn->GetLocalRole() != ROLE_SimulatedProxy)
		{
			// 权威端或自主代理需要Controller
			if (!GetController<AController>()) return false;
		}

		return true;
	}
	// 检查是否可用从DataAvailable转换到DataInitialized状态
	else if (CurrentState == YcGameplayTags::InitState_DataAvailable && DesiredState == YcGameplayTags::InitState_DataInitialized)
	{
		UYcAbilitySystemComponent* ASC = Pawn->FindComponentByClass<UYcAbilitySystemComponent>();
		return IsValid(ASC);
	}
	// 检查是否可以从DataInitialized转换到GameplayReady状态
	else if (CurrentState == YcGameplayTags::InitState_DataInitialized && DesiredState == YcGameplayTags::InitState_GameplayReady)
	{
		return true;
	}

	return false;
}

void UYcAICharacterComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
	FGameplayTag DesiredState)
{
#if DEBUG_INIT_STATE
	const FString LocalRoleName = StaticEnum<ENetRole>()->GetNameStringByValue((int64)GetOwner()->GetLocalRole());
	UE_LOG(LogYcGameplay, Log, TEXT("HandleChangeInitState [%s] / Role [%s] / CurrentState[%s] / DesiredState[%s]"),
	       *GetNameSafe(GetOwner()), *LocalRoleName, *CurrentState.ToString(), *DesiredState.ToString());
#endif
	
	if (CurrentState == YcGameplayTags::InitState_DataAvailable && DesiredState == YcGameplayTags::InitState_DataInitialized)
	{
		const APawn* Pawn = GetPawn<APawn>();
		if (!ensure(Pawn)) return;

		if (UYcPawnExtensionComponent* PawnExtComp = UYcPawnExtensionComponent::FindPawnExtensionComponent(Pawn))
		{
			// AI角色的ASC直接挂载在Character上，而不是PlayerState
			// 尝试从Character获取ASC组件
			if (UYcAbilitySystemComponent* ASC = Pawn->FindComponentByClass<UYcAbilitySystemComponent>())
			{
				// ASC的Owner和Avatar都是Character自己
				PawnExtComp->InitializeAbilitySystem(ASC, const_cast<APawn*>(Pawn));
			}
			else
			{
				UE_LOG(LogYcGameplay, Warning, TEXT("UYcAICharacterComponent::HandleChangeInitState - No ASC found on Pawn [%s]. AI characters should have ASC component directly on them."), 
					*GetNameSafe(Pawn));
			}
		}
	}
}

void UYcAICharacterComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
#if DEBUG_INIT_STATE
	const FString LocalRoleName = StaticEnum<ENetRole>()->GetNameStringByValue((int64)GetOwner()->GetLocalRole());
	UE_LOG(LogYcGameplay, Log, TEXT("OnActorInitStateChanged [%s] / Role [%s] FeatureName[%s] / FeatureState[%s]"), 
		*GetNameSafe(GetOwner()), *LocalRoleName, *Params.FeatureName.ToString(), *Params.FeatureState.ToString());
#endif
	
	if (Params.FeatureName == UYcPawnExtensionComponent::NAME_ActorFeatureName)
	{
		if (Params.FeatureState == YcGameplayTags::InitState_DataInitialized)
		{
			// 如果PawnExtensionComponent报告所有其他组件均已初始化完成，则尝试推进到下一状态
			CheckDefaultInitialization();
		}
	}
}

void UYcAICharacterComponent::CheckDefaultInitialization()
{
	static const TArray<FGameplayTag> StateChain = { YcGameplayTags::InitState_Spawned, YcGameplayTags::InitState_DataAvailable, YcGameplayTags::InitState_DataInitialized, YcGameplayTags::InitState_GameplayReady };
	
	// 此流程将从"InitState_Spawned"状态开始，依次推进数据初始化各阶段，直至达到游戏可运行状态(InitState_GameplayReady)
	ContinueInitStateChain(StateChain);
}

void UYcAICharacterComponent::OnRegister()
{
	Super::OnRegister();
	
	// 检验是否挂载在APawn及其子类上的
	if (!GetPawn<APawn>())
	{
		UE_LOG(LogYcGameplay, Error, TEXT("[UYcAICharacterComponent::OnRegister] This component has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint."));

#if WITH_EDITOR
		if (GIsEditor)
		{
			static const FText Message = NSLOCTEXT("YcAICharacterComponent", "NotOnPawnError", "has been added to a blueprint whose base class is not a Pawn. To use this component, it MUST be placed on a Pawn Blueprint. This will cause a crash if you PIE!");
			static const FName AICharacterMessageLogName = TEXT("YcAICharacterComponent");
			
			FMessageLog(AICharacterMessageLogName).Error()
				->AddToken(FUObjectToken::Create(this, FText::FromString(GetNameSafe(this))))
				->AddToken(FTextToken::Create(Message));
				
			FMessageLog(AICharacterMessageLogName).Open();
		}
#endif
	}
	else
	{
		// 尽早向初始化状态系统进行注册，此操作仅当处于游戏世界时才会生效
		OwnerPawn = GetPawn<APawn>();
		RegisterInitStateFeature();
	}
}

void UYcAICharacterComponent::BeginPlay()
{
	Super::BeginPlay();
#if DEBUG_INIT_STATE
	UKismetSystemLibrary::PrintString(this, TEXT("YcAICharacterComponent-BeginPlay."));
#endif
	
	// 监听PawnExtension组件的初始化状态变更事件
	BindOnActorInitStateChanged(UYcPawnExtensionComponent::NAME_ActorFeatureName, FGameplayTag(), false);
	
	// 在接收到生成完成通知后，继续执行剩余初始化流程
	ensure(TryToChangeInitState(YcGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UYcAICharacterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInitStateFeature();
	Super::EndPlay(EndPlayReason);
}
