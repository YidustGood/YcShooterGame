// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "YcInventoryItemDefinition.h"

#include "ItemFragment_GridItem.generated.h"

/**
 * Fragment data used by the grid-inventory item definition.
 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FItemFragment_GridItem : public FYcInventoryItemFragment
{
	GENERATED_BODY()

	FItemFragment_GridItem(const int32 X, const int32 Y)
		: Dimensions(FIntPoint(X, Y))
		, Icon(nullptr)
		, IconTexture(nullptr)
		, IconRotated(nullptr)
	{
	}

	FItemFragment_GridItem()
		: Dimensions(FIntPoint(1, 1))
		, Icon(nullptr)
		, IconTexture(nullptr)
		, IconRotated(nullptr)
	{
	}

	// Item size in grid tiles (X, Y).
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FIntPoint Dimensions;

	// Item icon material.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UMaterialInterface* Icon;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UTexture* IconTexture;

	// Item icon material for rotated rendering.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UMaterialInterface* IconRotated;

	// Whether this item can be rotated.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bCanRotate = true;

	// Base search duration in container (seconds). <= 0 means revealed immediately.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float SearchDuration = 0.0f;
};

USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FGridInventoryLine
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FVector2D Start;

	UPROPERTY(BlueprintReadWrite)
	FVector2D End;
};
