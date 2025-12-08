// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Character/YcPawnExtensionComponent.h"
#include "YcAbilitySystemComponent.h"
#include "YcGameplayTags.h"
#include "YiChenGameplay.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Character/YcPawnData.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcPawnExtensionComponent)

const FName UYcPawnExtensionComponent::NAME_ActorFeatureName("PawnExtension");

UYcPawnExtensionComponent::UYcPawnExtensionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	PawnData = nullptr;
	AbilitySystemComponent = nullptr;
}

void UYcPawnExtensionComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UYcPawnExtensionComponent, PawnData);
}

void UYcPawnExtensionComponent::OnRegister()
{
	Super::OnRegister();

	// 检查确保此组件只能添加到Pawn及其子类上
	const APawn* Pawn = GetPawn<APawn>();
	ensureAlwaysMsgf((Pawn != nullptr), TEXT("YcPawnExtensionComponent on [%s] can only be added to Pawn actors."), *GetNameSafe(GetOwner()));

	// 检查保证每个Pawn上只有一个PawnExtensionComponent
	TArray<UActorComponent*> PawnExtensionComponents;
	Pawn->GetComponents(UYcPawnExtensionComponent::StaticClass(), PawnExtensionComponents);
	ensureAlwaysMsgf((PawnExtensionComponents.Num() == 1), TEXT("Only one SCPawnExtensionComponent should exist on [%s]."), *GetNameSafe(GetOwner()));

	// 注册到初始化状态系统，仅在游戏世界中有效
	RegisterInitStateFeature();
}

void UYcPawnExtensionComponent::BeginPlay()
{
	Super::BeginPlay();
#if DEBUG_INIT_STATE
	UKismetSystemLibrary::PrintString(this, TEXT("UYcPawnExtensionComponent-BeginPlay."));
#endif

	// 监听所有组件的初始化状态变化
	BindOnActorInitStateChanged(NAME_None, FGameplayTag(), false);

	// 告诉初始化状态系统我们已完成生成，然后尝试其他默认初始化
	ensure(TryToChangeInitState(YcGameplayTags::InitState_Spawned));
	CheckDefaultInitialization();
}

void UYcPawnExtensionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UninitializeAbilitySystem();
	UnregisterInitStateFeature();

	Super::EndPlay(EndPlayReason);
}

void UYcPawnExtensionComponent::SetPawnData(const UYcPawnData* InPawnData)
{
	check(InPawnData);

	APawn* Pawn = GetPawnChecked<APawn>();

	// SetPawnData只能在权威端执行，其他端通过网络复制获得
	if (Pawn->GetLocalRole() != ROLE_Authority) return;

	// 防止PawnData重复设置
	if (PawnData)
	{
		UE_LOG(LogYcGameplay, Error, TEXT("Trying to set PawnData [%s] on pawn [%s] that already has valid PawnData [%s]."), *GetNameSafe(InPawnData), *GetNameSafe(Pawn), *GetNameSafe(PawnData));
		return;
	}

	PawnData = InPawnData;

	// 强制网络更新，确保此重要资源立即向客户端同步
	Pawn->ForceNetUpdate();

	// 尝试推进初始化状态
	CheckDefaultInitialization();
}

void UYcPawnExtensionComponent::OnRep_PawnData()
{
	// PawnData数据被复制到客户端，尝试推进初始化状态
	CheckDefaultInitialization();
}

bool UYcPawnExtensionComponent::CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState,
                                                   FGameplayTag DesiredState) const
{
	check(Manager);
	
	APawn* Pawn = GetPawn<APawn>();
	if (!CurrentState.IsValid() && DesiredState == YcGameplayTags::InitState_Spawned)
	{
		// 只要存在于有效Pawn上，就判定为已生成状态
		if (Pawn) return true;
	}
	if (CurrentState == YcGameplayTags::InitState_Spawned && DesiredState == YcGameplayTags::InitState_DataAvailable)
	{
		// 需要PawnData, 由GameMode调用SetPawnData进行设置
		if (!PawnData) return false;

		const bool bHasAuthority = Pawn->HasAuthority();
		const bool bIsLocallyControlled = Pawn->IsLocallyControlled();

		if (bHasAuthority || bIsLocallyControlled)
		{
			// 检查是否已被控制器控制
			if (!GetController<AController>()) return false;
		}

		return true;
	}
	else if (CurrentState == YcGameplayTags::InitState_DataAvailable && DesiredState == YcGameplayTags::InitState_DataInitialized)
	{
		// 当所有功能模块都达到数据可用状态时，才允许转换到数据已初始化状态, 因为PawnExtensionComponent把控所有功能扩展组件的初始化
		return Manager->HaveAllFeaturesReachedInitState(Pawn, YcGameplayTags::InitState_DataAvailable);
	}
	else if (CurrentState == YcGameplayTags::InitState_DataInitialized && DesiredState == YcGameplayTags::InitState_GameplayReady)
	{
		return true;
	}

	return false;
}

