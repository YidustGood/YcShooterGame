// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "AbilitySystem/Executions/YcHealExecution.h"

#include "AbilitySystem/Attributes/YcCombatSet.h"
#include "AbilitySystem/Attributes/YcShooterHealthSet.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcHealExecution)

/**
 * 治疗计算的静态数据
 * 用于缓存属性捕获定义，避免重复创建
 */
struct FHealStatics
{
	/** 基础治疗属性捕获定义，从源对象的CombatSet中捕获BaseHeal */
	FGameplayEffectAttributeCaptureDefinition BaseHealDef;

	FHealStatics()
	{
		// bSnapshot = false: 每次执行时获取实时的BaseHeal值，而不是GE Spec创建时的快照值
		// 对于周期性治疗GE，需要实时值以反映属性变化（如buff/debuff的添加或移除）
		BaseHealDef = FGameplayEffectAttributeCaptureDefinition(UYcCombatSet::GetBaseHealAttribute(), EGameplayEffectAttributeCaptureSource::Source, false);
	}
};

/**
 * 获取治疗计算静态数据（单例）
 * @return 静态数据引用
 */
static FHealStatics& HealStatics()
{
	static FHealStatics Statics;
	return Statics;
}

UYcHealExecution::UYcHealExecution()
{
	// 添加要捕获的与计算相关的属性
	RelevantAttributesToCapture.Add(HealStatics().BaseHealDef);
}

void UYcHealExecution::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
#if WITH_SERVER_CODE
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = SourceTags;
	EvaluateParameters.TargetTags = TargetTags;

	// 从治疗者的CombatSet属性集中获取BaseHeal，也就是基础治疗值
	// 治疗者的CombatSet属性集由UYcShooterCombatComponent组件提供和维护
	// 注意：BaseHealDef的bSnapshot参数设置为false，确保每次执行时都获取实时的属性值
	float BaseHeal = 0.0f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(HealStatics().BaseHealDef, EvaluateParameters, BaseHeal);

	// 计算最终治疗值（确保不为负数）
	const float HealingDone = FMath::Max(0.0f, BaseHeal);

	if (HealingDone > 0.0f)
	{
		// 应用治疗修改器，这会变成目标的+生命值
		// UYcShooterHealthSet::PostGameplayEffectExecute会将Healing转换为+Health，以实现对玩家进行治疗
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(UYcShooterHealthSet::GetHealingAttribute(), EGameplayModOp::Additive, HealingDone));
	}
#endif // #if WITH_SERVER_CODE
}