// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "AbilitySystem/Abilities/YcGameplayAbility_ResetCharacter.h"

#include "GameplayAbilitySpecHandle.h"
#include "YcAbilitySystemComponent.h"
#include "YcGameplayTags.h"
#include "Character/YcCharacter.h"
#include "GameFramework/GameplayMessageSubsystem.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameplayAbility_ResetCharacter)

UYcGameplayAbility_ResetCharacter::UYcGameplayAbility_ResetCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 设置为每个Actor一个实例，确保每个角色都有独立的重置技能实例
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	// 设置为服务器发起执行，保证重置逻辑只在服务器端执行，避免客户端作弊
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	// 仅在类默认对象（CDO）上设置触发器，避免每个实例重复添加
	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		// 添加技能触发标签到CDO，使技能能够响应 GameplayEvent.RequestReset 事件
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = YcGameplayTags::GameplayEvent_RequestReset;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

void UYcGameplayAbility_ResetCharacter::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	check(ActorInfo);

	UYcAbilitySystemComponent* YCASC = CastChecked<UYcAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());

	// 准备需要忽略的技能类型标签容器
	// 标记为 SurvivesDeath 的技能在角色重置时不会被取消（例如：复活技能、被动技能等）
	FGameplayTagContainer AbilityTypesToIgnore;
	AbilityTypesToIgnore.AddTag(YcGameplayTags::Ability_Behavior_SurvivesDeath);

	// 取消所有正在运行的技能（除了需要忽略的技能），并阻止新技能启动
	// 这确保角色在重置时处于干净的状态，没有残留的技能效果
	YCASC->CancelAbilities(nullptr, &AbilityTypesToIgnore, this);

	// 设置该技能不可被取消，确保重置流程完整执行
	SetCanBeCanceled(false);

	// 从角色对象执行实际的重置操作
	// 可以重写AYcCharacter::Reset()实现不同的逻辑
	if (AYcCharacter* YcChar = Cast<AYcCharacter>(CurrentActorInfo->AvatarActor.Get()))
	{
		YcChar->Reset();
	}

	// 广播玩家重置消息，通知其他系统（UI、音效、特效等）玩家已重置
	// 其他系统可以监听此消息来执行相应的重置逻辑
	FYcPlayerResetMessage Message;
	Message.OwnerPlayerState = CurrentActorInfo->OwnerActor.Get();
	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(YcGameplayTags::GameplayEvent_Reset, Message);

	// 调用父类的激活逻辑
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 立即结束技能，因为重置是一次性操作
	// bReplicateEndAbility = true: 将技能结束状态同步到客户端
	// bWasCanceled = false: 标记为正常结束而非被取消
	const bool bReplicateEndAbility = true;
	const bool bWasCanceled = false;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEndAbility, bWasCanceled);
}