// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcInventoryItemDefinition.h"
#include "YcInventoryFragment_QuickBarSlotHUDIcon.generated.h"

class UTexture2D;

/**
 * QuickBar active slot HUD icon config.
 * Only the texture is configured here; the widget owns the shared UI material.
 */
USTRUCT(BlueprintType, meta = (DisplayName = "QuickBarSlot HUD Icon"))
struct YICHENSHOOTERCORE_API FYcInventoryFragment_QuickBarSlotHUDIcon : public FYcInventoryItemFragment
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "HUD")
	TObjectPtr<UTexture2D> IconTexture = nullptr;
};
