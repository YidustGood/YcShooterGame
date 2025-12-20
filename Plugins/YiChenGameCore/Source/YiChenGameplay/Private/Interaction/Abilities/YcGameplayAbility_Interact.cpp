// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Interaction/Abilities/YcGameplayAbility_Interact.h"

#include "AbilitySystemComponent.h"
#include "NativeGameplayTags.h"
#include "YiChenGameplay.h"
#include "Abilities/Tasks/AbilityTask_WaitInputPress.h"
#include "Abilities/Tasks/AbilityTask_WaitInputRelease.h"
#include "Interaction/YcInteractableTarget.h"
#include "Interaction/YcInteractionStatics.h"
#include "Interaction/YcInteractionTypes.h"
#include "Interaction/Tasks/AbilityTask_GrantNearbyInteraction.h"
#include "Interaction/Tasks/AbilityTask_WaitForInteractableTargets_SingleLineTrace.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Utils/CommonSimpleUtil.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameplayAbility_Interact)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Ability_Interaction_Activate, "Ability.Interaction.Activate");
UE_DEFINE_GAMEPLAY_TAG(TAG_INTERACTION_DURATION_MESSAGE, "Ability.Interaction.Duration.Message");
UE_DEFINE_GAMEPLAY_TAG(TAG_Interaction_UpdateInteractions, "Ability.Interaction.UpdateInteractions");

// ==================== 性能计数器声明 ====================
DECLARE_CYCLE_STAT(TEXT("TriggerInteraction"), STAT_YcInteraction_TriggerInteraction, STATGROUP_YcInteraction);
DECLARE_CYCLE_STAT(TEXT("UpdateInteractions"), STAT_YcInteraction_UpdateInteractions, STATGROUP_YcInteraction);

UYcGameplayAbility_Interact::UYcGameplayAbility_Interact(const FObjectInitializer& ObjectInitializer)
{
	ActivationPolicy = EYcAbilityActivationPolicy::OnSpawn;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
}

void UYcGameplayAbility_Interact::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
												  const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	const UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();
	
	// 交互物GA授予仅在权威端执行
	if (AbilitySystem && AbilitySystem->GetOwnerRole() == ROLE_Authority)
	{
		// 获取周围可交互物体并将其InteractionOption中配置的GameplayAbility赋予当前玩家的的ASC组件, 物体的具体交互逻辑就可以在这个GameplayAbility中进行实现
		UAbilityTask_GrantNearbyInteraction* Task = UAbilityTask_GrantNearbyInteraction::GrantAbilitiesForNearbyInteractors(this, NearbyInteractionScanRange, NearbyInteractionScanRate);
		Task->ReadyForActivation();
	}
	
	// 玩家视野目标交互物更新逻辑仅在操控端进行
	AController* Controller = GetControllerFromActorInfo();
	if (Controller && Controller->IsLocalPlayerController())
	{
		LookForInteractables();
		ListenInteractPress();
	}
	
	YC_LOG(LogYcGameplay, Log, TEXT("技能-视野目标交互对象检测开启"));
}

void UYcGameplayAbility_Interact::UpdateInteractions(const TArray<FYcInteractionOption>& InteractiveOptions)
{
	YC_INTERACTION_SCOPE_CYCLE_COUNTER(UpdateInteractions);
	
	CurrentOptions = InteractiveOptions;
	if (CurrentOptions.Num() == 0) return;
	
	/**
	 * 如果在运行时Option没有设置交互UI类就指定为默认的
	 * 但是UAbilityTask_WaitForInteractableTargets::UpdateInteractableOptions先于当前这个UpdateInteractions函数执行
	 * 里面会先调用IYcInteractableTarget::OnPlayerFocusBegin, UYcInteractableComponent实现了该接口并在其中做UI添加的逻辑
	 * 但是遇上InteractionWidgetClass为空的时候, 这里的回退默认值会在UI添加逻辑之后执行, 导致交互组件在第一次交互时无法正确创建UI
	 * 简单的解决办法是在UYcInteractableComponent::OnPlayerFocusBegin中设置下一帧再执行UI创建逻辑
	 */
	for (auto& Option : CurrentOptions)
	{
		if (Option.InteractionWidgetClass.IsNull())
		{
			Option.InteractionWidgetClass = DefaultInteractionWidgetClass;
			Option.InteractableTarget->UpdateInteractionOption(Option);
		}
	}
}

