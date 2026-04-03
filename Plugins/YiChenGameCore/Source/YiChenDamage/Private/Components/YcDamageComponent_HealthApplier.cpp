// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Components/YcDamageComponent_HealthApplier.h"
#include "YcDamageGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageComponent_HealthApplier)

UYcDamageComponent_HealthApplier::UYcDamageComponent_HealthApplier()
{
	Priority = 200;
	DebugName = TEXT("HealthApplier");
}

void UYcDamageComponent_HealthApplier::Execute_Implementation(FYcAttributeSummaryParams& Params)
{
	FYcDamageSummaryParams& DamageParams = GetDamageParams(Params);

	// 如果执行被取消，不应用伤害
	if (Params.bCancelExecution)
	{
		if (bEnableDebugLog)
		{
			LogDebug(TEXT("Skipped: Execution canceled"));
		}
		return;
	}

	// 检查是否配置了伤害属性
	if (!DamageAttribute.IsValid())
	{
		if (bEnableDebugLog)
		{
			LogDebug(TEXT("Skipped: DamageAttribute not set"));
		}
		return;
	}

	// 计算最终伤害
	float FinalDamage = Params.GetFinalValue();

	// 应用最小/最大限制
	FinalDamage = FMath::Clamp(FinalDamage, MinDamage, MaxDamage);

	if (FinalDamage <= 0.0f)
	{
		if (bEnableDebugLog)
		{
			LogDebug(TEXT("Skipped: No damage to apply"));
		}
		return;
	}

	// 检查是否会归零（如果配置了 HealthAttribute）
	const bool bWillKill = WillTargetDie(Params, FinalDamage);

	// 应用伤害到 Damage 属性
	// AttributeSet 的 PostGameplayEffectExecute 会处理实际扣除逻辑
	AddOutputModifier(Params, DamageAttribute, FinalDamage);

	// 更新伤害信息
	DamageParams.DamageInfo.PracticalDamage = FinalDamage;

	// 检查生命值归零
	if (bWillKill && bAddOutOfHealthTag)
	{
		Params.bClamped = true;

		// 使用配置的标签或默认标签
		FGameplayTag TagToAdd = OutOfHealthTag;
		if (!TagToAdd.IsValid())
		{
			// 默认标签
			TagToAdd = YcDamageGameplayTags::Damage_Event_OutOfHealth;
		}

		if (TagToAdd.IsValid())
		{
			Params.TemporaryTags.AddTag(TagToAdd);
		}

		if (bEnableDebugLog)
		{
			LogDebug(TEXT("Target will be killed"));
		}
	}

	if (bEnableDebugLog)
	{
		LogDebug(FString::Printf(TEXT("Applied damage: %.2f via attribute: %s"), 
			FinalDamage, *DamageAttribute.GetName()));
	}
}

bool UYcDamageComponent_HealthApplier::WillTargetDie(FYcAttributeSummaryParams& Params, float FinalDamage) const
{
	// 如果没有配置 HealthAttribute，无法判断
	if (!HealthAttribute.IsValid())
	{
		return false;
	}

	// 获取当前生命值
	bool bFound = false;
	const float CurrentHealth = GetTargetAttribute(Params, HealthAttribute, &bFound);

	if (!bFound)
	{
		return false;
	}

	return (CurrentHealth - FinalDamage <= 0.0f);
}
