// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcEquipmentVisualLibrary.h"
#include "YcEquipmentInstance.h"
#include "YcEquipmentVisualData.h"
#include "YcInventoryItemInstance.h"
#include "YcInventoryLibrary.h"
#include "YiChenEquipment.h"
#include "Engine/AssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcEquipmentVisualLibrary)

UYcEquipmentVisualData* UYcEquipmentVisualLibrary::GetEquipmentVisualData(UYcEquipmentInstance* Equipment)
{
	if (!Equipment || !Equipment->GetAssociatedItem() || !Equipment->GetAssociatedItem()->GetItemDef())
	{
		UE_LOG(LogYcEquipment, Error, TEXT("获取 EquipmentVisualData 失败, 关联物品定义无效!"));
		return nullptr;
	}
	
	UPrimaryDataAsset* DataAsset = UYcInventoryLibrary::GetYcDataAssetByTag(*Equipment->GetAssociatedItem()->GetItemDef(), TAG_Yc_Asset_Item_EquipmentVisualData);
	
	return Cast<UYcEquipmentVisualData>(DataAsset);
}
