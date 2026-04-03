// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Components/YcDamageComponent_Influence.h"

#include "Subsystem/YcDamageInfluenceSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageComponent_Influence)

UYcDamageComponent_Influence::UYcDamageComponent_Influence()
{
	Priority = 25;
	DebugName = TEXT("InfluenceCollector");
}

void UYcDamageComponent_Influence::Execute_Implementation(FYcAttributeSummaryParams& Params)
{
	// 转换为伤害参数
	FYcDamageSummaryParams& DamageParams = static_cast<FYcDamageSummaryParams&>(Params);
	
	if (!bEnableInfluenceCollection)
	{
		return;
	}

	// 确定伤害类型标签
	FGameplayTag DamageTypeTag = DamageParams.DamageTypeTag;
	if (!DamageTypeTag.IsValid() && DefaultDamageTypeTag.IsValid())
	{
		DamageTypeTag = DefaultDamageTypeTag;
		DamageParams.DamageTypeTag = DamageTypeTag;
	}

	if (!DamageTypeTag.IsValid())
	{
		LogDebug(TEXT("No DamageTypeTag, skipping influence collection"));
		return;
	}

	// 获取 InfluenceSubsystem
	if (UWorld* World = GetWorld())
	{
		if (UYcDamageInfluenceSubsystem* InfluenceSubsystem = World->GetSubsystem<UYcDamageInfluenceSubsystem>())
		{
			// 应用所有加成
			InfluenceSubsystem->ApplyInfluences(DamageParams, DamageTypeTag);
			LogDebug(FString::Printf(TEXT("Applied influences for: %s"), *DamageTypeTag.ToString()));
		}
		else
		{
			LogDebug(TEXT("InfluenceSubsystem not found"));
		}
	}
}
