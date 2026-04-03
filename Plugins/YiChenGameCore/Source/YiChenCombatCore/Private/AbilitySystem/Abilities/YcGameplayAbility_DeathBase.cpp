// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "AbilitySystem/Abilities/YcGameplayAbility_DeathBase.h"

#include "Health/YcHealthComponent.h"
#include "YcAbilitySystemComponent.h"
#include "YcGameplayTags.h"
#include "YiChenCombatCore.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameplayAbility_DeathBase)

UYcGameplayAbility_DeathBase::UYcGameplayAbility_DeathBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;

	if (HasAnyFlags(RF_ClassDefaultObject))
	{
		FAbilityTriggerData TriggerData;
		TriggerData.TriggerTag = YcGameplayTags::GameplayEvent_Death;
		TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
		AbilityTriggers.Add(TriggerData);
	}
}

void UYcGameplayAbility_DeathBase::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	check(ActorInfo);

	UYcAbilitySystemComponent* ASC = CastChecked<UYcAbilitySystemComponent>(ActorInfo->AbilitySystemComponent.Get());

	FGameplayTagContainer AbilityTypesToIgnore;
	AbilityTypesToIgnore.AddTag(YcGameplayTags::Ability_Behavior_SurvivesDeath);

	ASC->CancelAbilities(nullptr, &AbilityTypesToIgnore, this);
	SetCanBeCanceled(false);

	if (!ChangeActivationGroup(EYcAbilityActivationGroup::Exclusive_Blocking))
	{
		UE_LOG(LogCombatCore, Error, TEXT("UYcGameplayAbility_DeathBase[%s] failed to change activation group to blocking."), *GetName());
	}

	if (bAutoStartDeath)
	{
		StartDeath();
	}

	K2_OnDeathSequenceStarted();

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UYcGameplayAbility_DeathBase::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	check(ActorInfo);

	K2_OnDeathSequenceEnding();
	FinishDeath();

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UYcGameplayAbility_DeathBase::StartDeath()
{
	UYcHealthComponent* HealthComponent = UYcHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo());
	if (!HealthComponent)
	{
		UE_LOG(LogCombatCore, Error, TEXT("StartDeath failed, invalid UYcHealthComponent for avatar[%s]."),
			GetAvatarActorFromActorInfo() ? *GetAvatarActorFromActorInfo()->GetName() : TEXT("None"));
		return;
	}

	if (HealthComponent->GetDeathState() == EYcDeathState::NotDead)
	{
		HealthComponent->StartDeath();
	}
}

void UYcGameplayAbility_DeathBase::FinishDeath()
{
	UYcHealthComponent* HealthComponent = UYcHealthComponent::FindHealthComponent(GetAvatarActorFromActorInfo());
	if (!HealthComponent)
	{
		UE_LOG(LogCombatCore, Error, TEXT("FinishDeath failed, invalid UYcHealthComponent for avatar[%s]."),
			GetAvatarActorFromActorInfo() ? *GetAvatarActorFromActorInfo()->GetName() : TEXT("None"));
		return;
	}

	if (HealthComponent->GetDeathState() == EYcDeathState::DeathStarted)
	{
		HealthComponent->FinishDeath();
	}
}