void UYcPawnExtensionComponent::HandleChangeInitState(UGameFrameworkComponentManager* Manager,
	FGameplayTag CurrentState, FGameplayTag DesiredState)
{
#if DEBUG_INIT_STATE
	const FString LocalRoleName = StaticEnum<ENetRole>()->GetNameStringByValue((int64)GetOwner()->GetLocalRole());
	UE_LOG(LogYcGameplay, Log, TEXT("HandleChangeInitState [%s] / Role [%s] / CurrentState[%s] / DesiredState[%s]"),
	       *GetNameSafe(GetOwner()), *LocalRoleName, *CurrentState.ToString(), *DesiredState.ToString())
#endif
	
	// 目前，所有这些处理都通过其他监听此状态变化的组件完成。
}

void UYcPawnExtensionComponent::OnActorInitStateChanged(const FActorInitStateChangedParams& Params)
{
#if DEBUG_INIT_STATE
	const FString LocalRoleName = StaticEnum<ENetRole>()->GetNameStringByValue((int64)GetOwner()->GetLocalRole());
	UE_LOG(LogYcGameplay, Log, TEXT("OnActorInitStateChanged [%s] / Role [%s] FeatureName[%s] / FeatureState[%s]"),
	       *GetNameSafe(GetOwner()), *LocalRoleName, *Params.FeatureName.ToString(), *Params.FeatureState.ToString());
#endif
	
	// 如果现在有另一功能扩展组件(也可称为功能模块)已达到"InitState_DataAvailable"状态，检查我们是否应过渡到"数据已初始化"状态
	if (Params.FeatureName != NAME_ActorFeatureName)
	{
		if (Params.FeatureState == YcGameplayTags::InitState_DataAvailable)
		{
			CheckDefaultInitialization();
		}
	}
	
	if (Params.FeatureState == YcGameplayTags::InitState_GameplayReady)
	{
		const FString RoleName = StaticEnum<ENetRole>()->GetNameStringByValue((int64)GetOwner()->GetLocalRole());
		UE_LOG(LogYcGameplay, Log, TEXT("YcPawnExtension: %s %s Feature Component is Ready. LocalRole= %s"),
		       *GetNameSafe(GetOwner()), *Params.FeatureName.ToString(), *RoleName);
	}
}

void UYcPawnExtensionComponent::CheckDefaultInitialization()
{
	// 在检查我们的进度之前，先尝试推进我们可能依赖的其他特性
	CheckDefaultInitializationForImplementers();

	static const TArray<FGameplayTag> StateChain = { YcGameplayTags::InitState_Spawned, YcGameplayTags::InitState_DataAvailable, YcGameplayTags::InitState_DataInitialized, YcGameplayTags::InitState_GameplayReady };

	// 此调用将尝试从已生成状态（仅在 BeginPlay 中设置）开始，逐步推进初始化状态链，依次经历各个数据初始化阶段，直至达到可投入游戏运行的状态。
	ContinueInitStateChain(StateChain);
}

