// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "YcInventorySceneTypes.generated.h"

UENUM(BlueprintType)
enum class EYcInventorySceneType : uint8
{
	/** 局内场景（对局中）。 */
	InMatch,
	/** 局外场景（大厅/仓库）。 */
	OutOfMatch
};

