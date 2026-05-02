// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Weapons/Fragments/YcFragment_MeleeAttackConfig.h"
#include "YcWeaponRuntimePayloads.generated.h"

/**
 * 近战攻击运行时 Payload
 * 由攻击发起侧写入，用于在统一伤害链中选择当前攻击 Profile。
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcMeleeAttackRuntimePayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EYcMeleeAttackType AttackType = EYcMeleeAttackType::Light;
};
