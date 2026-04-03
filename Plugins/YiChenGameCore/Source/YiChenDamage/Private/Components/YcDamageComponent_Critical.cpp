// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Components/YcDamageComponent_Critical.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageComponent_Critical)

UYcDamageComponent_Critical::UYcDamageComponent_Critical()
{
	Priority = 50;
	DebugName = TEXT("Critical");
}

void UYcDamageComponent_Critical::Execute_Implementation(FYcAttributeSummaryParams& Params)
{
	if (!bEnableCritical)
	{
		return;
	}

	FYcDamageSummaryParams& DamageParams = GetDamageParams(Params);

	// 获取暴击率
	float CriticalRate = DefaultCriticalRate;
	if (CriticalRateAttribute.IsValid())
	{
		bool bFound = false;
		CriticalRate = GetSourceAttribute(Params, CriticalRateAttribute, &bFound);
		if (!bFound)
		{
			CriticalRate = DefaultCriticalRate;
		}
	}

	// 初始化随机流
	if (bUseFixedSeed)
	{
		RandomStream.Initialize(FixedSeed);
	}
	else
	{
		RandomStream.GenerateNewSeed();
	}

	// 随机判定暴击
	const float RandomValue = RandomStream.FRand();
	if (RandomValue < CriticalRate)
	{
		// 暴击成功
		float CriticalDamage = DefaultCriticalDamage;
		if (CriticalDamageAttribute.IsValid())
		{
			bool bFound = false;
			CriticalDamage = GetSourceAttribute(Params, CriticalDamageAttribute, &bFound);
			if (!bFound)
			{
				CriticalDamage = DefaultCriticalDamage;
			}
		}

		// 应用暴击倍率到系数乘区
		Params.Coefficient *= CriticalDamage;

		// 添加暴击标签
		if (CriticalTag.IsValid())
		{
			Params.TemporaryTags.AddTag(CriticalTag);
		}

		// 记录加成
		Params.AddInfluence(EYcAttributeInfluencePriority::Coefficient, CriticalDamage, CriticalTag);

		if (bEnableDebugLog)
		{
			LogDebug(FString::Printf(TEXT("Critical hit! Rate: %.2f%%, Damage multiplier: %.2f"), CriticalRate * 100.0f, CriticalDamage));
		}
	}
	else if (bEnableDebugLog)
	{
		LogDebug(FString::Printf(TEXT("No critical. Random: %.2f, Rate: %.2f"), RandomValue, CriticalRate));
	}
}
