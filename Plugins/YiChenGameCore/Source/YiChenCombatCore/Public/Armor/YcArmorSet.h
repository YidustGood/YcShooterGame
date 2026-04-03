// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Attributes/YcAttributeSet.h"
#include "YcArmorSet.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_ArmorBroken);

/**
 * 护甲属性集
 * 管理单一护甲部位的耐久度、最大护甲值和护甲吸收比例
 * 护甲在受击时消耗耐久度，提供伤害减免效果
 * 当护甲耐久度归零时，不再提供减伤效果
 * 
 * 多 AttributeSet 架构：
 * - 每个护甲部位对应一个独立的 UYcArmorSet 实例
 * - 通过 AttributeSetTag 标识护甲部位（如 Armor.Head、Armor.Body）
 * - 使用 ASC 的 AddAttributeSet/GetAttributeSetByTag 管理多个护甲
 * 
 * 护甲部位示例：
 * - Armor.Head: 头部护甲
 * - Armor.Body: 身体护甲
 * - Armor.Limbs: 四肢护甲
 */
UCLASS()
class YICHENCOMBATCORE_API UYcArmorSet : public UYcAttributeSet
{
	GENERATED_BODY()
	
public:
	UYcArmorSet();

	ATTRIBUTE_ACCESSORS(UYcArmorSet, Armor);
	ATTRIBUTE_ACCESSORS(UYcArmorSet, MaxArmor);
	ATTRIBUTE_ACCESSORS(UYcArmorSet, ArmorAbsorption);
	
	/** 护甲值变化委托，当护甲耐久度改变时触发 */
	mutable FYcAttributeEvent OnArmorChanged;

	/** 最大护甲值变化委托 */
	mutable FYcAttributeEvent OnMaxArmorChanged;

	/** 护甲归零委托，当护甲耐久度降为0时广播 */
	mutable FYcAttributeEvent OnArmorBroken;

protected:
	/**
	 * 护甲耐久度网络复制回调
	 * @param OldValue 旧值
	 */
	UFUNCTION()
	void OnRep_Armor(const FGameplayAttributeData& OldValue);

	/**
	 * 最大护甲值网络复制回调
	 * @param OldValue 旧值
	 */
	UFUNCTION()
	void OnRep_MaxArmor(const FGameplayAttributeData& OldValue);

	/**
	 * 属性基础值变化前的预处理
	 * @param Attribute 属性
	 * @param NewValue 新值（可修改）
	 */
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	
	/**
	 * 属性当前值变化前的预处理
	 * @param Attribute 属性
	 * @param NewValue 新值（可修改）
	 */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	/**
	 * 属性变化后的后处理
	 * 确保护甲值不超过最大护甲值
	 * @param Attribute 属性
	 * @param OldValue 旧值
	 * @param NewValue 新值
	 */
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	/**
	 * 限制属性值在有效范围内
	 * @param Attribute 属性
	 * @param NewValue 新值（会被修改）
	 */
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;

private:
	/** 当前护甲耐久度，会被最大护甲值限制 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Armor, Category = "YcGameCore|Armor", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData Armor;

	/** 最大护甲耐久度，可以通过游戏效果修改 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxArmor, Category = "YcGameCore|Armor", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxArmor;

	/** 护甲伤害吸收比例（0.0~1.0），例如0.3表示吸收30%伤害 */
	UPROPERTY(BlueprintReadOnly, Category = "YcGameCore|Armor", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData ArmorAbsorption;

	/** 用于跟踪护甲是否已归零 */
	bool bOutOfArmor = true;

	/** 属性变化前存储的护甲值，用于计算变化量 */
	float MaxArmorBeforeAttributeChange = 0.0f;
	float ArmorBeforeAttributeChange = 0.0f;
};
