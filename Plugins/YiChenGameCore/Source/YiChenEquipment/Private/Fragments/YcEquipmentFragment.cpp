// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Fragments/YcEquipmentFragment.h"

#include "YcEquipmentInstance.h"
#include "YiChenEquipment.h"
#include "YcEquipmentVisualData.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcEquipmentFragment)

//////////////////////////////////////////////////////////////////////
// FEquipmentFragment_VisualAsset

TSharedPtr<FStreamableHandle> FEquipmentFragment_VisualAsset::LoadVisualDataAsync(FStreamableDelegate OnLoadCompleted) const
{
	if (!VisualDataId.IsValid())
	{
		UE_LOG(LogYcEquipment, Warning, TEXT("FEquipmentFragment_VisualAsset::LoadVisualDataAsync - Invalid VisualDataId"));
		return nullptr;
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	
	// 检查是否已加载
	if (AssetManager.GetPrimaryAssetObject(VisualDataId))
	{
		UE_LOG(LogYcEquipment, Log, TEXT("VisualData already loaded: %s"), *VisualDataId.ToString());
		// 已加载，直接触发回调
		if (OnLoadCompleted.IsBound())
		{
			OnLoadCompleted.Execute();
		}
		return nullptr;
	}

	UE_LOG(LogYcEquipment, Log, TEXT("Loading VisualData: %s with %d bundles"), 
		*VisualDataId.ToString(), AssetBundleNames.Num());
	
	return AssetManager.LoadPrimaryAsset(VisualDataId, AssetBundleNames, OnLoadCompleted);
}

void FEquipmentFragment_VisualAsset::UnloadVisualData() const
{
	if (!VisualDataId.IsValid())
	{
		return;
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	AssetManager.UnloadPrimaryAsset(VisualDataId);
	
	UE_LOG(LogYcEquipment, Log, TEXT("Unloaded VisualData: %s"), *VisualDataId.ToString());
}

UYcEquipmentVisualData* FEquipmentFragment_VisualAsset::GetLoadedVisualData() const
{
	if (!VisualDataId.IsValid())
	{
		return nullptr;
	}

	UAssetManager& AssetManager = UAssetManager::Get();
	return Cast<UYcEquipmentVisualData>(AssetManager.GetPrimaryAssetObject(VisualDataId));
}

bool FEquipmentFragment_VisualAsset::IsVisualDataLoaded() const
{
	return GetLoadedVisualData() != nullptr;
}

void FEquipmentFragment_VisualAsset::OnEquipmentInstanceCreated(UYcEquipmentInstance* Instance) const
{
	UE_LOG(LogYcEquipment, Warning, TEXT("FEquipmentFragment_VisualAsset::OnEquipmentInstanceCreated: %s"), *GetNameSafe(Instance));
}
