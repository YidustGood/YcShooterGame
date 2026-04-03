// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"
#include "YcArmorLibrary.generated.h"

class UAbilitySystemComponent;
class UYcArmorSet;

/**
 * 护甲查询函数库
 * 提供蓝图可用的护甲属性查询接口
 * 
 * 支持多 AttributeSet 护甲架构：
 * - 每个护甲部位对应一个独立的 UYcArmorSet
 * - 通过 ArmorTag 区分不同部位（如 Armor.Head、Armor.Body）
 * - 使用 ASC 的 GetAttributeSetByTag 查找特定部位的护甲
 */
UCLASS()
class YICHENCOMBATCORE_API UYcArmorLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 获取 Actor 指定部位的护甲属性集
	 * @param Target 目标 Actor
	 * @param ArmorTag 护甲部位标签（如 Armor.Head）
	 * @return 护甲属性集，无效时返回 nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "YcGameCore|Armor")
	static const UYcArmorSet* GetArmorSetByTag(AActor* Target, FGameplayTag ArmorTag);

	/**
	 * 获取 Actor 所有护甲属性集
	 * @param Target 目标 Actor
	 * @return 护甲属性集数组
	 */
	UFUNCTION(BlueprintPure, Category = "YcGameCore|Armor")
	static TArray<UYcArmorSet*> GetAllArmorSets(AActor* Target);

	/**
	 * 获取 Actor 指定部位的当前护甲值
	 * @param Target 目标 Actor
	 * @param ArmorTag 护甲部位标签
	 * @return 当前护甲耐久度，无效时返回 0
	 */
	UFUNCTION(BlueprintPure, Category = "YcGameCore|Armor")
	static float GetArmorByTag(AActor* Target, FGameplayTag ArmorTag);

	/**
	 * 获取 Actor 指定部位的最大护甲值
	 * @param Target 目标 Actor
	 * @param ArmorTag 护甲部位标签
	 * @return 最大护甲耐久度，无效时返回 0
	 */
	UFUNCTION(BlueprintPure, Category = "YcGameCore|Armor")
	static float GetMaxArmorByTag(AActor* Target, FGameplayTag ArmorTag);

	/**
	 * 获取 Actor 指定部位的归一化护甲值（0.0~1.0）
	 * @param Target 目标 Actor
	 * @param ArmorTag 护甲部位标签
	 * @return 归一化护甲值，无效时返回 0
	 */
	UFUNCTION(BlueprintPure, Category = "YcGameCore|Armor")
	static float GetArmorNormalizedByTag(AActor* Target, FGameplayTag ArmorTag);

	/**
	 * 获取 Actor 指定部位的护甲吸收比例
	 * @param Target 目标 Actor
	 * @param ArmorTag 护甲部位标签
	 * @return 护甲吸收比例（0.0~1.0），无效时返回 0
	 */
	UFUNCTION(BlueprintPure, Category = "YcGameCore|Armor")
	static float GetArmorAbsorptionByTag(AActor* Target, FGameplayTag ArmorTag);

	/**
	 * 检查 Actor 指定部位的护甲是否已破碎
	 * @param Target 目标 Actor
	 * @param ArmorTag 护甲部位标签
	 * @return 护甲耐久度为 0 时返回 true
	 */
	UFUNCTION(BlueprintPure, Category = "YcGameCore|Armor")
	static bool IsArmorBrokenByTag(AActor* Target, FGameplayTag ArmorTag);

	// ========== 兼容旧 API（使用默认护甲标签）==========

	/**
	 * 获取 Actor 的当前护甲值（使用第一个护甲属性集）
	 * @param Target 目标 Actor
	 * @return 当前护甲耐久度，无效时返回 0
	 */
	UFUNCTION(BlueprintPure, Category = "YcGameCore|Armor")
	static float GetArmor(AActor* Target);

	/**
	 * 获取 Actor 的最大护甲值（使用第一个护甲属性集）
	 * @param Target 目标 Actor
	 * @return 最大护甲耐久度，无效时返回 0
	 */
	UFUNCTION(BlueprintPure, Category = "YcGameCore|Armor")
	static float GetMaxArmor(AActor* Target);

	/**
	 * 获取 Actor 的归一化护甲值（0.0~1.0）
	 * @param Target 目标 Actor
	 * @return 归一化护甲值，无效时返回 0
	 */
	UFUNCTION(BlueprintPure, Category = "YcGameCore|Armor")
	static float GetArmorNormalized(AActor* Target);

private:
	/** 获取 Actor 的 ASC */
	static UAbilitySystemComponent* GetASCFromActor(AActor* Target);
};
