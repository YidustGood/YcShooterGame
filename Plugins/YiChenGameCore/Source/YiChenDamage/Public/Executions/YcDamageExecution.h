// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "YcDamageSummaryParams.h"
#include "Executions/YcAttributeExecution.h"
#include "YcDamageExecution.generated.h"

class IYcAbilitySourceInterface;

/**
 * 伤害执行计算基类
 * 继承自通用属性执行器，添加伤害特有的初始化逻辑
 * 
 * 设计思想：
 * - 作为协调者，定义执行顺序，不包含具体逻辑
 * - 通过父类 Components 数组配置伤害计算流程
 * - 支持蓝图配置和 C++ 扩展
 * 
 * 功能特性：
 * - 从 GE Context 获取 DamageTypeTag 和 HitZone
 * - 广播伤害事件到 EventSubsystem
 * - 伤害可视化调试
 * 
 * 使用方式：
 * 1. 创建 GE，选择此 Execution
 * 2. 在 Components 数组中添加所需的伤害组件
 * 3. 配置每个组件的参数
 */
UCLASS(Blueprintable)
class YICHENDAMAGE_API UYcDamageExecution : public UYcAttributeExecution
{
	GENERATED_BODY()

public:
	UYcDamageExecution();

protected:
	// -------------------------------------------------------------------
	// ExecutionCalculation 接口重写
	// -------------------------------------------------------------------

	/**
	 * 执行伤害计算
	 * 1. 初始化 Params（添加伤害特有数据）
	 * 2. 按 Priority 遍历 Components（使用父类数组）
	 * 3. 计算最终伤害
	 * 4. 广播伤害事件
	 * 5. 输出调试信息和可视化
	 */
	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

protected:
	// -------------------------------------------------------------------
	// 内部函数
	// -------------------------------------------------------------------

	/**
	 * 初始化伤害参数
	 * - 从 GE Context 获取 DamageTypeTag
	 * - 从 PhysMaterial 派生 HitZone
	 */
	virtual void InitializeParams(FYcAttributeSummaryParams& Params, const FGameplayEffectCustomExecutionParameters& ExecutionParams) const;

	/**
	 * 输出调试信息
	 */
	virtual void LogDebugInfo(const FYcAttributeSummaryParams& Params) const override;

private:
	/**
	 * 广播伤害事件
	 */
	void BroadcastDamageEvent(const FYcDamageSummaryParams& Params, const FGameplayTagContainer& SourceTags) const;

#if !UE_BUILD_SHIPPING
	/**
	 * 绘制伤害可视化
	 */
	void DrawDamageVisualization(const FYcDamageSummaryParams& Params, const FGameplayTagContainer& SourceTags, UWorld* World) const;
#endif
};
