// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DataRegistryId.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "Fragments/ItemFragment_GridRegions.h"

#include "YcGridInventoryPlacementRuleLibrary.generated.h"

class UYcInventoryItemInstance;

UCLASS()
class YCGRIDINVENTORYRUNTIME_API UYcGridInventoryPlacementRuleLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 用物品定义ID校验是否满足区域标签约束。
	 * Check whether an item definition passes region tag constraint.
	 */
	UFUNCTION(BlueprintCallable, Category = "Yc|Inventory|Grid|Rules")
	static bool PassesTagConstraintForItemDef(const FDataRegistryId& ItemDefId, const FGridRegionTagConstraint& Constraint);

	/**
	 * 用物品实例校验是否满足区域标签约束。
	 * Check whether an item instance passes region tag constraint.
	 */
	UFUNCTION(BlueprintCallable, Category = "Yc|Inventory|Grid|Rules")
	static bool PassesTagConstraintForItemInstance(UYcInventoryItemInstance* ItemInst, const FGridRegionTagConstraint& Constraint);
};
