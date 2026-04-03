// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Executions/YcAttributeSummaryParams.h"
#include "YcDamageSummaryParams.generated.h"

/**
 * 伤害信息
 * 记录最终伤害在各属性上的分布
 */
USTRUCT(BlueprintType)
struct FYcDamageInfo
{
	GENERATED_BODY()

	/** 各属性扣除量映射 (Attribute -> Damage) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FGameplayAttribute, float> DamageMap;

	/** 预估伤害值（应用加成后） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float EstimatedDamage = 0.0f;

	/** 实际伤害值（应用护甲等后） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PracticalDamage = 0.0f;
};

/**
 * 伤害计算参数
 * 继承自通用属性计算参数，添加伤害特有的数据
 * 
 * 扩展字段：
 * - DamageTypeTag: 伤害类型标签（物理、火焰、冰冻等）
 * - HitZone: 命中部位标签（头部、身体等）
 * - DamageInfo: 伤害分布信息
 */
USTRUCT(BlueprintType)
struct YICHENDAMAGE_API FYcDamageSummaryParams : public FYcAttributeSummaryParams
{
	GENERATED_BODY()

	// -------------------------------------------------------------------
	// 伤害特有数据
	// -------------------------------------------------------------------

	/** 伤害类型标签 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag DamageTypeTag;

	/** 命中部位标签（如 HitZone.Head, HitZone.Body） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag HitZone;

	/** 伤害信息 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FYcDamageInfo DamageInfo;

	// -------------------------------------------------------------------
	// 向后兼容的访问器
	// -------------------------------------------------------------------

	/** 基础伤害值（访问基类的 BaseValue） */
	float GetBaseDamage() const { return BaseValue; }
	void SetBaseDamage(float Value) { BaseValue = Value; }

	/** 计算最终伤害值（基类 GetFinalValue 的别名） */
	float GetFinalDamage() const
	{
		return GetFinalValue();
	}

	/** 重置计算参数 */
	void Reset()
	{
		FYcAttributeSummaryParams::Reset();
		DamageTypeTag = FGameplayTag();
		HitZone = FGameplayTag();
		DamageInfo = FYcDamageInfo();
	}

	/** 添加加成记录 */
	void AddInfluence(EYcAttributeInfluencePriority Priority, float Value, const FGameplayTag& SourceTag = FGameplayTag())
	{
		FYcAttributeSummaryParams::AddInfluence(Priority, Value, SourceTag);
	}
};
