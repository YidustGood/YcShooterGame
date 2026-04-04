// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "YcInventoryItemDefinition.h"

#include "ItemFragment_Heal.generated.h"

/**
 * Simple heal-consumable fragment for gameplay-side examples.
 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FItemFragment_Heal : public FYcInventoryItemFragment
{
	GENERATED_BODY()

	// Health amount restored after using this item.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float HealAmount = 25.0f;

	// Whether this item should be consumed after successful use.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bConsumeOnUse = true;

	// How many stacks to consume on one use.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int32 ConsumeCount = 1;
};
