// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Executions/YcAttributeExecutionComponent.h"
#include "YcDamageSummaryParams.h"
#include "YcDamageExecutionComponent.generated.h"

/**
 * 伤害执行组件基类
 * 所有伤害计算组件的抽象基类，提供统一的接口和过滤机制
 * 
 * 设计思想：
 * - 继承自通用属性执行组件基类
 * - 提供伤害特有的辅助函数
 * - 子类重写 Execute_Implementation 实现具体伤害逻辑
 * 
 * 使用方式：
 * 1. 继承此类创建自定义伤害组件
 * 2. 重写 Execute_Implementation 实现具体逻辑
 * 3. 使用 DamageTypeTags 设置伤害类型过滤条件
 * 4. 使用 Priority 控制执行顺序
 */
UCLASS(BlueprintType, Blueprintable, DefaultToInstanced, EditInlineNew, Abstract)
class YICHENDAMAGE_API UYcDamageExecutionComponent : public UYcAttributeExecutionComponent
{
	GENERATED_BODY()

public:
	UYcDamageExecutionComponent();

	// -------------------------------------------------------------------
	// 伤害类型过滤
	// -------------------------------------------------------------------

	/**
	 * 伤害类型标签过滤
	 * 匹配任意一个即执行
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Filter")
	FGameplayTagContainer DamageTypeTags;

	// -------------------------------------------------------------------
	// 执行接口重写
	// -------------------------------------------------------------------

	/**
	 * 检查组件是否应该执行
	 * 增加伤害类型标签过滤
	 */
	virtual bool ShouldExecute(const FYcAttributeSummaryParams& Params, const FGameplayTagContainer& InSourceTags, const FGameplayTagContainer& InTargetTags) const override;

protected:
	// -------------------------------------------------------------------
	// 内部辅助函数（供子类使用）
	// -------------------------------------------------------------------

	/**
	 * 获取伤害参数（类型转换辅助）
	 * @param Params 通用参数引用
	 * @return 伤害参数引用
	 */
	UFUNCTION(BlueprintCallable)
	static FYcDamageSummaryParams& GetDamageParams(FYcAttributeSummaryParams& Params)
	{
		return static_cast<FYcDamageSummaryParams&>(Params);
	}

	/**
	 * 获取伤害参数（const 版本）
	 */
	static const FYcDamageSummaryParams& GetDamageParams(const FYcAttributeSummaryParams& Params)
	{
		return static_cast<const FYcDamageSummaryParams&>(Params);
	}

	/**
	 * 检查伤害类型是否匹配
	 * @param DamageTypeTag 要检查的伤害类型标签
	 * @return 是否匹配
	 */
	UFUNCTION(BlueprintCallable)
	bool IsDamageTypeMatched(const FGameplayTag& DamageTypeTag) const
	{
		if (DamageTypeTags.IsEmpty())
		{
			return true;
		}
		return DamageTypeTags.HasTag(DamageTypeTag);
	}

	/**
	 * 应用伤害到目标属性
	 * @param Params 参数结构体
	 * @param Attribute 要修改的属性
	 * @param Damage 伤害值
	 */
	void ApplyDamageToAttribute(FYcAttributeSummaryParams& Params, const FGameplayAttribute& Attribute, float Damage) const;
};
