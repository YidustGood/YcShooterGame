// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcAbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAbilitySystemComponent)

UYcAbilitySystemComponent::UYcAbilitySystemComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

UYcAbilitySystemComponent* UYcAbilitySystemComponent::GetAbilitySystemComponentFromActor(const AActor* Actor,
	bool LookForComponent)
{
	return Cast<UYcAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor, LookForComponent));
}

bool UYcAbilitySystemComponent::TryCancelAbilityByClass(TSubclassOf<UGameplayAbility> InAbilityToCancel)
{
	bool bSuccess = false;

	const UGameplayAbility* const InAbilityCDO = InAbilityToCancel.GetDefaultObject();

	for (const FGameplayAbilitySpec& Spec : ActivatableAbilities.Items)
	{
		if (Spec.IsActive() && Spec.Ability == InAbilityCDO)
		{
			CancelAbilityHandle(Spec.Handle);
			bSuccess = true;
			break;
		}
	}

	return bSuccess;
}
