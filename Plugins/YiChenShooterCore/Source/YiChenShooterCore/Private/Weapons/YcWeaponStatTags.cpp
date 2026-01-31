// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons//YcWeaponStatTags.h"

namespace YcWeaponStatTags
{
	// 射击参数
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_FireRate, "Weapon.Stat.FireRate");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Range_Max, "Weapon.Stat.Range.Max");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_BulletRadius, "Weapon.Stat.BulletRadius");

	// 伤害参数
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Damage_Base, "Weapon.Stat.Damage.Base");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Damage_HeadshotMultiplier, "Weapon.Stat.Damage.HeadshotMultiplier");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Damage_ArmorPenetration, "Weapon.Stat.Damage.ArmorPenetration");

	// 弹药参数
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Magazine_Size, "Weapon.Stat.Magazine.Size");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Magazine_ReserveMax, "Weapon.Stat.Magazine.ReserveMax");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Reload_Time, "Weapon.Stat.Reload.Time");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Reload_TacticalTime, "Weapon.Stat.Reload.TacticalTime");

	// 扩散参数
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Spread_HipFire_Base, "Weapon.Stat.Spread.HipFire.Base");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Spread_HipFire_Max, "Weapon.Stat.Spread.HipFire.Max");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Spread_HipFire_PerShot, "Weapon.Stat.Spread.HipFire.PerShot");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Spread_ADS_Base, "Weapon.Stat.Spread.ADS.Base");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Spread_ADS_Max, "Weapon.Stat.Spread.ADS.Max");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Spread_ADS_PerShot, "Weapon.Stat.Spread.ADS.PerShot");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Spread_Recovery, "Weapon.Stat.Spread.Recovery");

	// 后坐力参数
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Recoil_Vertical_Min, "Weapon.Stat.Recoil.Vertical.Min");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Recoil_Vertical_Max, "Weapon.Stat.Recoil.Vertical.Max");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Recoil_Horizontal_Min, "Weapon.Stat.Recoil.Horizontal.Min");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Recoil_Horizontal_Max, "Weapon.Stat.Recoil.Horizontal.Max");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Recoil_ADSMultiplier, "Weapon.Stat.Recoil.ADSMultiplier");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Recoil_HipFireMultiplier, "Weapon.Stat.Recoil.HipFireMultiplier");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_Recoil_Recovery, "Weapon.Stat.Recoil.Recovery");

	// 瞄准参数
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_ADS_Time, "Weapon.Stat.ADS.Time");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_ADS_FOVMultiplier, "Weapon.Stat.ADS.FOVMultiplier");
	UE_DEFINE_GAMEPLAY_TAG(Weapon_Stat_ADS_MoveSpeed, "Weapon.Stat.ADS.MoveSpeed");
}

namespace YcAttachmentSlotTags
{
	UE_DEFINE_GAMEPLAY_TAG(Attachment_Slot_Muzzle, "Attachment.Slot.Muzzle");
	UE_DEFINE_GAMEPLAY_TAG(Attachment_Slot_Barrel, "Attachment.Slot.Barrel");
	UE_DEFINE_GAMEPLAY_TAG(Attachment_Slot_Optic, "Attachment.Slot.Optic");
	UE_DEFINE_GAMEPLAY_TAG(Attachment_Slot_Stock, "Attachment.Slot.Stock");
	UE_DEFINE_GAMEPLAY_TAG(Attachment_Slot_Underbarrel, "Attachment.Slot.Underbarrel");
	UE_DEFINE_GAMEPLAY_TAG(Attachment_Slot_Magazine, "Attachment.Slot.Magazine");
	UE_DEFINE_GAMEPLAY_TAG(Attachment_Slot_RearGrip, "Attachment.Slot.RearGrip");
	UE_DEFINE_GAMEPLAY_TAG(Attachment_Slot_Laser, "Attachment.Slot.Laser");
	UE_DEFINE_GAMEPLAY_TAG(Attachment_Slot_Perk, "Attachment.Slot.Perk");
	UE_DEFINE_GAMEPLAY_TAG(Attachment_Slot_Ammunition, "Attachment.Slot.Ammunition");
}
