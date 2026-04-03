// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YiChenDamage/Public/Executions/YcDamageExecutionComponent.h"
#include "YiChenDamage/Public/Executions/YcDamageSummaryParams.h"
#include "YcDamageComponent_DamageTypeResistance.generated.h"

class UAttributeSet;
class UYcDamageTypeSettings;
class UYcResistanceComponent;

/**
 * 伤害类型抗性组件
 * 
 * 功能：
 * - 使用 UYcDamageTypeSettings 配置的伤害类型定义
 * - 根据伤害类型查询目标的对应抗性
 * - 应用抗性减免伤害
 * 
 * 抗性查询优先级：
 * 1. UYcResistanceComponent（高效 O(1) 查询）
 * 2. 传统 AttributeSet（回退方案，O(N*M) 遍历）
 * 
 * 使用场景：
 * - 元素伤害系统（火焰、冰冻、雷电等）
 * - 物理护甲系统
 * - 魔法抗性系统
 * 
 * 配置方式：
 * 1. 创建 UYcDamageTypeSettings 数据资产，配置伤害类型定义
 * 2. 在组件中引用该资产
 * 3. 目标 Actor 添加 UYcResistanceComponent（推荐）或传统 AttributeSet
 * 
 * 与 UYcDamageTypeSettings 的关系：
 * - UYcDamageTypeSettings：集中配置所有伤害类型定义（数据层）
 * - 本组件：执行抗性计算逻辑（逻辑层）
 */
UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "Damage Type Resistance"))
class YICHENDAMAGE_API UYcDamageComponent_DamageTypeResistance : public UYcDamageExecutionComponent
{
	GENERATED_BODY()

public:
	UYcDamageComponent_DamageTypeResistance();

	// -------------------------------------------------------------------
	// 配置
	// -------------------------------------------------------------------

	/** 伤害类型配置资产（优先使用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	TObjectPtr<UYcDamageTypeSettings> DamageTypeSettings;

	/** 最大抗性上限（限制所有抗性的最大值，如 0.8 表示最多减免 80%） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float MaxResistance = 0.8f;

	/** 默认抗性（未匹配到类型时使用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config")
	float DefaultResistance = 0.0f;

	// -------------------------------------------------------------------
	// UYcDamageExecutionComponent 接口
	// -------------------------------------------------------------------

	virtual void Execute_Implementation(FYcAttributeSummaryParams& Params) override;

protected:
	/**
	 * 获取目标抗性值（优先使用高效方案）
	 * 
	 * 查询优先级：
	 * 1. UYcResistanceComponent（O(1) 查询）
	 * 2. 传统 AttributeSet（O(N*M) 遍历）
	 * 
	 * @param Params 伤害参数
	 * @param ResistanceTag 抗性标签（如 Resistance.Fire）
	 * @return 抗性值，未找到返回 0
	 */
	float GetTargetResistance(const FYcDamageSummaryParams& Params, const FGameplayTag& ResistanceTag) const;

	/**
	 * 获取目标属性值（通过 AttributeSet 反射，回退方案）
	 * @param Params 伤害参数
	 * @param AttributeTag 属性标签（如 Resistance.Fire）
	 * @return 属性值，未找到返回 0
	 */
	float GetTargetAttributeValueByTag(const FYcDamageSummaryParams& Params, const FGameplayTag& AttributeTag) const;
};
