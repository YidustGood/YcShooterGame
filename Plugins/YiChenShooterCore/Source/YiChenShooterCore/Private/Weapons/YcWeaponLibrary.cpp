// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Weapons/YcWeaponLibrary.h"

#include "YcEquipmentInstance.h"
#include "YcInventoryItemInstance.h"
#include "YcInventoryLibrary.h"
#include "YiChenShooterCore.h"
#include "Weapons/YcWeaponVisualData.h"
#include "Weapons/YcHitScanWeaponInstance.h"
#include "Weapons/Fragments/YcEquipmentFragment_ReticleConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcWeaponLibrary)

UYcWeaponVisualData* UYcWeaponLibrary::GetWeaponVisualData(UYcEquipmentInstance* Equipment)
{
	if (!Equipment || !Equipment->GetAssociatedItem() || !Equipment->GetAssociatedItem()->GetItemDef())
	{
		UE_LOG(LogYcShooterCore, Error, TEXT("获取 WeaponVisualData 失败, 关联物品定义无效!"));
		return nullptr;
	}
	
	UPrimaryDataAsset* DataAsset = UYcInventoryLibrary::GetYcDataAssetByTag(*Equipment->GetAssociatedItem()->GetItemDef(), TAG_Yc_Asset_Item_WeaponVisualData);
	
	return Cast<UYcWeaponVisualData>(DataAsset);
}

void UYcWeaponLibrary::BreakWeaponStats(
	const FYcComputedWeaponStats& Stats,
	int32& BulletsPerCartridge,
	float& MaxDamageRange,
	float& BulletTraceSweepRadius,
	float& FireRate,
	float& BaseDamage,
	float& HeadshotMultiplier,
	float& ArmorPenetration,
	int32& MagazineSize,
	int32& MaxReserveAmmo,
	float& ReloadTime,
	float& TacticalReloadTime,
	float& HipFireBaseSpread,
	float& HipFireMaxSpread,
	float& HipFireSpreadPerShot,
	float& SpreadRecoveryRate,
	float& SpreadRecoveryDelay,
	float& SpreadExponent,
	float& ADSBaseSpread,
	float& ADSMaxSpread,
	float& ADSSpreadPerShot,
	float& MovingSpreadMultiplier,
	float& JumpingSpreadMultiplier,
	float& CrouchingSpreadMultiplier,
	float& VerticalRecoilMin,
	float& VerticalRecoilMax,
	float& HorizontalRecoilMin,
	float& HorizontalRecoilMax,
	float& ADSRecoilMultiplier,
	float& HipFireRecoilMultiplier,
	float& CrouchingRecoilMultiplier,
	float& RecoilRecoveryRate,
	float& RecoilRecoveryDelay,
	float& RecoilRecoveryPercent,
	float& AimDownSightTime,
	float& ADSFOVMultiplier,
	float& ADSMoveSpeedMultiplier)
{
	BulletsPerCartridge = Stats.BulletsPerCartridge;
	MaxDamageRange = Stats.MaxDamageRange;
	BulletTraceSweepRadius = Stats.BulletTraceSweepRadius;
	FireRate = Stats.FireRate;
	BaseDamage = Stats.BaseDamage;
	HeadshotMultiplier = Stats.HeadshotMultiplier;
	ArmorPenetration = Stats.ArmorPenetration;
	MagazineSize = Stats.MagazineSize;
	MaxReserveAmmo = Stats.MaxReserveAmmo;
	ReloadTime = Stats.ReloadTime;
	TacticalReloadTime = Stats.TacticalReloadTime;
	HipFireBaseSpread = Stats.HipFireBaseSpread;
	HipFireMaxSpread = Stats.HipFireMaxSpread;
	HipFireSpreadPerShot = Stats.HipFireSpreadPerShot;
	SpreadRecoveryRate = Stats.SpreadRecoveryRate;
	SpreadRecoveryDelay = Stats.SpreadRecoveryDelay;
	SpreadExponent = Stats.SpreadExponent;
	ADSBaseSpread = Stats.ADSBaseSpread;
	ADSMaxSpread = Stats.ADSMaxSpread;
	ADSSpreadPerShot = Stats.ADSSpreadPerShot;
	MovingSpreadMultiplier = Stats.MovingSpreadMultiplier;
	JumpingSpreadMultiplier = Stats.JumpingSpreadMultiplier;
	CrouchingSpreadMultiplier = Stats.CrouchingSpreadMultiplier;
	VerticalRecoilMin = Stats.VerticalRecoilMin;
	VerticalRecoilMax = Stats.VerticalRecoilMax;
	HorizontalRecoilMin = Stats.HorizontalRecoilMin;
	HorizontalRecoilMax = Stats.HorizontalRecoilMax;
	ADSRecoilMultiplier = Stats.ADSRecoilMultiplier;
	HipFireRecoilMultiplier = Stats.HipFireRecoilMultiplier;
	CrouchingRecoilMultiplier = Stats.CrouchingRecoilMultiplier;
	RecoilRecoveryRate = Stats.RecoilRecoveryRate;
	RecoilRecoveryDelay = Stats.RecoilRecoveryDelay;
	RecoilRecoveryPercent = Stats.RecoilRecoveryPercent;
	AimDownSightTime = Stats.AimDownSightTime;
	ADSFOVMultiplier = Stats.ADSFOVMultiplier;
	ADSMoveSpeedMultiplier = Stats.ADSMoveSpeedMultiplier;
}

