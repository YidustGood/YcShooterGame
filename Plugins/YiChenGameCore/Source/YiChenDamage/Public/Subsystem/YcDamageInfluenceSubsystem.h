// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "YiChenAbility/Public/Executions/YcAttributeSummaryParams.h"
#include "Executions/YcDamageSummaryParams.h"
#include "YcDamageInfluenceSubsystem.generated.h"

class UAbilitySystemComponent;
class UDataTable;

/**
 * 属性来源类型
 */
UENUM(BlueprintType)
enum class EYcDamageInfluenceSource : uint8
{
	/** 无来源 */
	None = 0,
	/** 从伤害源获取 */
	FromSource = 1,
	/** 从目标获取 */
	FromTarget = 2,
};

/**
 * 伤害加成数据行
 * 用于数据表配置
 */
USTRUCT(BlueprintType)
struct FYcDamageInfluenceRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 伤害类型标签（用于匹配） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayTag DamageTypeTag;

	/** 加成优先级 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	EYcAttributeInfluencePriority Priority = EYcAttributeInfluencePriority::Coefficient;

	/** 要读取的属性 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	FGameplayAttribute Attribute;

	/** 属性来源 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	EYcDamageInfluenceSource Source = EYcDamageInfluenceSource::FromTarget;

	/** 值乘数（对属性值进行缩放） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float ValueMultiplier = 1.0f;

	/** 值偏移（对属性值进行偏移） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	float ValueOffset = 0.0f;

	/** 是否反转值（用于减伤等） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Damage")
	bool bInvertValue = false;

	/**
	 * 计算最终加成值
	 */
	float CalculateValue(float AttributeValue) const
	{
		float Value = AttributeValue * ValueMultiplier + ValueOffset;
		if (bInvertValue)
		{
			Value = -Value;
		}
		return Value;
	}
};

/**
 * 伤害加成子系统
 * 根据 DamageTypeTag 查询需要读取的属性，并计算加成值
 */
UCLASS()
class YICHENDAMAGE_API UYcDamageInfluenceSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	// -------------------------------------------------------------------
	// USubsystem 接口
	// -------------------------------------------------------------------

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// -------------------------------------------------------------------
	// 配置
	// -------------------------------------------------------------------

	/** 设置加成数据表 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|Influence")
	void SetInfluenceTable(UDataTable* InInfluenceTable);

	/** 获取加成数据表 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|Influence")
	UDataTable* GetInfluenceTable() const { return InfluenceTable; }

	// -------------------------------------------------------------------
	// 查询接口
	// -------------------------------------------------------------------

	/**
	 * 获取指定伤害类型的加成配置
	 * @param DamageTypeTag 伤害类型标签
	 * @return 加成配置数组
	 */
	UFUNCTION(BlueprintCallable, Category = "YcDamage|Influence")
	TArray<FYcDamageInfluenceRow> GetInfluencesForDamageType(const FGameplayTag& DamageTypeTag) const;

	/**
	 * 计算并应用所有加成
	 * @param Params 伤害参数（会被修改）
	 * @param DamageTypeTag 伤害类型标签
	 */
	void ApplyInfluences(FYcDamageSummaryParams& Params, const FGameplayTag& DamageTypeTag) const;

private:
	// -------------------------------------------------------------------
	// 内部数据
	// -------------------------------------------------------------------

	/** 加成数据表 */
	UPROPERTY()
	TObjectPtr<UDataTable> InfluenceTable = nullptr;

	/** 缓存的加成配置映射 */
	TMap<FGameplayTag, TArray<FYcDamageInfluenceRow>> CachedInfluenceMap;

	/** 缓存是否已初始化 */
	bool bCacheInitialized = false;

	// -------------------------------------------------------------------
	// 内部函数
	// -------------------------------------------------------------------

	/** 初始化缓存 */
	void InitializeCache();

	/** 获取属性值 */
	float GetAttributeValue(UAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute, bool& bFound) const;
};
