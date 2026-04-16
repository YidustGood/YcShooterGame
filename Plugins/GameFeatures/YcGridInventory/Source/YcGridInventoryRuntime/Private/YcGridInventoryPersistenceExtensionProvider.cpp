// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcGridInventoryPersistenceExtensionProvider.h"

#include "GridInventoryManagerComponent.h"
#include "YcInventoryItemInstance.h"
#include "YcInventoryManagerComponent.h"

const FName UYcGridInventoryPersistenceExtensionProvider::ExtensionKey(TEXT("GridInventory.Placements"));

FName UYcGridInventoryPersistenceExtensionProvider::GetExtensionKey() const
{
	return ExtensionKey;
}

int32 UYcGridInventoryPersistenceExtensionProvider::GetExtensionVersion() const
{
	return CurrentVersion;
}

bool UYcGridInventoryPersistenceExtensionProvider::CanHandleInventory(const UYcInventoryManagerComponent* Inventory) const
{
	return AsGrid(Inventory) != nullptr;
}

bool UYcGridInventoryPersistenceExtensionProvider::BuildInventoryExtensionPayload(const UYcInventoryManagerComponent* Inventory, FInstancedStruct& OutPayload, FString& OutReason) const
{
	const UGridInventoryManagerComponent* GridInventory = AsGrid(Inventory);
	if (!GridInventory)
	{
		OutReason = TEXT("Inventory is not UGridInventoryManagerComponent.");
		return false;
	}

	FYcGridInventoryExtensionPayload Payload;
	// 读取当前库存内所有已落位物品的坐标与旋转信息。
	const TMap<UYcInventoryItemInstance*, FIntPoint> TileMap = const_cast<UGridInventoryManagerComponent*>(GridInventory)->GetGridItemsTileMapByRegion(FGameplayTag(), -1);
	const TMap<UYcInventoryItemInstance*, bool> RotationMap = const_cast<UGridInventoryManagerComponent*>(GridInventory)->GetGridItemRotationMap();

	for (const TPair<UYcInventoryItemInstance*, FIntPoint>& Pair : TileMap)
	{
		UYcInventoryItemInstance* ItemInst = Pair.Key;
		if (!IsValid(ItemInst))
		{
			continue;
		}

		FYcGridInventoryPlacementRecord Placement;
		Placement.ItemInstId = ItemInst->GetItemInstId();
		Placement.GridTile = Pair.Value;
		if (const bool* bRotated = RotationMap.Find(ItemInst))
		{
			Placement.bRotated = *bRotated;
		}

		FGameplayTag RegionId;
		int32 PocketIndex = -1;
		// 额外记录区域/口袋，用于多区域恢复。
		if (GridInventory->GetItemPlacementRegion(ItemInst, RegionId, PocketIndex))
		{
			Placement.GridRegionId = RegionId;
			Placement.GridPocketIndex = PocketIndex;
		}

		Payload.Placements.Add(MoveTemp(Placement));
	}

	Payload.Placements.Sort([](const FYcGridInventoryPlacementRecord& A, const FYcGridInventoryPlacementRecord& B)
	{
		return A.ItemInstId.ToString() < B.ItemInstId.ToString();
	});

	// 统一放入结构化扩展载荷，交由 MetaInventory 快照保存。
	OutPayload.InitializeAs<FYcGridInventoryExtensionPayload>(MoveTemp(Payload));
	return true;
}

bool UYcGridInventoryPersistenceExtensionProvider::ApplyInventoryExtensionPayload(UYcInventoryManagerComponent* Inventory, const FInstancedStruct& Payload, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& ItemMap, FString& OutReason) const
{
	UGridInventoryManagerComponent* GridInventory = AsGrid(Inventory);
	if (!GridInventory)
	{
		OutReason = TEXT("Inventory is not UGridInventoryManagerComponent.");
		return false;
	}

	if (!Payload.IsValid() || !Payload.GetScriptStruct()->IsChildOf(FYcGridInventoryExtensionPayload::StaticStruct()))
	{
		OutReason = TEXT("Payload struct type mismatch.");
		return false;
	}

	const FYcGridInventoryExtensionPayload& GridPayload = Payload.Get<FYcGridInventoryExtensionPayload>();
	if (GridPayload.Placements.IsEmpty())
	{
		return true;
	}

	// 先清空当前网格占位，再按快照重放，避免旧占位残留冲突。
	for (const TPair<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& Pair : ItemMap)
	{
		if (IsValid(Pair.Value))
		{
			GridInventory->OnRemoveGridItem(Pair.Value);
		}
	}

	for (const FYcGridInventoryPlacementRecord& Placement : GridPayload.Placements)
	{
		const TObjectPtr<UYcInventoryItemInstance>* FoundItem = ItemMap.Find(Placement.ItemInstId);
		if (!FoundItem || !IsValid(*FoundItem))
		{
			continue;
		}

		UYcInventoryItemInstance* ItemInst = *FoundItem;
		const int32 StackCount = FMath::Max(1, GridInventory->GetStackCountByItemInstance(ItemInst));
		if (GridInventory->OnGridItemInstanceAdded(ItemInst, StackCount, Placement.GridTile, Placement.bRotated, Placement.GridRegionId, Placement.GridPocketIndex))
		{
			continue;
		}

		// 快照目标位不可用时，回退到首个可放置位，保证恢复尽量成功。
		FGameplayTag FitRegionId;
		int32 FitPocketIndex = -1;
		FIntPoint FitTile = FIntPoint::ZeroValue;
		bool bFitRotated = false;
		if (GridInventory->FindFirstFitPlacementForItemInst(ItemInst, FitRegionId, FitPocketIndex, FitTile, bFitRotated))
		{
			GridInventory->OnGridItemInstanceAdded(ItemInst, StackCount, FitTile, bFitRotated, FitRegionId, FitPocketIndex);
		}
	}

	return true;
}

UGridInventoryManagerComponent* UYcGridInventoryPersistenceExtensionProvider::AsGrid(UYcInventoryManagerComponent* Inventory)
{
	return Cast<UGridInventoryManagerComponent>(Inventory);
}

const UGridInventoryManagerComponent* UYcGridInventoryPersistenceExtensionProvider::AsGrid(const UYcInventoryManagerComponent* Inventory)
{
	return Cast<UGridInventoryManagerComponent>(Inventory);
}
