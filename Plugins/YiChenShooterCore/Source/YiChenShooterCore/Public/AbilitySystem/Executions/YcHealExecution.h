// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayEffectExecutionCalculation.h"
#include "YcHealExecution.generated.h"

/**
 * 治疗执行计算
 * 游戏效果用于对健康属性施加治疗量的执行计算类
 * 从CombatSet获取基础治疗值，计算最终治疗量并输出到HealthSet的Healing属性
 */
UCLASS()
class YICHENSHOOTERCORE_API UYcHealExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
public:
	/**
	 * 构造函数
	 * 初始化需要捕获的属性（BaseHeal）
	 */
	UYcHealExecution();

protected:
	/**
	 * 执行治疗计算
	 * 从CombatSet获取基础治疗值，计算最终治疗量，最终输出到HealthSet的Healing属性
	 * @param ExecutionParams 执行参数
	 * @param OutExecutionOutput 执行输出
	 */
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};