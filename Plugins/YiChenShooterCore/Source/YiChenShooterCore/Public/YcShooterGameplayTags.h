// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace YcShooterGameplayTags
{
	// ================================================================================
	// 武器动作标签 (Asset.Weapon.Action.*)
	// 用于 UYcWeaponVisualData::ExtendedActions
	// ================================================================================
	
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Action_Fire);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Action_Fire_ADS);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Action_Fire_Hip);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Action_Reload);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Action_Reload_Empty);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Action_Reload_Tactical);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Action_Inspect);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Action_Holster);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Action_Holster_Quick);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Action_Unholster);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Action_Unholster_Quick);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Action_Melee);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Action_Melee_Alt);

	// ================================================================================
	// 武器姿态动画标签 (Asset.Weapon.Pose.*)
	// 用于 UYcWeaponVisualData::ExtendedPoseAnims
	// ================================================================================
	
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Pose_Idle);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Pose_Aim);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Pose_Run);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Pose_Sprint);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Pose_Crouch);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Pose_Prone);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Pose_Lowered);

	// ================================================================================
	// 武器特效标签 (Asset.Weapon.VFX.*)
	// 用于 UYcWeaponVisualData::VFXEffects
	// ================================================================================
	
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_VFX_MuzzleFlash);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_VFX_MuzzleFlash_Suppressed);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_VFX_Shell);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_VFX_Tracer);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_VFX_Impact);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_VFX_Impact_Flesh);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_VFX_Impact_Metal);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_VFX_Impact_Concrete);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_VFX_Impact_Wood);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_VFX_Impact_Water);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_VFX_Smoke);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_VFX_Laser);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_VFX_Flashlight);

	// ================================================================================
	// 武器音效标签 (Asset.Weapon.Sound.*)
	// 用于 UYcWeaponVisualData::SoundEffects
	// ================================================================================
	
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Sound_Fire);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Sound_Fire_Suppressed);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Sound_Fire_Tail);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Sound_Reload);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Sound_Reload_Empty);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Sound_DryFire);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Sound_BoltAction);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Sound_Equip);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Sound_Unequip);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Sound_Impact);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Sound_Impact_Flesh);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Sound_Impact_Metal);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Sound_Impact_Concrete);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Sound_Impact_Wood);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Sound_Impact_Water);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Sound_Shell);
	YICHENSHOOTERCORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Asset_Weapon_Sound_Whizby);
}
