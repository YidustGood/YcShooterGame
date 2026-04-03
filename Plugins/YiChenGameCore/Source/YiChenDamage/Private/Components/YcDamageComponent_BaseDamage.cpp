// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Components/YcDamageComponent_BaseDamage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageComponent_BaseDamage)

UYcDamageComponent_BaseDamage::UYcDamageComponent_BaseDamage()
{
	Priority = 10;
}

void UYcDamageComponent_BaseDamage::Execute_Implementation(FYcAttributeSummaryParams& Params)
{
	FYcDamageSummaryParams& DamageParams = GetDamageParams(Params);

	float BaseDamage = FixedBaseDamage;

	// 如果使用 SetByCaller，尝试获取值
	if (bUseSetByCaller && SetByCallerTag.IsValid())
	{
		bool bFound = false;
		const float SetByCallerValue = GetSetByCallerValue(Params, SetByCallerTag, &bFound);
		if (bFound)
		{
			BaseDamage = SetByCallerValue;
		}
	}

	// 应用倍率
	BaseDamage *= BaseDamageMultiplier;

	// 设置基础伤害
	DamageParams.SetBaseDamage(BaseDamage);

	if (bEnableDebugLog)
	{
		LogDebug(FString::Printf(TEXT("BaseDamage set to: %.2f (multiplier: %.2f)"), BaseDamage, BaseDamageMultiplier));
	}
}