void UYcWeaponLibrary::BreakWeaponStats_Damage(
	const FYcComputedWeaponStats& Stats,
	float& BaseDamage,
	float& HeadshotMultiplier,
	float& ArmorPenetration,
	float& MaxDamageRange)
{
	BaseDamage = Stats.BaseDamage;
	HeadshotMultiplier = Stats.HeadshotMultiplier;
	ArmorPenetration = Stats.ArmorPenetration;
	MaxDamageRange = Stats.MaxDamageRange;
}

void UYcWeaponLibrary::BreakWeaponStats_Firing(
	const FYcComputedWeaponStats& Stats,
	int32& BulletsPerCartridge,
	float& FireRate,
	float& MaxDamageRange,
	float& BulletTraceSweepRadius)
{
	BulletsPerCartridge = Stats.BulletsPerCartridge;
	FireRate = Stats.FireRate;
	MaxDamageRange = Stats.MaxDamageRange;
	BulletTraceSweepRadius = Stats.BulletTraceSweepRadius;
}

void UYcWeaponLibrary::BreakWeaponStats_Ammo(
	const FYcComputedWeaponStats& Stats,
	int32& MagazineSize,
	int32& MaxReserveAmmo,
	float& ReloadTime,
	float& TacticalReloadTime)
{
	MagazineSize = Stats.MagazineSize;
	MaxReserveAmmo = Stats.MaxReserveAmmo;
	ReloadTime = Stats.ReloadTime;
	TacticalReloadTime = Stats.TacticalReloadTime;
}

void UYcWeaponLibrary::BreakWeaponStats_Spread(
	const FYcComputedWeaponStats& Stats,
	float& HipFireBaseSpread,
	float& HipFireMaxSpread,
	float& HipFireSpreadPerShot,
	float& ADSBaseSpread,
	float& ADSMaxSpread,
	float& ADSSpreadPerShot,
	float& SpreadRecoveryRate,
	float& SpreadRecoveryDelay,
	float& SpreadExponent,
	float& MovingSpreadMultiplier,
	float& JumpingSpreadMultiplier,
	float& CrouchingSpreadMultiplier)
{
	HipFireBaseSpread = Stats.HipFireBaseSpread;
	HipFireMaxSpread = Stats.HipFireMaxSpread;
	HipFireSpreadPerShot = Stats.HipFireSpreadPerShot;
	ADSBaseSpread = Stats.ADSBaseSpread;
	ADSMaxSpread = Stats.ADSMaxSpread;
	ADSSpreadPerShot = Stats.ADSSpreadPerShot;
	SpreadRecoveryRate = Stats.SpreadRecoveryRate;
	SpreadRecoveryDelay = Stats.SpreadRecoveryDelay;
	SpreadExponent = Stats.SpreadExponent;
	MovingSpreadMultiplier = Stats.MovingSpreadMultiplier;
	JumpingSpreadMultiplier = Stats.JumpingSpreadMultiplier;
	CrouchingSpreadMultiplier = Stats.CrouchingSpreadMultiplier;
}

void UYcWeaponLibrary::BreakWeaponStats_Recoil(
	const FYcComputedWeaponStats& Stats,
	float& VerticalRecoilMin,
	float& VerticalRecoilMax,
	float& HorizontalRecoilMin,
	float& HorizontalRecoilMax,
	float& ADSRecoilMultiplier,
	float& HipFireRecoilMultiplier,
	float& CrouchingRecoilMultiplier,
	float& RecoilRecoveryRate,
	float& RecoilRecoveryDelay,
	float& RecoilRecoveryPercent)
{
	VerticalRecoilMin = Stats.VerticalRecoilMin;
	VerticalRecoilMax = Stats.VerticalRecoilMax;
	HorizontalRecoilMin = Stats.HorizontalRecoilMin;
	HorizontalRecoilMax = Stats.HorizontalRecoilMax;
	ADSRecoilMultiplier = Stats.ADSRecoilMultiplier;
	HipFireRecoilMultiplier = Stats.HipFireRecoilMultiplier;
	CrouchingRecoilMultiplier = Stats.CrouchingRecoilMultiplier;
	RecoilRecoveryRate = Stats.RecoilRecoveryRate;
	RecoilRecoveryDelay = Stats.RecoilRecoveryDelay;
	RecoilRecoveryPercent = Stats.RecoilRecoveryPercent;
}

bool UYcWeaponLibrary::GetWeaponReticleConfig(UYcEquipmentInstance* Equipment,
	FYcEquipmentFragment_ReticleConfig& ReticleConfig)
{
	if (!Equipment)
	{
		return false;
	}

	if (const FYcEquipmentFragment_ReticleConfig* Fragment = Equipment->GetTypedFragment<FYcEquipmentFragment_ReticleConfig>())
	{
		ReticleConfig = *Fragment;
		return true;
	}

	return false;
}
