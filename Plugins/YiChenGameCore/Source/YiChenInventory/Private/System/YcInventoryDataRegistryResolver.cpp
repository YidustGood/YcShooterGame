// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcInventoryDataRegistryResolver.h"
#include "YcInventoryItemDefinition.h"
#include "Fragments/ItemFragment_DataAsset.h"
#include "System/YcDataRegistrySubsystem.h"

bool FYcInventoryDataRegistryResolver::ResolveAssets(
	const FDataRegistryId& InDataRegistryId, 
	TArray<FPrimaryAssetId>& OutAssetIds) const
{

	const FItemFragment_DataAsset* DataAssetFragment = TryGetDataAssetFragment(InDataRegistryId);
	
	if (!DataAssetFragment)
	{
		// 没有 DataAsset Fragment，返回空列表（不是错误）
		UE_LOG(LogTemp, Verbose, 
			TEXT("FYcInventoryDataRegistryResolver: ItemDefinition 没有 DataAsset Fragment: %s"),
			*InDataRegistryId.ToString());
		return false;
	}
	
	// 遍历 DataAssetMap 提取所有 PrimaryAssetId
	for (const auto& Pair : DataAssetFragment->DataAssetMap)
	{
		const FYcDataAssetEntry& Entry = Pair.Value;
		if (Entry.DataAssetId.IsValid())
		{
			OutAssetIds.Add(Entry.DataAssetId);
		}
	}
	
	UE_LOG(LogTemp, Verbose, 
		TEXT("FYcInventoryDataRegistryResolver: 解析 %s 成功，找到 %d 个资产"),
		*InDataRegistryId.ToString(), OutAssetIds.Num());
	
	return true;
}

void FYcInventoryDataRegistryResolver::GetBundleNames(
	const FDataRegistryId& InDataRegistryId, 
	TArray<FName>& OutBundleNames) const
{
	const FItemFragment_DataAsset* DataAssetFragment = TryGetDataAssetFragment(InDataRegistryId);
	
	if (!DataAssetFragment)
	{
		// 没有 DataAsset Fragment，返回空列表（不是错误）
		UE_LOG(LogTemp, Verbose, 
			TEXT("FYcInventoryDataRegistryResolver: ItemDefinition 没有 DataAsset Fragment: %s"),
			*InDataRegistryId.ToString());
		return;
	}
	
	// 遍历 DataAssetMap 提取所有 BundleName
	for (const auto& Pair : DataAssetFragment->DataAssetMap)
	{
		const FYcDataAssetEntry& Entry = Pair.Value;
		if (Entry.DataAssetId.IsValid())
		{
			OutBundleNames.Append(Entry.AssetBundleNames);
		}
	}
}

const FItemFragment_DataAsset* FYcInventoryDataRegistryResolver::TryGetDataAssetFragment(
	const FDataRegistryId& InDataRegistryId) const
{
	// 通过 DataRegistryId 获取 ItemDefinition
	const FYcInventoryItemDefinition* ItemDefinition = 
		UYcDataRegistrySubsystem::GetCachedItem<FYcInventoryItemDefinition>(InDataRegistryId);
	
	if (!ItemDefinition)
	{
		UE_LOG(LogTemp, Warning, 
			TEXT("FYcInventoryDataRegistryResolver: 无法找到 ItemDefinition: %s"),
			*InDataRegistryId.ToString());
		return nullptr;
	}
	
	// 获取 FItemFragment_DataAsset
	const FItemFragment_DataAsset* DataAssetFragment = 
		ItemDefinition->GetTypedFragment<FItemFragment_DataAsset>();
	
	return DataAssetFragment;
}
