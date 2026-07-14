// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcInventoryLibrary.h"

#include "DataRegistrySubsystem.h"
#include "YcInventoryItemInstance.h"
#include "YcInventoryManagerComponent.h"
#include "YcPickupable.h"
#include "YiChenInventory.h"
#include "System/YcDataRegistrySubsystem.h"
#include "Engine/AssetManager.h"
#include "Fragments/ItemFragment_DataAsset.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcInventoryLibrary)

FInstancedStruct UYcInventoryLibrary::FindItemFragment(const FYcInventoryItemDefinition& ItemDef,
                                                       const UScriptStruct* FragmentStructType)
{
	FInstancedStruct Result;
	for (auto& Frag : ItemDef.Fragments)
	{	
		if (FragmentStructType == Frag.GetScriptStruct())
		{
			Result.InitializeAs(Frag.GetScriptStruct(), Frag.GetMemory());
			return Result;
		}
	}
	return Result;
}

FInstancedStruct UYcInventoryLibrary::FindItemFragmentById(const FDataRegistryId ItemDefId,
	const UScriptStruct* FragmentStructType)
{
	FYcInventoryItemDefinition ItemDef;
	GetItemDefinition(ItemDefId, ItemDef);
	
	return FindItemFragment(ItemDef, FragmentStructType);
}

UYcInventoryManagerComponent* UYcInventoryLibrary::GetInventoryManagerComponent(const AActor* InventoryOwnerActor)
{
	return UYcInventoryManagerComponent::FindInventoryManager(InventoryOwnerActor);
}

bool UYcInventoryLibrary::HasItem(const AActor* InventoryOwnerActor, const FDataRegistryId& ItemDataRegistryId)
{
	return GetItemCount(InventoryOwnerActor, ItemDataRegistryId) > 0;
}

int32 UYcInventoryLibrary::GetItemCount(const AActor* InventoryOwnerActor, const FDataRegistryId& ItemDataRegistryId)
{
	if (!InventoryOwnerActor || !ItemDataRegistryId.IsValid())
	{
		return 0;
	}

	UYcInventoryManagerComponent* InventoryManager = GetInventoryManagerComponent(InventoryOwnerActor);
	if (!InventoryManager)
	{
		return 0;
	}

	FYcInventoryItemDefinition ItemDef;
	if (!GetItemDefinition(ItemDataRegistryId, ItemDef))
	{
		return 0;
	}

	int32 TotalCount = 0;
	for (UYcInventoryItemInstance* ItemInstance : InventoryManager->GetAllItemInstance())
	{
		if (!IsValid(ItemInstance))
		{
			continue;
		}

		const FYcInventoryItemDefinition* EntryItemDef = ItemInstance->GetItemDef();
		if (EntryItemDef && EntryItemDef->ItemId == ItemDef.ItemId)
		{
			TotalCount += InventoryManager->GetStackCountByItemInstance(ItemInstance);
		}
	}

	return TotalCount;
}

