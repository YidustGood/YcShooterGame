// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/Fragments/YcFragment_MeleeAttackConfig.h"

#include "NativeGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcFragment_MeleeAttackConfig)

namespace
{
	FGameplayTag GetTag(const TCHAR* TagName)
	{
		return FGameplayTag::RequestGameplayTag(FName(TagName), false);
	}
}

FYcFragment_MeleeAttackConfig::FYcFragment_MeleeAttackConfig()
{
	LightAttack.ActionTag = GetTag(TEXT("Asset.Weapon.Action.Melee.Light"));
	HeavyAttack.ActionTag = GetTag(TEXT("Asset.Weapon.Action.Melee.Heavy"));
	HeavyAttack.BaseDamage = 50.0f;
	HeavyAttack.TraceShape = EYcMeleeTraceShape::Capsule;
	HeavyAttack.TraceCapsuleRadius = 8.0f;
	HeavyAttack.TraceCapsuleHalfHeight = 12.0f;
	HeavyAttack.BackstabDamageMultiplier = 1.5f;
}

const FYcMeleeAttackProfile& FYcFragment_MeleeAttackConfig::GetAttackProfile(EYcMeleeAttackType AttackType) const
{
	return AttackType == EYcMeleeAttackType::Heavy ? HeavyAttack : LightAttack;
}

FString FYcFragment_MeleeAttackConfig::GetDebugString() const
{
	return FString::Printf(TEXT("MeleeAttackConfig: Light=%.1f(%d) Heavy=%.1f(%d)"),
		LightAttack.BaseDamage,
		LightAttack.HitZoneDamageMultipliers.Num(),
		HeavyAttack.BaseDamage,
		HeavyAttack.HitZoneDamageMultipliers.Num());
}
