// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Armor/YcArmorGameplayTags.h"

namespace YcArmorGameplayTags
{
	// ================================================================================
	// 护甲耐久度标签
	// ================================================================================

	UE_DEFINE_GAMEPLAY_TAG(Armor_Durability, "Armor.Durability");

	// ================================================================================
	// 护甲槽位标签
	// ================================================================================

	UE_DEFINE_GAMEPLAY_TAG(Armor_Slot_Head, "Armor.Slot.Head");
	UE_DEFINE_GAMEPLAY_TAG(Armor_Slot_Chest, "Armor.Slot.Chest");
	UE_DEFINE_GAMEPLAY_TAG(Armor_Slot_Legs, "Armor.Slot.Legs");
	UE_DEFINE_GAMEPLAY_TAG(Armor_Slot_Feet, "Armor.Slot.Feet");

	// ================================================================================
	// 护甲受击部位标签
	// ================================================================================

	UE_DEFINE_GAMEPLAY_TAG(Armor_HitZone_Head, "Armor.HitZone.Head");
	UE_DEFINE_GAMEPLAY_TAG(Armor_HitZone_Torso, "Armor.HitZone.Torso");
	UE_DEFINE_GAMEPLAY_TAG(Armor_HitZone_Arms, "Armor.HitZone.Arms");
	UE_DEFINE_GAMEPLAY_TAG(Armor_HitZone_Legs, "Armor.HitZone.Legs");
}
