// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/WorldSubsystem.h"
#include "YcDamageEventSubsystem.generated.h"

class UAbilitySystemComponent;

/**
 * 伤害事件数据
 * 包含伤害计算完成后的所有相关信息
 */
USTRUCT(BlueprintType)
struct FYcDamageEventData
{
	GENERATED_BODY()

	/** 伤害来源 Actor */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> Instigator = nullptr;

	/** 伤害目标 Actor */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> Target = nullptr;

	/** 最终伤害值 */
	UPROPERTY(BlueprintReadOnly)
	float FinalDamage = 0.0f;

	/** 基础伤害值 */
	UPROPERTY(BlueprintReadOnly)
	float BaseDamage = 0.0f;

	/** 伤害类型标签 */
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag DamageType;

	/** 事件标签（暴击、闪避等） */
	UPROPERTY(BlueprintReadOnly)
	FGameplayTagContainer EventTags;

	/** 是否被闪避 */
	UPROPERTY(BlueprintReadOnly)
	bool bWasDodged = false;

	/** 是否被免疫 */
	UPROPERTY(BlueprintReadOnly)
	bool bWasImmune = false;

	/** 是否暴击 */
	UPROPERTY(BlueprintReadOnly)
	bool bWasCritical = false;

	/** 命中区域标签 */
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag HitZone;
};

/** 伤害事件委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDamageApplied, const FYcDamageEventData&, DamageEvent, const FGameplayTagContainer&, SourceTags);

/**
 * 伤害事件子系统
 * 全局伤害事件广播中心，供 UI、成就、音效等系统监听
 */
UCLASS()
class YICHENDAMAGE_API UYcDamageEventSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** 伤害应用事件 */
	UPROPERTY(BlueprintAssignable, Category = "Damage Events")
	FOnDamageApplied OnDamageApplied;

	/** 伤害被闪避事件 */
	UPROPERTY(BlueprintAssignable, Category = "Damage Events")
	FOnDamageApplied OnDamageDodged;

	/** 伤害被免疫事件 */
	UPROPERTY(BlueprintAssignable, Category = "Damage Events")
	FOnDamageApplied OnDamageImmune;

	/** 暴击事件 */
	UPROPERTY(BlueprintAssignable, Category = "Damage Events")
	FOnDamageApplied OnDamageCritical;

	/**
	 * 广播伤害事件
	 * @param EventData 伤害事件数据
	 * @param SourceTags 来源标签
	 */
	UFUNCTION(BlueprintCallable, Category = "Damage Events")
	void BroadcastDamageEvent(const FYcDamageEventData& EventData, const FGameplayTagContainer& SourceTags);

	/**
	 * 从伤害参数创建事件数据
	 * @param Params 伤害参数
	 * @param Instigator 来源 Actor
	 * @param Target 目标 Actor
	 */
	static FYcDamageEventData CreateEventDataFromParams(const struct FYcDamageSummaryParams& Params, AActor* Instigator, AActor* Target);
};
