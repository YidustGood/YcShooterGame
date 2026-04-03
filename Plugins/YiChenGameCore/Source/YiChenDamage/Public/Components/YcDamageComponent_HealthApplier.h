// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Executions/YcDamageExecutionComponent.h"
#include "YcDamageComponent_HealthApplier.generated.h"

/**
 * 生命值应用组件（解耦版）
 * 将计算后的伤害应用到目标的属性
 * 
 * 设计原理：
 * - 使用 FGameplayAttribute 解耦，不依赖具体 AttributeSet 类型
 * - 推荐配置 Damage 属性，让 AttributeSet 的 PostGameplayEffectExecute 处理实际逻辑
 * - 支持任意 GAS 项目的属性系统
 * 
 * 执行优先级: 200 (最后执行)
 * 
 * 配置示例：
 * - DamageAttribute: 选择 YcHealthSet 的 Damage 属性
 * - HealthSet 会自动响应 Damage 变化，扣除 Health 并触发事件
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, meta = (DisplayName = "Health Applier"))
class YICHENDAMAGE_API UYcDamageComponent_HealthApplier : public UYcDamageExecutionComponent
{
	GENERATED_BODY()

public:
	UYcDamageComponent_HealthApplier();

protected:
	// -------------------------------------------------------------------
	// 属性配置（解耦设计）
	// -------------------------------------------------------------------

	/**
	 * 伤害属性
	 * 伤害值写入此属性，由目标 AttributeSet 处理实际扣除逻辑
	 * 
	 * 推荐配置：
	 * - YcHealthSet 的 Damage 属性
	 * - HealthSet 的 PostGameplayEffectExecute 会自动扣除 Health
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (GameplayAttributeFilter = "true"))
	FGameplayAttribute DamageAttribute;

	/** 
	 * 生命值属性（可选，用于读取当前生命值）
	 * 仅用于检查是否会归零，不直接修改
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (GameplayAttributeFilter = "true"))
	FGameplayAttribute HealthAttribute;

	// -------------------------------------------------------------------
	// 伤害限制
	// -------------------------------------------------------------------

	/** 最小伤害值（伤害不会低于此值） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float MinDamage = 0.0f;

	/** 最大伤害值（伤害不会高于此值） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float MaxDamage = FLT_MAX;

	// -------------------------------------------------------------------
	// 事件配置
	// -------------------------------------------------------------------

	/** 是否在生命值归零时添加临时标签 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	bool bAddOutOfHealthTag = true;

	/** 生命值归零标签 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config", meta = (EditCondition = "bAddOutOfHealthTag"))
	FGameplayTag OutOfHealthTag;

	// -------------------------------------------------------------------
	// 执行接口
	// -------------------------------------------------------------------

	virtual void Execute_Implementation(FYcAttributeSummaryParams& Params) override;

private:
	/** 检查目标是否会归零 */
	bool WillTargetDie(FYcAttributeSummaryParams& Params, float FinalDamage) const;
};
