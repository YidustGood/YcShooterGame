// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcInventoryItemDefinition.h"
#include "YiChenEquipment/Public/YcEquipmentDefinition.h"
#include "InventoryFragment_Equippable.generated.h"

struct FYcEquipmentDefinition;

/**
 * 可装备功能片段, 给InventoryItem添加了该片段后就拥有了可装备的功能
 */
USTRUCT(BlueprintType)
struct YICHENEQUIPMENT_API FInventoryFragment_Equippable : public FYcInventoryItemFragment
{
	GENERATED_BODY()
	
	// 装备定义类型
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FYcEquipmentDefinition EquipmentDef;
};