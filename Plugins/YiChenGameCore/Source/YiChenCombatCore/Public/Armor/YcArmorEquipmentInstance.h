// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcEquipmentInstance.h"
#include "YcArmorEquipmentInstance.generated.h"

struct FGameplayEffectSpec;
class UAbilitySystemComponent;
class UYcAbilitySystemComponent;
struct FInventoryFragment_ArmorStats;
class UYcArmorSet;

/**
 * UYcArmorEquipmentInstance - 护甲装备实例
 * 
 * 负责护甲装备时的 GAS AttributeSet 创建和卸下时的清理。
 * 护甲的静态数据存储在 ItemDefinition 的 FInventoryFragment_ArmorStats 中，
 * 动态数据（耐久度）存储在 ItemInstance 的 FloatTagsStack 中。
 * 
 * 多 AttributeSet 架构：
 * - 每个护甲装备实例对应一个独立的 UYcArmorSet
 * - 通过 ArmorTag 标识护甲部位（如 Armor.Head、Armor.Body）
 * - 使用 ASC 的 AddAttributeSet/GetAttributeSetByTag 管理多个护甲
 * 
 * 耐久度同步机制：
 * - 装备时从 ItemInstance 读取初始耐久度初始化 AttributeSet
 * - 通过监听 AttributeSet 变化回调实时同步耐久度到 ItemInstance
 * - 卸下时只需移除 AttributeSet，无需额外保存（已实时同步）
 */
UCLASS()
class YICHENCOMBATCORE_API UYcArmorEquipmentInstance : public UYcEquipmentInstance
{
	GENERATED_BODY()

public:
	UYcArmorEquipmentInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~ Begin UYcEquipmentInstance Interface
	virtual void OnEquipmentInstanceCreated(const FYcEquipmentDefinition& Definition) override;
	virtual void OnEquipped() override;
	virtual void OnUnequipped() override;
	//~ End UYcEquipmentInstance Interface

	/** 获取护甲静态数据 Fragment */
	const FInventoryFragment_ArmorStats* GetArmorStatsFragment() const;

	/** 获取当前耐久度（从 ItemInstance 读取） */
	float GetCurrentDurability() const;

	/** 设置当前耐久度（写入 ItemInstance） */
	void SetCurrentDurability(float Value);

	/** 检查护甲是否已损坏 */
	bool IsBroken() const;

	/** 获取护甲标签 */
	FGameplayTag GetArmorTag() const { return ArmorTag; }

protected:
	/** 应用护甲属性到 GAS（创建并初始化 UYcArmorSet） */
	void ApplyArmorAttribute();

	/** 移除护甲属性集（从 ASC 移除 UYcArmorSet） */
	void RemoveArmorAttribute();

	/** 获取拥有者的 YcAbilitySystemComponent */
	UYcAbilitySystemComponent* GetOwnerYcASC() const;

	/** 绑定护甲属性变化监听 */
	void BindArmorChangeDelegate();

	/** 解绑护甲属性变化监听 */
	void UnbindArmorChangeDelegate();

	/** 护甲耐久度变化回调 - 实时写回 ItemInstance */
	void OnArmorAttributeChanged(AActor* EffectInstigator, AActor* EffectCauser, const FGameplayEffectSpec* EffectSpec, float EffectMagnitude, float OldValue, float NewValue);

	/** 广播护甲消息（装备/卸下/变化/破碎） */
	void BroadcastArmorMessage(FGameplayTag MessageTag, float OldValue = 0.0f, float NewValue = 0.0f);

private:
	/** 缓存的护甲静态数据 Fragment */
	const FInventoryFragment_ArmorStats* ArmorStats = nullptr;

	/** 护甲标签，用于标识护甲部位（如 Armor.Head、Armor.Body） */
	FGameplayTag ArmorTag;

	/** 缓存的护甲属性集引用（用于解绑委托） */
	TWeakObjectPtr<const UYcArmorSet> CachedArmorSet;
};
