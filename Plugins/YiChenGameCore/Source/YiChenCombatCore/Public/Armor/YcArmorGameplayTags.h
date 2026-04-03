// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace YcArmorGameplayTags
{
	// ================================================================================
	// 护甲耐久度标签 (Armor.Durability)
	// 用于 FloatTagsStack 存储护甲物品实例的当前耐久度
	// ================================================================================

	YICHENCOMBATCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Armor_Durability);

	// ================================================================================
	// 护甲槽位标签 (Armor.Slot.*)
	// 用于标识护甲部位，在 ASC 中查找对应的 UYcArmorSet
	// ================================================================================

	YICHENCOMBATCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Armor_Slot_Head);
	YICHENCOMBATCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Armor_Slot_Chest);
	YICHENCOMBATCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Armor_Slot_Legs);
	YICHENCOMBATCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Armor_Slot_Feet);

	// ================================================================================
	// 护甲受击部位标签 (Armor.HitZone.*)
	// 用于命中检测，关联到护甲槽位
	// ================================================================================

	YICHENCOMBATCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Armor_HitZone_Head);
	YICHENCOMBATCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Armor_HitZone_Torso);
	YICHENCOMBATCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Armor_HitZone_Arms);
	YICHENCOMBATCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Armor_HitZone_Legs);
}
