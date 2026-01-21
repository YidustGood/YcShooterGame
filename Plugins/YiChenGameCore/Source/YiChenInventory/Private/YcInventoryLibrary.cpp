// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcInventoryLibrary.h"

#include "DataRegistrySubsystem.h"
#include "YcInventoryItemInstance.h"
#include "YcInventoryManagerComponent.h"
#include "YiChenInventory.h"
#include "Fragments/ItemFragment_DataAsset.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcInventoryLibrary)

TInstancedStruct<FYcInventoryItemFragment> UYcInventoryLibrary::FindItemFragment(const FYcInventoryItemDefinition& ItemDef,
                                                                                 const UScriptStruct* FragmentStructType)
{
	TInstancedStruct<FYcInventoryItemFragment> NullStruct;
	for (auto& Frag : ItemDef.Fragments)
	{	
		if (FragmentStructType == Frag.GetScriptStruct())
		{
			return Frag;
		}
	}
	return NullStruct;
}

UYcInventoryManagerComponent* UYcInventoryLibrary::GetInventoryManagerComponent(const AActor* Actor)
{
	if(Actor == nullptr) return nullptr;
	
	if(UYcInventoryManagerComponent* InventoryManagerComponent = Actor->FindComponentByClass<UYcInventoryManagerComponent>())
	{
		return InventoryManagerComponent;
	}
	
	return nullptr;
}

bool UYcInventoryLibrary::GetItemDefinition(const FDataRegistryId& ItemDataRegistryId, FYcInventoryItemDefinition& OutItemDef)
{
	// 验证ItemRegistryId有效性
	if (!ItemDataRegistryId.IsValid())
	{
		UE_LOG(LogYcInventory, Error, TEXT("UYcInventoryLibrary::GetItemDefinition - ItemRegistryId is invalid."));
		return false;
	}
	
	// 从DataRegistry获取物品定义
	const UDataRegistrySubsystem* Subsystem = UDataRegistrySubsystem::Get();
	if (!Subsystem)
	{
		UE_LOG(LogYcInventory, Error, TEXT("UYcInventoryLibrary::GetItemDefinition - DataRegistrySubsystem not available."));
		return false;
	}
	
	// 使用模板方法直接获取类型安全的指针
	const FYcInventoryItemDefinition* ItemDef = Subsystem->GetCachedItem<FYcInventoryItemDefinition>(ItemDataRegistryId);
	if (!ItemDef)
	{
		UE_LOG(LogYcInventory, Error, TEXT("UYcInventoryLibrary::GetItemDefinition - Failed to get ItemDef from DataRegistry: %s. Make sure the item is cached/acquired first."),
			*ItemDataRegistryId.ToString());
		return false;
	}
	
	// 检查物品是否启用
	if (!ItemDef->bEnableItem)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("UYcInventoryLibrary::GetItemDefinition - Item is disabled: %s"), *ItemDef->ItemId.ToString());
		return false;
	}
	
	OutItemDef = *ItemDef;
	return true;
}

void UYcInventoryLibrary::LoadItemDefDataAssetAsync(UObject* WorldContextObject,
	const FDataRegistryId& ItemDataRegistryId)
{
	FYcInventoryItemDefinition ItemDef;
	if (!GetItemDefinition(ItemDataRegistryId, ItemDef)) return;
	
	// @TODO 目前这样做只支持Item配置一个FItemFragment_DataAsset, 所有的资产都要配置在这里, 是否考虑支持多个FItemFragment_DataAsset呢?
	 FItemFragment_DataAsset* DataAssetFragment = const_cast<FItemFragment_DataAsset*>(ItemDef.GetTypedFragment<FItemFragment_DataAsset>());
	if (!DataAssetFragment) return;
	
	DataAssetFragment->LoadAllDataAssetAsync(WorldContextObject);
}

UPrimaryDataAsset* UYcInventoryLibrary::GetYcDataAssetByTag(const FYcInventoryItemDefinition& ItemDef,
	const FGameplayTag& AssetTag)
{
	const FItemFragment_DataAsset* DataAssetFragment = ItemDef.GetTypedFragment<FItemFragment_DataAsset>();
	if (!DataAssetFragment) return nullptr;
	
	const FYcDataAssetEntry* Entry = DataAssetFragment->GetDataAssetByTag(AssetTag);
	if (!Entry) return nullptr;
	
	return Entry->GetLoadedDataAsset();
}

bool UYcInventoryLibrary::GetYcDataAssetEntryByTag(const FYcInventoryItemDefinition& ItemDef,
	const FGameplayTag& AssetTag, FYcDataAssetEntry& OutDataAssetEntry)
{
	const FItemFragment_DataAsset* DataAssetFragment = ItemDef.GetTypedFragment<FItemFragment_DataAsset>();
	if (!DataAssetFragment) return false;
	
	const FYcDataAssetEntry* Entry = DataAssetFragment->GetDataAssetByTag(AssetTag);
	if (!Entry) return false;
	
	OutDataAssetEntry = *Entry;
	
	return true;
}