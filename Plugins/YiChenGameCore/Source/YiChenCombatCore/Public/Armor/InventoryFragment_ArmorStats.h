// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "YcInventoryItemDefinition.h"
#include "InventoryFragment_ArmorStats.generated.h"

/**
 * 护甲静态数据 Fragment
 * 存储护甲的配置属性，附加在 ItemDefinition 上
 * 动态数据（如当前耐久度）存储在 ItemInstance 的 FloatTagsStack 中
 * 
 * 多 AttributeSet 架构：
 * - 每个护甲对应一个独立的 UYcArmorSet 实例
 * - 通过 ArmorTag 标识护甲部位（如 Armor.Head、Armor.Body）
 */
USTRUCT(BlueprintType)
struct YICHENCOMBATCORE_API FInventoryFragment_ArmorStats : public FYcInventoryItemFragment
{
	GENERATED_BODY()

	/** 
	 * 护甲标签，用于标识护甲部位
	 * 用于在 ASC 中查找对应的 UYcArmorSet
	 * 例如：Armor.Head、Armor.Body、Armor.Legs
	 * 插件提供的护甲伤害计算组件会将命中区域Tag和这个Tag做关联查询以实现护甲吸收伤害的功能, 需要ArmorTag和人物可被命中区域Tag一一匹配
	 * 当然也可以自定义伤害计算组件实现
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armor")
	FGameplayTag ArmorTag;

	/** 最大耐久度 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armor")
	float MaxDurability = 100.0f;

	/** 护甲减伤比例 (0.0 ~ 1.0) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armor", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ArmorAbsorption = 0.3f;

	/** 耐久度消耗 GameplayTag，用于在 FloatTagsStack 中存储当前耐久 */
	static FGameplayTag GetDurabilityTag();

	//~ Begin FYcInventoryItemFragment Interface
	virtual void OnInstanceCreated(UYcInventoryItemInstance* Instance) const override;
	//~ End FYcInventoryItemFragment Interface

	/** 获取物品实例的当前耐久度 */
	static float GetCurrentDurability(const UYcInventoryItemInstance* Instance);

	/** 设置物品实例的当前耐久度 */
	static void SetCurrentDurability(UYcInventoryItemInstance* Instance, float Value);

	/** 检查护甲是否已损坏（耐久度 <= 0） */
	static bool IsBroken(const UYcInventoryItemInstance* Instance);
};
