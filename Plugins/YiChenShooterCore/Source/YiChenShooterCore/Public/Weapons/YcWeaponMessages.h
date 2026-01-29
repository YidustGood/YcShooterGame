// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"
#include "YcWeaponMessages.generated.h"

class UYcWeaponInstance;
class UYcHitScanWeaponInstance;
class APawn;
class APlayerController;

// ════════════════════════════════════════════════════════════════════════════
// GameplayMessage Tags - 武器系统消息标签
// ════════════════════════════════════════════════════════════════════════════

/** 武器属性变化消息 */
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Message_Weapon_StatsChanged);

// ════════════════════════════════════════════════════════════════════════════
// 消息结构体
// ════════════════════════════════════════════════════════════════════════════

/**
 * FYcWeaponStatsChangedMessage - 武器属性变化消息
 * 
 * 当武器属性发生变化时广播（配件变化、Buff、技能等）。
 * UI 和其他解耦系统可以监听此消息更新显示。
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcWeaponStatsChangedMessage
{
	GENERATED_BODY()

	/** 武器实例弱引用 */
	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	TWeakObjectPtr<UYcWeaponInstance> WeaponInstance;

	/** 武器拥有者Pawn弱引用 */
	UPROPERTY(BlueprintReadOnly, Category="Weapon")
	TWeakObjectPtr<APawn> OwnerPawn;
};

/**
 * FYcWeaponADSMessage - 武器 ADS 状态变化消息
 * 
 * 当武器进入或退出 ADS（瞄准）状态时广播。
 * 动画系统和其他解耦系统可以监听此消息来响应 ADS 状态变化。
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcWeaponADSMessage
{
	GENERATED_BODY()

	/** 是否正在瞄准 */
	UPROPERTY(BlueprintReadWrite, Category="Weapon")
	bool bIsAiming = false;

	/** 武器实例弱引用 */
	UPROPERTY(BlueprintReadWrite, Category="Weapon")
	TWeakObjectPtr<UYcHitScanWeaponInstance> WeaponInstance;

	/** 玩家控制器弱引用 */
	UPROPERTY(BlueprintReadWrite, Category="Weapon")
	TWeakObjectPtr<APlayerController> PlayerController;
};
