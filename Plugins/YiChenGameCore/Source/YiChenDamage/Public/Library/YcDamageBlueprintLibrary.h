// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "YiChenAbility/Public/Executions/YcAttributeSummaryParams.h"
#include "YcDamageBlueprintLibrary.generated.h"

class UAbilitySystemComponent;

/**
 * 伤害系统蓝图函数库
 * 提供蓝图可访问的伤害计算辅助函数
 * 
 * 注意：所有乘区操作函数接受基类 FYcAttributeSummaryParams&，
 * 可同时用于通用属性计算和伤害计算。
 */
UCLASS()
class YICHENDAMAGE_API UYcDamageBlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// -------------------------------------------------------------------
	// 属性获取函数
	// -------------------------------------------------------------------

	/**
	 * 从源获取属性值
	 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|Attribute", meta = (WorldContext = "WorldContextObject"))
	static float GetSourceAttribute(const UObject* WorldContextObject, UAbilitySystemComponent* SourceASC, const FGameplayAttribute& Attribute, bool& bFound);

	/**
	 * 从目标获取属性值
	 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|Attribute", meta = (WorldContext = "WorldContextObject"))
	static float GetTargetAttribute(const UObject* WorldContextObject, UAbilitySystemComponent* TargetASC, const FGameplayAttribute& Attribute, bool& bFound);

	// -------------------------------------------------------------------
	// 乘区操作函数（通用，适用于所有属性计算）
	// -------------------------------------------------------------------

	/**
	 * 设置覆盖值
	 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|Influence")
	static void SetOverride(UPARAM(ref) FYcAttributeSummaryParams& Params, float Value);

	/**
	 * 添加乘前加成
	 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|Influence")
	static void AddPreMultiplyAdditive(UPARAM(ref) FYcAttributeSummaryParams& Params, float Value);

	/**
	 * 添加系数乘区
	 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|Influence")
	static void MultiplyCoefficient(UPARAM(ref) FYcAttributeSummaryParams& Params, float Value);

	/**
	 * 添加乘后加成
	 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|Influence")
	static void AddPostMultiplyAdditive(UPARAM(ref) FYcAttributeSummaryParams& Params, float Value);

	// -------------------------------------------------------------------
	// 结果获取函数
	// -------------------------------------------------------------------

	/**
	 * 获取最终值
	 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|Result")
	static float GetFinalValue(const FYcAttributeSummaryParams& Params);

	/**
	 * 获取最终伤害值（兼容旧接口）
	 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|Result", meta = (DeprecatedFunction, DeprecationMessage = "Use GetFinalValue instead"))
	static float GetFinalDamage(const FYcDamageSummaryParams& Params);

	/**
	 * 获取预估伤害值
	 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|Result")
	static float GetEstimatedDamage(const FYcDamageSummaryParams& Params);

	/**
	 * 获取实际伤害值
	 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|Result")
	static float GetPracticalDamage(const FYcDamageSummaryParams& Params);

	// -------------------------------------------------------------------
	// 控制函数（通用，适用于所有属性计算）
	// -------------------------------------------------------------------

	/**
	 * 取消后续执行
	 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|Control")
	static void CancelExecution(UPARAM(ref) FYcAttributeSummaryParams& Params);

	/**
	 * 添加临时标签
	 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|Control")
	static void AddTemporaryTag(UPARAM(ref) FYcAttributeSummaryParams& Params, const FGameplayTag& Tag);

	/**
	 * 检查是否有临时标签
	 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|Control")
	static bool HasTemporaryTag(const FYcAttributeSummaryParams& Params, const FGameplayTag& Tag);

	// -------------------------------------------------------------------
	// SetByCaller 函数
	// -------------------------------------------------------------------

	/**
	 * 获取 SetByCaller 值
	 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|SetByCaller")
	static float GetSetByCallerValue(const FYcAttributeSummaryParams& Params, const FGameplayTag& Tag, bool& bFound);

	/**
	 * 检查是否有 SetByCaller
	 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|SetByCaller")
	static bool HasSetByCaller(const FYcAttributeSummaryParams& Params, const FGameplayTag& Tag);

	// -------------------------------------------------------------------
	// 调试函数
	// -------------------------------------------------------------------

	/**
	 * 获取伤害计算详情字符串
	 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|Debug")
	static FString GetDamageDebugString(const FYcDamageSummaryParams& Params);

	/**
	 * 打印伤害计算详情
	 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|Debug")
	static void PrintDamageDebug(const FYcDamageSummaryParams& Params);
};
