// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/YcMeleeWeaponInstance.h"

#include "YcGameplayEffectContext.h"
#include "Weapons/YcWeaponRuntimePayloads.h"
#include "Weapons/Fragments/YcFragment_MeleeAttackConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcMeleeWeaponInstance)

UYcMeleeWeaponInstance::UYcMeleeWeaponInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UYcMeleeWeaponInstance::BuildCurrentDamageProfileView(const FYcGameplayEffectContext* EffectContext, FYcWeaponDamageProfileView& OutView) const
{
	if (EffectContext)
	{
		if (const FYcMeleeAttackRuntimePayload* MeleePayload = EffectContext->FindRuntimePayload<FYcMeleeAttackRuntimePayload>())
		{
			if (const FYcFragment_MeleeAttackConfig* MeleeConfig = GetTypedFragment<FYcFragment_MeleeAttackConfig>())
			{
				OutView = FYcWeaponDamageProfileView();
				OutView.HitZoneDamageMultipliers = MeleeConfig->GetAttackProfile(MeleePayload->AttackType).HitZoneDamageMultipliers;
				return true;
			}
		}
	}

	return Super::BuildCurrentDamageProfileView(EffectContext, OutView);
}
