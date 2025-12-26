// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "AbilitySystem/Attributes/YcCombatSet.h"

#include "Net/UnrealNetwork.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcCombatSet)

class FLifetimeProperty;

UYcCombatSet::UYcCombatSet()
	: BaseDamage(0.0f)
	, BaseHeal(0.0f)
{
}

void UYcCombatSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UYcCombatSet, BaseDamage, COND_OwnerOnly, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UYcCombatSet, BaseHeal, COND_OwnerOnly, REPNOTIFY_Always);
}

void UYcCombatSet::OnRep_BaseDamage(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UYcCombatSet, BaseDamage, OldValue);
}

void UYcCombatSet::OnRep_BaseHeal(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UYcCombatSet, BaseHeal, OldValue);

}