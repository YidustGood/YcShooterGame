// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Settings/YcDamageTypeSettings.h"

const FYcDamageTypeDefinition* UYcDamageTypeSettings::FindDamageType(const FGameplayTag& DamageTypeTag) const
{
	for (const FYcDamageTypeDefinition& Def : DamageTypes)
	{
		if (Def.DamageTypeTag == DamageTypeTag)
		{
			return &Def;
		}
	}
	return nullptr;
}

float UYcDamageTypeSettings::GetResistanceCoefficient(const FGameplayTag& DamageTypeTag) const
{
	if (const FYcDamageTypeDefinition* Def = FindDamageType(DamageTypeTag))
	{
		return Def->BaseResistanceCoefficient;
	}
	return 1.0f;
}
