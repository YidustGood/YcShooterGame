// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Components/YcDamageComponent_Dodge.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageComponent_Dodge)

UYcDamageComponent_Dodge::UYcDamageComponent_Dodge()
{
	Priority = 15;
	DebugName = TEXT("Dodge");
}

void UYcDamageComponent_Dodge::Execute_Implementation(FYcAttributeSummaryParams& Params)
{
	if (!bEnableDodge)
	{
		return;
	}

	// 获取闪避率
	float DodgeRate = DefaultDodgeRate;
	if (DodgeRateAttribute.IsValid())
	{
		bool bFound = false;
		DodgeRate = GetTargetAttribute(Params, DodgeRateAttribute, &bFound);
		if (!bFound)
		{
			DodgeRate = DefaultDodgeRate;
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

	// 随机判定闪避
	const float RandomValue = RandomStream.FRand();
	if (RandomValue < DodgeRate)
	{
		// 闪避成功，取消伤害执行
		Params.bCancelExecution = true;

		// 添加闪避标签
		if (DodgeTag.IsValid())
		{
			Params.TemporaryTags.AddTag(DodgeTag);
		}

		if (bEnableDebugLog)
		{
			LogDebug(FString::Printf(TEXT("Dodge successful! Rate: %.2f%%"), DodgeRate * 100.0f));
		}
	}
	else if (bEnableDebugLog)
	{
		LogDebug(FString::Printf(TEXT("No dodge. Random: %.2f, Rate: %.2f"), RandomValue, DodgeRate));
	}
}
