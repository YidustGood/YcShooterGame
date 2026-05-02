// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "YcDamageRuntimePayloads.generated.h"

/**
 * 背刺运行时 Payload
 * 由攻击发起侧在命中时写入上下文，供背刺组件读取。
 */
USTRUCT(BlueprintType)
struct YICHENDAMAGE_API FYcBackstabRuntimePayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="1.0"))
	float DamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0.0", ClampMax="90.0"))
	float MaxAngleDegrees = 45.0f;
};
