// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "AbilitySystem/Abilities/YcGameplayAbility_Death.h"

#include "YcAbilitySystemComponent.h"
#include "YcGameplayTags.h"
#include "YiChenGameplay.h"
#include "Abilities/GameplayAbility.h"
#include "Character/YcHealthComponent.h"
#include "Utils/CommonSimpleUtil.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameplayAbility_Death)

UYcGameplayAbility_Death::UYcGameplayAbility_Death(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;	// 实例化政策：为每个Actor实例化技能
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;	// 网络执行政策：仅服务器执行

	bAutoStartDeath = true;

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		// 默认为CDO对象添加能力触发标签。由HealthComponent的HandleOutOfHealth函数中调用AbilitySystemComponent->HandleGameplayEvent函数触发GameplayEvent_Death
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = YcGameplayTags::GameplayEvent_Death;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

void UYcGameplayAbility_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	check(ActorInfo);

	UYcAbilitySystemComponent* ASC = CastChecked<UYcAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());

	// 死亡后会取消一些激活了的技能，而这里则是指定那些不会因为死亡而被取消的技能标签列表
	FGameplayTagContainer AbilityTypesToIgnore;
	AbilityTypesToIgnore.AddTag(YcGameplayTags::Ability_Behavior_SurvivesDeath);

	// 取消除了拥有AbilityTypesToIgnore中的标签的技能外的所有技能，并阻止其他技能开始
	ASC->CancelAbilities(nullptr, &AbilityTypesToIgnore, this);

	SetCanBeCanceled(false);

	// 将激活组改为阻塞性排他，阻止其他技能激活
	if (!ChangeActivationGroup(EYcAbilityActivationGroup::Exclusive_Blocking))
	{
		UE_LOG(LogYcGameplay, Error, TEXT("UYcGameplayAbility_ShooterDeath::ActivateAbility: Ability [%s] failed to change activation group to blocking."), *GetName());
	}

	if (bAutoStartDeath)
	{
		StartDeath();
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UYcGameplayAbility_Death::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	check(ActorInfo);
	
	// 总是在技能结束时尝试完成死亡，以防技能没有结束
	// 如果死亡还没有开始，这个函数不会做任何事情
	FinishDeath();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UYcGameplayAbility_Death::StartDeath()
{
	UYcHealthComponent* HealthComponent = UYcHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo());
	if(!HealthComponent)
	{
		const FString ErrMsg = FString::Printf(TEXT("StartDeath failed,[%s] get UYcHealthComponent invalid."), GetAvatarActorFromActorInfo() ? *GetAvatarActorFromActorInfo()->GetName() : TEXT("Invalid avatar actor from actor info."));
		YC_LOG(LogYcGameplay, Error, *ErrMsg);
		return;
	}
	
	if (HealthComponent->GetDeathState() == EYcDeathState::NotDead)
	{
		HealthComponent->StartDeath();
	}
}

void UYcGameplayAbility_Death::FinishDeath()
{
	UYcHealthComponent* HealthComponent = UYcHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo());
	if(!HealthComponent)
	{
		const FString ErrMsg = FString::Printf(TEXT("FinishDeath failed,[%s] get UYcHealthComponent invalid."), GetAvatarActorFromActorInfo() ? *GetAvatarActorFromActorInfo()->GetName() : TEXT("Invalid avatar actor from actor info."));
		YC_LOG(LogYcGameplay, Error, *ErrMsg);
		return;
	}
	
	if (HealthComponent->GetDeathState() == EYcDeathState::DeathStarted)
	{
		HealthComponent->FinishDeath();
	}
}