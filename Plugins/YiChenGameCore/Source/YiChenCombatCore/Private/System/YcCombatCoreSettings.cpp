// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcCombatCoreSettings.h"

#include "GameplayEffect.h"

namespace
{
	static TSubclassOf<UGameplayEffect> LoadOptionalGameplayEffectClass(const TSoftClassPtr<UGameplayEffect>& SoftClass)
	{
		if (SoftClass.IsNull())
		{
			return nullptr;
		}

		return SoftClass.LoadSynchronous();
	}
}

TSubclassOf<UGameplayEffect> UYcCombatCoreSettings::GetDamageSelfDestructGameplayEffect() const
{
	return LoadOptionalGameplayEffectClass(DamageSelfDestructGameplayEffect);
}

TSubclassOf<UGameplayEffect> UYcCombatCoreSettings::GetDamageGameplayEffect_SetByCaller() const
{
	return LoadOptionalGameplayEffectClass(DamageGameplayEffect_SetByCaller);
}

TSubclassOf<UGameplayEffect> UYcCombatCoreSettings::GetHealGameplayEffect_SetByCaller() const
{
	return LoadOptionalGameplayEffectClass(HealGameplayEffect_SetByCaller);
}
