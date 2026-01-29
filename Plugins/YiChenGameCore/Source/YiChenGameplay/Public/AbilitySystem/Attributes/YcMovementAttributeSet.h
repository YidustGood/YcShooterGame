// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once


#include "AbilitySystemComponent.h"
#include "Attributes/YcAttributeSet.h"
#include "YcMovementAttributeSet.generated.h"

/**
 * 移动属性集
 * 
 * 管理角色移动相关的属性，支持通过GameplayEffect修改移动速度
 * 
 * 属性说明：
 * - MaxWalkSpeed: 最大行走速度（基础值）
 * - MaxWalkSpeedMultiplier: 移动速度倍率（用于叠加多个速度修改效果）
 * 
 * 使用场景：
 * - ADS瞄准时降低移速
 * - 冲刺技能提升移速
 * - Buff/Debuff效果修改移速
 * - 装备/道具影响移速
 * 
 * 设计说明：
 * - 使用倍率属性支持多个GE的正确叠加
 * - 最终速度 = MaxWalkSpeed * MaxWalkSpeedMultiplier
 * - 倍率默认为1.0，通过Additive或Multiplicative修改器叠加
 */
UCLASS(BlueprintType)
class YICHENGAMEPLAY_API UYcMovementAttributeSet : public UYcAttributeSet
{
	GENERATED_BODY()

public:
	UYcMovementAttributeSet();

	// AttributeSet重写
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	// 最大行走速度（基础值）
	UPROPERTY(BlueprintReadOnly, Category = "Movement", ReplicatedUsing = OnRep_MaxWalkSpeed)
	FGameplayAttributeData MaxWalkSpeed;
	ATTRIBUTE_ACCESSORS(UYcMovementAttributeSet, MaxWalkSpeed)

	// 移动速度倍率（用于叠加多个效果）
	UPROPERTY(BlueprintReadOnly, Category = "Movement", ReplicatedUsing = OnRep_MaxWalkSpeedMultiplier)
	FGameplayAttributeData MaxWalkSpeedMultiplier;
	ATTRIBUTE_ACCESSORS(UYcMovementAttributeSet, MaxWalkSpeedMultiplier)

protected:
	UFUNCTION()
	virtual void OnRep_MaxWalkSpeed(const FGameplayAttributeData& OldValue);

	UFUNCTION()
	virtual void OnRep_MaxWalkSpeedMultiplier(const FGameplayAttributeData& OldValue);

	// 网络复制
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	// 限制属性值在合理范围内
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;
};