void UYcGameplayAbility_Interact::TriggerInteraction()
{
	YC_INTERACTION_SCOPE_CYCLE_COUNTER(TriggerInteraction);
	
	if (CurrentOptions.Num() == 0) return;

	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponentFromActorInfo();

	if(!AbilitySystem) return;

	// 获取第一个交互选项
	const FYcInteractionOption& InteractionOption = CurrentOptions[0];
	
	AActor* Instigator = GetAvatarActorFromActorInfo();
	// 获取交互选项所属的Actor对象
	AActor* InteractableTargetActor = UYcInteractionStatics::GetActorFromInteractableTarget(InteractionOption.InteractableTarget);

	// 允许交互目标在我们传递事件数据前进行自定义，以便其注入仅自身知道的额外数据
	// （例如物体专属的自定义参数）
	FGameplayEventData Payload;
	Payload.EventTag = TAG_Ability_Interaction_Activate;
	Payload.Instigator = Instigator;
	Payload.Target = InteractableTargetActor;

	// 如有需要，可让交互目标进一步修改事件数据，例如墙上的按钮希望指定要打开的门 Actor
	// 它可以将 Payload.Target 覆盖为门 Actor，方便能力在正确目标上执行
	// ——从而完成按钮→门的间接交互。
	InteractionOption.InteractableTarget->CustomizeInteractionEventData(TAG_Ability_Interaction_Activate, Payload);

	// 从 Payload 中取出目标 Actor，用作此次交互的 Avatar；交互源 InteractableTarget Actor 作为 Owner
	// （区分所有者与化身角色）。
	AActor* TargetActor = const_cast<AActor*>(ToRawPtr(Payload.Target));

	// 构建交互所需的 ActorInfo 结构体
	FGameplayAbilityActorInfo ActorInfo;
	ActorInfo.InitFromActor(InteractableTargetActor, TargetActor, InteractionOption.TargetAbilitySystem);

	// 使用事件标签触发目标能力
	const bool bSuccess = InteractionOption.TargetAbilitySystem->TriggerAbilityFromGameplayEvent(
		InteractionOption.TargetInteractionAbilityHandle,
		&ActorInfo,
		TAG_Ability_Interaction_Activate,
		&Payload,
		*InteractionOption.TargetAbilitySystem
	);
}

void UYcGameplayAbility_Interact::LookForInteractables()
{
	FYcInteractionQuery Query;
	Query.RequestingAvatar = GetAvatarActorFromActorInfo();
	Query.RequestingController = GetControllerFromActorInfo();
	FCollisionProfileName();
	
	auto* Task = UAbilityTask_WaitForInteractableTargets_SingleLineTrace::WaitForInteractableTargets_SingleLineTrace(
this, 
			Query, 
			TraceProfile, 
			MakeTargetLocationInfoFromOwnerActor(), 
			ViewedInteractionScanRange, 
			ViewedInteractionScanRate);
	
	// 绑定交互目标变化的回调函数, 以处理变化
	Task->OnInteractableTargetsChanged.AddDynamic(this, &ThisClass::UpdateInteractions);
	Task->ReadyForActivation();
}

void UYcGameplayAbility_Interact::ListenInteractPress()
{
	UAbilityTask_WaitInputPress* InputPressTask = UAbilityTask_WaitInputPress::WaitInputPress(this);
	check(InputPressTask);
	InputPressTask->ReadyForActivation();
	UAbilityTask_WaitInputRelease* InputReleaseTask = UAbilityTask_WaitInputRelease::WaitInputRelease(this);
	check(InputReleaseTask);
	InputReleaseTask->ReadyForActivation();
	InputPressTask->OnPress.AddDynamic(this, &ThisClass::OnInputPressed);
}

void UYcGameplayAbility_Interact::OnInputPressed(float TimeWaited)
{
	// 交互按键按下触发当前交互目标对象的交互技能
	TriggerInteraction();
	// 刷新交互按键按下监听
	ListenInteractPress();
	
	if (CurrentOptions.Num() != 0)
	{
		
		UE_LOG(LogYcGameplay, Log, TEXT("通过按键触发交互技能: %s"), *UKismetSystemLibrary::GetClassDisplayName(CurrentOptions[0].InteractionAbilityToGrant));
	}
}

void UYcGameplayAbility_Interact::OnInputReleased(float TimeHeld)
{
	// @TODO 或许可以增加取消技能逻辑, 以实现需要长按进行交互的功能
}