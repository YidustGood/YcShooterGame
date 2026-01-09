// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcEquipmentVisualData.h"

UYcEquipmentVisualData::UYcEquipmentVisualData()
{
}

const FYcEquipmentVisualAssetEntry* UYcEquipmentVisualData::GetAsset(FGameplayTag AssetTag) const
{
	if (!AssetTag.IsValid())
	{
		return nullptr;
	}

	return ExtendedAssets.Find(AssetTag);
}

TArray<const FYcEquipmentVisualAssetEntry*> UYcEquipmentVisualData::GetAssetsByType(EEquipmentVisualAssetType AssetType) const
{
	TArray<const FYcEquipmentVisualAssetEntry*> Result;
	
	for (const auto& Pair : ExtendedAssets)
	{
		if (Pair.Value.AssetType == AssetType)
		{
			Result.Add(&Pair.Value);
		}
	}
	
	return Result;
}

bool UYcEquipmentVisualData::K2_GetAsset(FGameplayTag AssetTag, FYcEquipmentVisualAssetEntry& OutAssetEntry) const
{
	if (const FYcEquipmentVisualAssetEntry* Found = GetAsset(AssetTag))
	{
		OutAssetEntry = *Found;
		return true;
	}
	return false;
}

UObject* UYcEquipmentVisualData::K2_GetAssetObject(FGameplayTag AssetTag) const
{
	if (const FYcEquipmentVisualAssetEntry* Found = GetAsset(AssetTag))
	{
		return Found->Asset.Get();
	}
	return nullptr;
}

bool UYcEquipmentVisualData::HasAsset(FGameplayTag AssetTag) const
{
	return AssetTag.IsValid() && ExtendedAssets.Contains(AssetTag);
}

TArray<FGameplayTag> UYcEquipmentVisualData::GetAllAssetTags() const
{
	TArray<FGameplayTag> Tags;
	ExtendedAssets.GetKeys(Tags);
	return Tags;
}
