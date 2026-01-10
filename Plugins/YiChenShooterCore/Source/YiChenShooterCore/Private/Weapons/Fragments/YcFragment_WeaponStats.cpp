// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/Fragments/YcFragment_WeaponStats.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcFragment_WeaponStats)

FYcFragment_WeaponStats::FYcFragment_WeaponStats()
	// 基础射击参数
	: BulletsPerCartridge(1)
	, MaxDamageRange(25000.0f)  // 250米
	, BulletTraceSweepRadius(0.0f)
	, FireRate(600.0f)  // 每分钟600发
	, FireMode(EYcFireMode::Auto)
	, BurstCount(3)
	, BurstIntervalMultiplier(2.0f)
	// 伤害参数
	, BaseDamage(25.0f)
	, HeadshotMultiplier(2.0f)
	, ArmorPenetration(0.0f)
	// 弹药参数
	, MagazineSize(30)
	, MaxReserveAmmo(120)
	, ReloadTime(2.0f)
	, TacticalReloadTime(1.5f)
	// 腰射扩散参数
	, HipFireBaseSpread(2.0f)
	, HipFireMaxSpread(6.0f)
	, HipFireSpreadPerShot(0.5f)
	, SpreadRecoveryRate(8.0f)
	, SpreadRecoveryDelay(0.1f)
	, SpreadExponent(1.0f)
	// 瞄准扩散参数 (COD风格：瞄准时无扩散)
	, ADSBaseSpread(0.0f)
	, ADSMaxSpread(0.0f)
	, ADSSpreadPerShot(0.0f)
	// 状态扩散乘数
	, MovingSpreadMultiplier(1.5f)
	, JumpingSpreadMultiplier(3.0f)
	, CrouchingSpreadMultiplier(0.7f)
	// 后坐力参数
	, bUseRecoilPattern(false)
	, RecoilPatternLoopStart(0)
	, VerticalRecoilMin(0.3f)
	, VerticalRecoilMax(0.5f)
	, HorizontalRecoilMin(-0.2f)
	, HorizontalRecoilMax(0.2f)
	// 后坐力乘数
	, ADSRecoilMultiplier(1.0f)
	, HipFireRecoilMultiplier(0.6f)
	, CrouchingRecoilMultiplier(0.8f)
	// 后坐力恢复
	, RecoilRecoveryRate(10.0f)
	, RecoilRecoveryDelay(0.15f)
	, RecoilRecoveryPercent(0.5f)  // COD风格：只恢复50%
	// 首发精准度
	, bEnableFirstShotAccuracy(true)
	, FirstShotAccuracyTime(0.2f)
	, bFirstShotAccuracyHipFireOnly(true)
	// 瞄准参数
	, AimDownSightTime(0.2f)
	, ADSFOVMultiplier(0.8f)
	, ADSMoveSpeedMultiplier(0.8f)
{
}