bool UYcInventoryLibrary::ConsumeItem(const AActor* InventoryOwnerActor, const FDataRegistryId& ItemDataRegistryId, int32 Count)
{
	if (!InventoryOwnerActor || !ItemDataRegistryId.IsValid() || Count < 0)
	{
		return false;
	}

	if (Count == 0)
	{
		return true;
	}

	UYcInventoryManagerComponent* InventoryManager = GetInventoryManagerComponent(InventoryOwnerActor);
	if (!InventoryManager)
	{
		return false;
	}

	FYcInventoryItemDefinition ItemDef;
	if (!GetItemDefinition(ItemDataRegistryId, ItemDef))
	{
		return false;
	}

	if (GetItemCount(InventoryOwnerActor, ItemDataRegistryId) < Count)
	{
		return false;
	}

	int32 RemainingToConsume = Count;
	for (UYcInventoryItemInstance* ItemInstance : InventoryManager->GetAllItemInstance())
	{
		if (!IsValid(ItemInstance))
		{
			continue;
		}

		const FYcInventoryItemDefinition* EntryItemDef = ItemInstance->GetItemDef();
		if (!EntryItemDef || EntryItemDef->ItemId != ItemDef.ItemId)
		{
			continue;
		}

		const int32 StackCount = InventoryManager->GetStackCountByItemInstance(ItemInstance);
		if (StackCount <= 0)
		{
			continue;
		}

		const int32 ConsumeThisTime = FMath::Min(StackCount, RemainingToConsume);
		if (!InventoryManager->ConsumeItemInstance(ItemInstance, ConsumeThisTime))
		{
			return false;
		}

		RemainingToConsume -= ConsumeThisTime;
		if (RemainingToConsume == 0)
		{
			return true;
		}
	}

	return false;
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
		UYcDataRegistrySubsystem::PrimeItemForRuntime(ItemDataRegistryId);
		UE_LOG(LogYcInventory, Error, TEXT("UYcInventoryLibrary::GetItemDefinition - Failed to get ItemDef from DataRegistry: %s. Requested acquire/recovery; retry after registries are ready."),
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

bool UYcInventoryLibrary::LoadItemDataAssetByTagAsync(UObject* WorldContextObject, const FDataRegistryId& ItemDataRegistryId,
	const FGameplayTag& AssetTag)
{
	if (!IsValid(WorldContextObject) || !ItemDataRegistryId.IsValid() || !AssetTag.IsValid())
	{
		UE_LOG(LogYcInventory, Warning, TEXT("LoadItemDataAssetByTagAsync failed: invalid args. Context=%s Item=%s Tag=%s"),
			*GetNameSafe(WorldContextObject),
			*ItemDataRegistryId.ToString(),
			*AssetTag.ToString());
		return false;
	}

	FYcInventoryItemDefinition ItemDef;
	if (!GetItemDefinition(ItemDataRegistryId, ItemDef))
	{
		return false;
	}

	FYcDataAssetEntry Entry;
	if (!GetYcDataAssetEntryByTag(ItemDef, AssetTag, Entry))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("LoadItemDataAssetByTagAsync failed: item %s has no data asset for tag %s."),
			*ItemDataRegistryId.ToString(),
			*AssetTag.ToString());
		return false;
	}

	const TWeakObjectPtr<UObject> WeakRelatedObject(WorldContextObject);
	const FPrimaryAssetId AssetId = Entry.DataAssetId;
	auto BroadcastLoadedMessage = [WeakRelatedObject, AssetTag, AssetId]()
	{
		UObject* RelatedObject = WeakRelatedObject.Get();
		if (!IsValid(RelatedObject))
		{
			UE_LOG(LogYcInventory, Warning, TEXT("LoadItemDataAssetByTagAsync skipped broadcast: related object invalid. Asset=%s Tag=%s"),
				*AssetId.ToString(),
				*AssetTag.ToString());
			return;
		}

		UWorld* World = RelatedObject->GetWorld();
		if (!World || !World->IsGameWorld())
		{
			UE_LOG(LogYcInventory, Warning, TEXT("LoadItemDataAssetByTagAsync skipped broadcast: invalid world. RelatedObject=%s Asset=%s Tag=%s"),
				*GetNameSafe(RelatedObject),
				*AssetId.ToString(),
				*AssetTag.ToString());
			return;
		}

		FYcDataAssetLifecycleMessage Message;
		Message.LoadedDataAsset = Cast<UPrimaryDataAsset>(UAssetManager::Get().GetPrimaryAssetObject(AssetId));
		Message.RelatedObject = RelatedObject;
		Message.AssetTag = AssetTag;
		Message.bIsLoaded = true;

		UGameplayMessageSubsystem::Get(World).BroadcastMessage(AssetTag, Message);
		UE_LOG(LogYcInventory, Log, TEXT("LoadItemDataAssetByTagAsync broadcasted: RelatedObject=%s Asset=%s Tag=%s LoadedAsset=%s"),
			*GetNameSafe(RelatedObject),
			*AssetId.ToString(),
			*AssetTag.ToString(),
			*GetNameSafe(Message.LoadedDataAsset));
	};

	if (Entry.IsDataAssetLoaded())
	{
		BroadcastLoadedMessage();
		return true;
	}

	FStreamableDelegate OnLoadCompleted;
	OnLoadCompleted.BindLambda(BroadcastLoadedMessage);
	const TSharedPtr<FStreamableHandle> LoadHandle = Entry.LoadDataAssetAsync(OnLoadCompleted, false);
	if (!LoadHandle.IsValid() && !Entry.IsDataAssetLoaded())
	{
		UE_LOG(LogYcInventory, Warning, TEXT("LoadItemDataAssetByTagAsync request did not return a valid handle and asset is still not loaded. Context=%s Item=%s Tag=%s Asset=%s"),
			*GetNameSafe(WorldContextObject),
			*ItemDataRegistryId.ToString(),
			*AssetTag.ToString(),
			*Entry.DataAssetId.ToString());
	}
	return true;
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

bool UYcInventoryLibrary::GetPrimaryPickupItemRegistryId(const FYcInventoryPickup& PickupInventory,
	FDataRegistryId& OutItemDataRegistryId)
{
	OutItemDataRegistryId = FDataRegistryId();

	for (const FYcPickupInstance& PickupInstance : PickupInventory.Instances)
	{
		if (IsValid(PickupInstance.Item) && PickupInstance.Item->GetItemRegistryId().IsValid())
		{
			OutItemDataRegistryId = PickupInstance.Item->GetItemRegistryId();
			return true;
		}
	}

	for (const FYcPickupTemplate& PickupTemplate : PickupInventory.Templates)
	{
		if (PickupTemplate.ItemRegistryId.IsValid())
		{
			OutItemDataRegistryId = PickupTemplate.ItemRegistryId;
			return true;
		}
	}

	return false;
}

bool UYcInventoryLibrary::GetPickupInventory(const UObject* Object, FYcInventoryPickup& OutPickup)
{
	check(Object);
	if (const auto YcPickupable = Cast<IYcPickupable>(Object))
	{
		OutPickup = YcPickupable->GetPickupInventory();
		return true;
	}
	return false;
}
