// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "YcDamageTypeSettings.generated.h"

/**
 * 伤害类型定义
 * 定义一种伤害类型的属性和抗性关系
 */
USTRUCT(BlueprintType)
struct FYcDamageTypeDefinition
{
	GENERATED_BODY()

	/** 伤害类型标签 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag DamageTypeTag;

	/** 显示名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	/** 抗性属性标签（如 Resistance.Fire） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag ResistanceAttributeTag;

	/** 基础抗性系数（0=完全抵抗，1=正常伤害，>1=易伤） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float BaseResistanceCoefficient = 1.0f;
};

/**
 * 伤害类型配置资产
 * 集中管理所有伤害类型的定义和抗性关系
 */
UCLASS(BlueprintType, Const)
class YICHENDAMAGE_API UYcDamageTypeSettings : public UDataAsset
{
	GENERATED_BODY()

public:
	/** 所有伤害类型定义 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Types")
	TArray<FYcDamageTypeDefinition> DamageTypes;

	/** 默认伤害类型标签 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Types")
	FGameplayTag DefaultDamageType;

	/**
	 * 查找伤害类型定义
	 * @param DamageTypeTag 伤害类型标签
	 * @return 伤害类型定义指针，未找到返回 nullptr
	 */
	const FYcDamageTypeDefinition* FindDamageType(const FGameplayTag& DamageTypeTag) const;

	/**
	 * 获取抗性系数
	 * @param DamageTypeTag 伤害类型标签
	 * @return 抗性系数，未找到返回 1.0
	 */
	float GetResistanceCoefficient(const FGameplayTag& DamageTypeTag) const;
};
