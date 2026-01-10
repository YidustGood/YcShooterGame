// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "Attributes/YcAttributeSet.h"
#include "YcCombatSet.generated.h"

/**
 * 战斗属性集
 * 定义应用伤害或治疗所需的基础属性
 * 属性示例包括：基础伤害、基础治疗等
 */
UCLASS()
class YICHENSHOOTERCORE_API UYcCombatSet : public UYcAttributeSet
{
	GENERATED_BODY()
public:

	UYcCombatSet();

	ATTRIBUTE_ACCESSORS(UYcCombatSet, BaseDamage);
	ATTRIBUTE_ACCESSORS(UYcCombatSet, BaseHeal);

protected:

	/**
	 * 基础伤害网络复制回调
	 * @param OldValue 旧值
	 */
	UFUNCTION()
	void OnRep_BaseDamage(const FGameplayAttributeData& OldValue);

	/**
	 * 基础治疗网络复制回调
	 * @param OldValue 旧值
	 */
	UFUNCTION()
	void OnRep_BaseHeal(const FGameplayAttributeData& OldValue);

private:

	/** 在伤害执行中应用的基础伤害量 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseDamage, Category = "YcGameCore|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData BaseDamage;

	/** 在治疗执行中应用的基础治疗量 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_BaseHeal, Category = "YcGameCore|Combat", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData BaseHeal;
};