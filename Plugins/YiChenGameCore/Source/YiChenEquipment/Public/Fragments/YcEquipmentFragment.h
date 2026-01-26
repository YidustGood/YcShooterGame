// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once


#include "YcEquipmentDefinition.h"
#include "YcEquipmentFragment.generated.h"

class UYcEquipmentInstance;
class UYcEquipmentVisualData;

/**
 * 快捷装备栏槽位 Fragment
 * 用于指定装备所能放置的QuickBar Slot Index
 */
USTRUCT(BlueprintType)
struct YICHENEQUIPMENT_API FEquipmentFragment_QuickBarSlot : public FYcEquipmentFragment
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 SlotIndex = 0;
};
