// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "YcAttributeSummaryParams.generated.h"

class UAbilitySystemComponent;
struct FGameplayEffectCustomExecutionParameters;
struct FGameplayEffectCustomExecutionOutput;

/**
 * 属性加成优先级
 * 定义不同类型的加成在计算公式中的位置
 */
UENUM(BlueprintType)
enum class EYcAttributeInfluencePriority : uint8
{
	/** 覆盖模式 - 直接覆盖基础值 */
	Override = 0,
	/** 乘前加成 - 在系数乘区之前加减 */
	PreMultiplyAdditive = 1,
	/** 系数乘区 - 乘法计算 */
	Coefficient = 2,
	/** 乘后加成 - 在系数乘区之后加减 */
	PostMultiplyAdditive = 3,
};

/**
 * 属性加成数据
 * 记录单个加成的来源和数值
 */
USTRUCT(BlueprintType)
struct FYcAttributeInfluence
{
	GENERATED_BODY()

	/** 加成优先级 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EYcAttributeInfluencePriority Priority = EYcAttributeInfluencePriority::Coefficient;

	/** 加成数值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Value = 0.0f;

	/** 加成来源标签（用于调试） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag SourceTag;
};

/**
 * 通用属性计算参数基类
 * 封装属性计算相关的中间数据，支持多乘区公式
 * 
 * 设计思想：
 * - 作为属性修改执行器的参数容器
 * - 支持链式计算：基础值 -> 乘前加成 -> 系数乘区 -> 乘后加成
 * - 子类可扩展添加特定领域的数据（如伤害类型、治疗类型等）
 * 
 * 计算公式：
 * 最终值 = (基础值 + 乘前加成) × 系数乘区 + 乘后加成
 * 如果启用覆盖，则：最终值 = 覆盖值 + 乘前加成) × 系数乘区 + 乘后加成
 */
USTRUCT(BlueprintType)
struct YICHENABILITY_API FYcAttributeSummaryParams
{
	GENERATED_BODY()

	// -------------------------------------------------------------------
	// 基础数据
	// -------------------------------------------------------------------

	/** 基础值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseValue = 0.0f;

	/** SetByCaller 数据映射 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TMap<FGameplayTag, float> SetByCallerMagnitudes;

	// -------------------------------------------------------------------
	// 计算乘区
	// -------------------------------------------------------------------

	/** 是否覆盖基础值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bOverride = false;

	/** 覆盖值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OverrideValue = 0.0f;

	/** 乘前加成区 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PreMultiplyAdditive = 0.0f;

	/** 系数乘区 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float Coefficient = 1.0f;

	/** 乘后加成区 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float PostMultiplyAdditive = 0.0f;

	// -------------------------------------------------------------------
	// 结果数据
	// -------------------------------------------------------------------

	/** 临时标签（用于标记计算过程中的状态） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTagContainer TemporaryTags;

	/** 加成记录（用于调试） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FYcAttributeInfluence> InfluenceRecords;

	// -------------------------------------------------------------------
	// 控制标志
	// -------------------------------------------------------------------

	/** 是否取消后续执行 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bCancelExecution = false;

	/** 是否触及下限 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bClamped = false;

	// -------------------------------------------------------------------
	// 执行上下文（非 UPROPERTY，仅运行时使用）
	// -------------------------------------------------------------------

	/** 执行参数指针 */
	const FGameplayEffectCustomExecutionParameters* ExecParams = nullptr;

	/** 执行输出指针 */
	FGameplayEffectCustomExecutionOutput* ExecOutput = nullptr;

	/** 源 ASC */
	UAbilitySystemComponent* SourceASC = nullptr;

	/** 目标 ASC */
	UAbilitySystemComponent* TargetASC = nullptr;

	// -------------------------------------------------------------------
	// 辅助函数
	// -------------------------------------------------------------------

	/**
	 * 计算最终值
	 * 公式: (基础值 + 乘前加成) × 系数乘区 + 乘后加成
	 */
	float GetFinalValue() const
	{
		const float EffectiveBase = bOverride ? OverrideValue : BaseValue;
		return (EffectiveBase + PreMultiplyAdditive) * Coefficient + PostMultiplyAdditive;
	}

	/** 重置计算参数 */
	void Reset()
	{
		BaseValue = 0.0f;
		SetByCallerMagnitudes.Empty();
		
		bOverride = false;
		OverrideValue = 0.0f;
		PreMultiplyAdditive = 0.0f;
		Coefficient = 1.0f;
		PostMultiplyAdditive = 0.0f;
		
		TemporaryTags.Reset();
		InfluenceRecords.Empty();
		
		bCancelExecution = false;
		bClamped = false;
		
		ExecParams = nullptr;
		ExecOutput = nullptr;
		SourceASC = nullptr;
		TargetASC = nullptr;
	}

	/** 添加加成记录 */
	void AddInfluence(EYcAttributeInfluencePriority Priority, float Value, const FGameplayTag& SourceTag = FGameplayTag())
	{
		FYcAttributeInfluence Influence;
		Influence.Priority = Priority;
		Influence.Value = Value;
		Influence.SourceTag = SourceTag;
		InfluenceRecords.Add(Influence);
	}
};
