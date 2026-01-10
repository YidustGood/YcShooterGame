// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Fragments/YcEquipmentFragment.h"
#include "YcWeaponLibrary.generated.h"

class UYcEquipmentInstance;
class UYcWeaponVisualData;
struct FYcComputedWeaponStats;

/**
 * 武器系统蓝图函数库
 * 提供武器相关的便捷函数
 */
UCLASS()
class YICHENSHOOTERCORE_API UYcWeaponLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * 获取装备的武器视觉数据（便捷函数）
	 * @param Equipment 装备实例
	 * @return 武器视觉数据指针，未找到或未加载返回 nullptr
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Weapon|Fragment")
	static UYcWeaponVisualData* GetWeaponVisualData(UYcEquipmentInstance* Equipment);

	// ════════════════════════════════════════════════════════════════════════
	// Break 函数 - 分解武器数值结构体
	// ════════════════════════════════════════════════════════════════════════

	/**
	 * 分解武器数值 - 完整版
	 * 包含所有武器属性，未使用的引脚可以在蓝图中收起
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Stats", meta = (NativeBreakFunc, AdvancedDisplay = "1"))
	static void BreakWeaponStats(
		const FYcComputedWeaponStats& Stats,
		// 射击参数
		int32& BulletsPerCartridge,
		float& MaxDamageRange,
		float& BulletTraceSweepRadius,
		float& FireRate,
		// 伤害参数
		float& BaseDamage,
		float& HeadshotMultiplier,
		float& ArmorPenetration,
		// 弹药参数
		int32& MagazineSize,
		int32& MaxReserveAmmo,
		float& ReloadTime,
		float& TacticalReloadTime,
		// 腰射扩散参数
		float& HipFireBaseSpread,
		float& HipFireMaxSpread,
		float& HipFireSpreadPerShot,
		float& SpreadRecoveryRate,
		float& SpreadRecoveryDelay,
		float& SpreadExponent,
		// 瞄准扩散参数
		float& ADSBaseSpread,
		float& ADSMaxSpread,
		float& ADSSpreadPerShot,
		// 状态扩散乘数
		float& MovingSpreadMultiplier,
		float& JumpingSpreadMultiplier,
		float& CrouchingSpreadMultiplier,
		// 后坐力参数
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
		// 瞄准参数
		float& AimDownSightTime,
		float& ADSFOVMultiplier,
		float& ADSMoveSpeedMultiplier
	);

	/**
	 * 分解武器数值 - 伤害相关
	 * 只包含伤害计算需要的属性
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Stats", meta = (NativeBreakFunc))
	static void BreakWeaponStats_Damage(
		const FYcComputedWeaponStats& Stats,
		float& BaseDamage,
		float& HeadshotMultiplier,
		float& ArmorPenetration,
		float& MaxDamageRange
	);

	/**
	 * 分解武器数值 - 射击相关
	 * 只包含射击机制需要的属性
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Stats", meta = (NativeBreakFunc))
	static void BreakWeaponStats_Firing(
		const FYcComputedWeaponStats& Stats,
		int32& BulletsPerCartridge,
		float& FireRate,
		float& MaxDamageRange,
		float& BulletTraceSweepRadius
	);

	/**
	 * 分解武器数值 - 弹药相关
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Stats", meta = (NativeBreakFunc))
	static void BreakWeaponStats_Ammo(
		const FYcComputedWeaponStats& Stats,
		int32& MagazineSize,
		int32& MaxReserveAmmo,
		float& ReloadTime,
		float& TacticalReloadTime
	);

	/**
	 * 分解武器数值 - 扩散相关
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Stats", meta = (NativeBreakFunc, AdvancedDisplay = "4"))
	static void BreakWeaponStats_Spread(
		const FYcComputedWeaponStats& Stats,
		// 腰射
		float& HipFireBaseSpread,
		float& HipFireMaxSpread,
		float& HipFireSpreadPerShot,
		// 瞄准
		float& ADSBaseSpread,
		float& ADSMaxSpread,
		float& ADSSpreadPerShot,
		// 恢复
		float& SpreadRecoveryRate,
		float& SpreadRecoveryDelay,
		float& SpreadExponent,
		// 状态乘数
		float& MovingSpreadMultiplier,
		float& JumpingSpreadMultiplier,
		float& CrouchingSpreadMultiplier
	);

	/**
	 * 分解武器数值 - 后坐力相关
	 */
	UFUNCTION(BlueprintPure, Category = "Weapon|Stats", meta = (NativeBreakFunc, AdvancedDisplay = "5"))
	static void BreakWeaponStats_Recoil(
		const FYcComputedWeaponStats& Stats,
		float& VerticalRecoilMin,
		float& VerticalRecoilMax,
		float& HorizontalRecoilMin,
		float& HorizontalRecoilMax,
		// 乘数
		float& ADSRecoilMultiplier,
		float& HipFireRecoilMultiplier,
		float& CrouchingRecoilMultiplier,
		// 恢复
		float& RecoilRecoveryRate,
		float& RecoilRecoveryDelay,
		float& RecoilRecoveryPercent
	);
};