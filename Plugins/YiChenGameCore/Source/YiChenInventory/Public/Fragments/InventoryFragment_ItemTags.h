// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "YcInventoryItemDefinition.h"
#include "InventoryFragment_ItemTags.generated.h"

/**
 * 物品标签功能片段
 * 
 * 为物品实例配置 GameplayTag，用于物品分类和匹配。
 * 在物品实例创建时，会将配置的标签添加到 ItemInstance 的 OwnedTags 中。
 * 
 * 使用场景：
 * - 弹药类型标记（如 Ammo.Type.9mm, Ammo.Type.556）
 * - 物品类型标记（如 Item.Type.Weapon, Item.Type.Consumable）
 * - 装备槽位标记（如 Equipment.Slot.Primary, Equipment.Slot.Secondary）
 * 
 * 示例：
 * - 9mm 子弹箱：配置 OwnedTags = { Ammo.Type.9mm, Item.Consumable }
 * - 步枪武器：配置 OwnedTags = { Ammo.Type.556, Item.Type.Weapon, Equipment.Slot.Primary }
 * 
 * 这样武器拾取弹药时可以通过 HasMatchingGameplayTag() 检查弹药类型是否匹配。
 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FInventoryFragment_ItemTags : public FYcInventoryItemFragment
{
	GENERATED_BODY()
	
	/** 
	 * 物品拥有的标签
	 * 这些标签会在物品实例创建时添加到 ItemInstance 的 OwnedTags 中
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Tags")
	FGameplayTagContainer OwnedTags;
	
	/** 
	 * 物品实例创建时的回调
	 * 将配置的标签添加到实例的 OwnedTags 中
	 */
	virtual void OnInstanceCreated(UYcInventoryItemInstance* Instance) const override;
};