void UYcPawnExtensionComponent::InitializeAbilitySystem(UYcAbilitySystemComponent* InASC, AActor* InOwnerActor)
{
#if DEBUG_INIT_STATE
	const FString LocalRoleName = StaticEnum<ENetRole>()->GetNameStringByValue((int64)GetOwner()->GetLocalRole());
	UE_LOG(LogYcGameplay, Log, TEXT("InitializeAbilitySystem [%s] / Role [%s]"), *GetNameSafe(GetOwner()), *LocalRoleName);
#endif
	check(InASC);
	check(InOwnerActor);
	
	// 如果ASC没有变化则不需要重新初始化
	if (AbilitySystemComponent == InASC) return;

	// 清理旧的技能系统组件
	if (AbilitySystemComponent)
	{
		UninitializeAbilitySystem();
	}
	
	APawn* Pawn = GetPawnChecked<APawn>();
	const AActor* ExistingAvatar = InASC->GetAvatarActor();
	
	// 如果已有其他Pawn作为ASC的Avatar，需要先移除它
	// 这可能发生在客户端网络延迟的情况下：新Pawn先完成生成且被控制，旧Pawn才被移除
	if ((ExistingAvatar != nullptr) && (ExistingAvatar != Pawn))
	{
		ensure(!ExistingAvatar->HasAuthority());

		if (UYcPawnExtensionComponent* OtherExtensionComponent = FindPawnExtensionComponent(ExistingAvatar))
		{
			OtherExtensionComponent->UninitializeAbilitySystem();
		}
	}
	
	AbilitySystemComponent = InASC;
	// 初始化ASC的Actor信息，设置Owner和Avatar
	AbilitySystemComponent->InitAbilityActorInfo(InOwnerActor, Pawn);
	
	if (ensure(PawnData))
	{
		InASC->SetTagRelationshipMapping(PawnData->TagRelationshipMapping);
	}

	OnAbilitySystemInitialized.Broadcast();
}

void UYcPawnExtensionComponent::UninitializeAbilitySystem()
{
	if (!AbilitySystemComponent) return;

	// 只有当我们仍然是ASC的Avatar时才需要反初始化（否则其他Pawn已经做过了）
	if (AbilitySystemComponent->GetAvatarActor() == GetOwner())
	{
		// 收集需要忽略的Ability类型（带有此标签的Ability在死亡后不会被取消）
		FGameplayTagContainer AbilityTypesToIgnore;
		AbilityTypesToIgnore.AddTag(YcGameplayTags::Ability_Behavior_SurvivesDeath);

		// 取消所有Ability、清除输入、移除GameplayCue
		AbilitySystemComponent->CancelAbilities(nullptr, &AbilityTypesToIgnore);
		AbilitySystemComponent->ClearAbilityInput();
		AbilitySystemComponent->RemoveAllGameplayCues();

		if (AbilitySystemComponent->GetOwnerActor() != nullptr)
		{
			// 清除Avatar配对
			AbilitySystemComponent->SetAvatarActor(nullptr);
		}
		else
		{
			// 如果ASC没有有效的Owner，需要清除所有Actor信息，而不仅是Avatar配对
			AbilitySystemComponent->ClearActorInfo();
		}

		OnAbilitySystemUninitialized.Broadcast();
	}

	AbilitySystemComponent = nullptr;
}

void UYcPawnExtensionComponent::OnAbilitySystemInitialized_RegisterAndCall(const FSimpleMulticastDelegate::FDelegate& Delegate)
{
	if (!OnAbilitySystemInitialized.IsBoundToObject(Delegate.GetUObject()))
	{
		OnAbilitySystemInitialized.Add(Delegate);
	}

	if (AbilitySystemComponent)
	{
		Delegate.Execute();
	}
}

void UYcPawnExtensionComponent::OnAbilitySystemUninitialized_Register(const FSimpleMulticastDelegate::FDelegate& Delegate)
{
	if (!OnAbilitySystemUninitialized.IsBoundToObject(Delegate.GetUObject()))
	{
		OnAbilitySystemUninitialized.Add(Delegate);
	}
}

void UYcPawnExtensionComponent::HandleControllerChanged()
{
	// 如果ASC存在且我们是其Avatar，需要更新Owner信息
	if (AbilitySystemComponent && (AbilitySystemComponent->GetAvatarActor() == GetPawnChecked<APawn>()))
	{
		ensure(AbilitySystemComponent->AbilityActorInfo->OwnerActor == AbilitySystemComponent->GetOwnerActor());
		if (AbilitySystemComponent->GetOwnerActor() == nullptr)
		{
			// Owner已经失效，需要反初始化ASC
			UninitializeAbilitySystem();
		}
		else
		{
			// 刷新ASC的Actor信息
			AbilitySystemComponent->RefreshAbilityActorInfo();
		}
	}

	// 尝试推进初始化状态
	CheckDefaultInitialization();
}

void UYcPawnExtensionComponent::HandlePlayerStateReplicated()
{
	// PlayerState复制完成，尝试推进初始化状态
	CheckDefaultInitialization();
}

void UYcPawnExtensionComponent::SetupPlayerInputComponent()
{
	// InputComponent设置完成，尝试推进初始化状态
	CheckDefaultInitialization();
}