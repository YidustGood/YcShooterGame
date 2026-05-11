// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcGridItemContainerLibrary.h"

#include "DataRegistrySubsystem.h"
#include "GridInventoryManagerComponent.h"
#include "System/YcDataRegistrySubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGridItemContainerLibrary)

DEFINE_LOG_CATEGORY_STATIC(LogYcGridItemContainer, Log, All);

namespace YcGridItemContainerLibrary
{
	static bool IsValidLootEntry(const FYcContainerLootPoolEntry& Entry)
	{
		return Entry.ItemRegistryId.IsValid() && Entry.StackCount > 0;
	}

	static bool TryAddItemByFirstFit(UGridInventoryManagerComponent* InventoryManager, const FDataRegistryId& ItemRegistryId, const int32 StackCount)
	{
		if (!InventoryManager || !ItemRegistryId.IsValid() || StackCount <= 0)
		{
			return false;
		}

		FGameplayTag RegionId;
		int32 PocketIndex = -1;
		FIntPoint Tile = FIntPoint::ZeroValue;
		bool bRotated = false;
		if (!InventoryManager->FindFirstFitPlacement(ItemRegistryId, RegionId, PocketIndex, Tile, bRotated))
		{
			return false;
		}

		return InventoryManager->TryAddGridItemByDefinition(ItemRegistryId, StackCount, Tile, bRotated, RegionId, PocketIndex);
	}
}

bool UYcGridItemContainerLibrary::GetLootPoolRow(const FDataRegistryId& LootPoolId, FYcContainerLootPoolRow& OutLootPoolRow)
{
	OutLootPoolRow = FYcContainerLootPoolRow();

	if (!LootPoolId.IsValid())
	{
		return false;
	}

	const UDataRegistrySubsystem* RegistrySubsystem = UDataRegistrySubsystem::Get();
	if (!RegistrySubsystem)
	{
		UE_LOG(LogYcGridItemContainer, Warning, TEXT("GetLootPoolRow failed: DataRegistrySubsystem is unavailable. LootPoolId=%s"), *LootPoolId.ToString());
		return false;
	}

	const FYcContainerLootPoolRow* LootPoolRow = RegistrySubsystem->GetCachedItem<FYcContainerLootPoolRow>(LootPoolId);
	if (!LootPoolRow)
	{
		UYcDataRegistrySubsystem::PrimeItemForRuntime(LootPoolId);
		UE_LOG(LogYcGridItemContainer, Warning, TEXT("GetLootPoolRow failed: loot pool is not cached or not found. Requested acquire/recovery. LootPoolId=%s"), *LootPoolId.ToString());
		return false;
	}

	OutLootPoolRow = *LootPoolRow;
	return true;
}

int32 UYcGridItemContainerLibrary::PopulateContainerInventory(
	UGridInventoryManagerComponent* InventoryManager,
	const FYcInventoryPickup& LegacyPickupInventory,
	EYcContainerItemSpawnStrategy SpawnStrategy,
	const FDataRegistryId& LootPoolId,
	int32 SpawnCountOverride)
{
	if (!InventoryManager)
	{
		UE_LOG(LogYcGridItemContainer, Warning, TEXT("PopulateContainerInventory ignored: InventoryManager is null."));
		return 0;
	}

	int32 AddedCount = 0;

	switch (SpawnStrategy)
	{
	case EYcContainerItemSpawnStrategy::LegacyStaticInventory:
		for (const FYcPickupTemplate& ItemTemplate : LegacyPickupInventory.Templates)
		{
			if (!ItemTemplate.ItemRegistryId.IsValid() || ItemTemplate.StackCount <= 0)
			{
				continue;
			}

			if (YcGridItemContainerLibrary::TryAddItemByFirstFit(InventoryManager, ItemTemplate.ItemRegistryId, ItemTemplate.StackCount))
			{
				++AddedCount;
			}
		}
		return AddedCount;

	case EYcContainerItemSpawnStrategy::RandomPool:
		break;

	default:
		UE_LOG(LogYcGridItemContainer, Warning, TEXT("PopulateContainerInventory encountered unknown strategy value: %d"), static_cast<int32>(SpawnStrategy));
		return 0;
	}

	FYcContainerLootPoolRow LootPoolRow;
	if (!GetLootPoolRow(LootPoolId, LootPoolRow))
	{
		return 0;
	}

	TArray<FYcContainerLootPoolEntry> ValidCandidates;
	ValidCandidates.Reserve(LootPoolRow.Candidates.Num());
	for (const FYcContainerLootPoolEntry& Candidate : LootPoolRow.Candidates)
	{
		if (YcGridItemContainerLibrary::IsValidLootEntry(Candidate))
		{
			ValidCandidates.Add(Candidate);
		}
	}

	if (ValidCandidates.Num() <= 0)
	{
		UE_LOG(LogYcGridItemContainer, Warning, TEXT("PopulateContainerInventory ignored: loot pool has no valid candidates. LootPoolId=%s"), *LootPoolId.ToString());
		return 0;
	}

	const int32 EffectiveSpawnCount = SpawnCountOverride > 0 ? SpawnCountOverride : LootPoolRow.SpawnCount;
	if (EffectiveSpawnCount <= 0)
	{
		UE_LOG(LogYcGridItemContainer, Warning, TEXT("PopulateContainerInventory ignored: effective spawn count is <= 0. LootPoolId=%s Override=%d Default=%d"),
			*LootPoolId.ToString(),
			SpawnCountOverride,
			LootPoolRow.SpawnCount);
		return 0;
	}

	for (int32 SpawnIndex = 0; SpawnIndex < EffectiveSpawnCount; ++SpawnIndex)
	{
		const int32 CandidateIndex = FMath::RandRange(0, ValidCandidates.Num() - 1);
		const FYcContainerLootPoolEntry& Candidate = ValidCandidates[CandidateIndex];
		if (!YcGridItemContainerLibrary::TryAddItemByFirstFit(InventoryManager, Candidate.ItemRegistryId, Candidate.StackCount))
		{
			UE_LOG(LogYcGridItemContainer, Verbose, TEXT("PopulateContainerInventory stopped early: no more fitting position. LootPoolId=%s SpawnIndex=%d Item=%s"),
				*LootPoolId.ToString(),
				SpawnIndex,
				*Candidate.ItemRegistryId.ToString());
			break;
		}

		++AddedCount;
	}

	return AddedCount;
}
