// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Executions/YcDamageExecutionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageExecutionComponent)

UYcDamageExecutionComponent::UYcDamageExecutionComponent()
{
}

bool UYcDamageExecutionComponent::ShouldExecute(const FYcAttributeSummaryParams& Params, const FGameplayTagContainer& InSourceTags, const FGameplayTagContainer& InTargetTags) const
{
	// 首先检查基类条件
	if (!Super::ShouldExecute(Params, InSourceTags, InTargetTags))
	{
		return false;
	}

	// 检查伤害类型过滤
	const FYcDamageSummaryParams& DamageParams = GetDamageParams(Params);
	if (!IsDamageTypeMatched(DamageParams.DamageTypeTag))
	{
		return false;
	}

	return true;
}

void UYcDamageExecutionComponent::ApplyDamageToAttribute(FYcAttributeSummaryParams& Params, const FGameplayAttribute& Attribute, float Damage) const
{
	AddOutputModifier(Params, Attribute, -FMath::Abs(Damage), EGameplayModOp::Additive);
}
