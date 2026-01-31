// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

/**
 * 武器属性GameplayTag定义
 * 
 * 这些Tag用于配件系统中标识武器属性
 * 配件的StatModifier通过这些Tag指定要修改的属性
 */
namespace YcWeaponStatTags
{
	// ════════════════════════════════════════════════════════════════════════
	// 射击参数
	// ════════════════════════════════════════════════════════════════════════
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_FireRate);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Range_Max);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_BulletRadius);

	// ════════════════════════════════════════════════════════════════════════
	// 伤害参数
	// ════════════════════════════════════════════════════════════════════════
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Damage_Base);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Damage_HeadshotMultiplier);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Damage_ArmorPenetration);

	// ════════════════════════════════════════════════════════════════════════
	// 弹药参数
	// ════════════════════════════════════════════════════════════════════════
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Magazine_Size);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Magazine_ReserveMax);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Reload_Time);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Reload_TacticalTime);

	// ════════════════════════════════════════════════════════════════════════
	// 扩散参数
	// ════════════════════════════════════════════════════════════════════════
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Spread_HipFire_Base);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Spread_HipFire_Max);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Spread_HipFire_PerShot);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Spread_ADS_Base);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Spread_ADS_Max);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Spread_ADS_PerShot);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Spread_Recovery);

	// ════════════════════════════════════════════════════════════════════════
	// 后坐力参数
	// ════════════════════════════════════════════════════════════════════════
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Recoil_Vertical_Min);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Recoil_Vertical_Max);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Recoil_Horizontal_Min);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Recoil_Horizontal_Max);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Recoil_ADSMultiplier);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Recoil_HipFireMultiplier);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_Recoil_Recovery);

	// ════════════════════════════════════════════════════════════════════════
	// 瞄准参数
	// ════════════════════════════════════════════════════════════════════════
	
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_ADS_Time);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_ADS_FOVMultiplier);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Weapon_Stat_ADS_MoveSpeed);
}

/**
 * 配件槽位GameplayTag定义
 */
namespace YcAttachmentSlotTags
{
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_Slot_Muzzle);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_Slot_Barrel);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_Slot_Optic);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_Slot_Stock);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_Slot_Underbarrel);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_Slot_Magazine);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_Slot_RearGrip);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_Slot_Laser);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_Slot_Perk);
	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Attachment_Slot_Ammunition);
}
