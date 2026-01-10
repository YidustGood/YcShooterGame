// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayEffectExecutionCalculation.h"
#include "YcDamageExecution.generated.h"

/**
 * 伤害执行计算
 * 游戏效果用于对健康属性施加伤害的执行计算类
 * 计算最终伤害值，考虑距离衰减、物理材质衰减、团队伤害规则等因素
 */
UCLASS()
class YICHENSHOOTERCORE_API UYcDamageExecution : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()
public:
	UYcDamageExecution();

protected:

	/**
	 * 执行伤害计算
	 * 从CombatSet获取基础伤害，应用距离衰减、物理材质衰减、团队伤害规则等，最终输出到HealthSet的Damage属性
	 * @param ExecutionParams 执行参数
	 * @param OutExecutionOutput 执行输出
	 */
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};