// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcMetaInventorySubsystem.h"

#include "System/YcInventoryPersistenceProvider.h"
#include "System/YcInventoryPersistenceProvider_LocalSave.h"
#include "System/YcMetaInventoryItemRecordCodec.h"
#include "System/YcInventorySceneContext.h"
#include "System/YcMetaInventoryVersion.h"
#include "YcInventoryItemInstance.h"
#include "YcInventoryManagerComponent.h"
#include "YcInventoryOperationTypes.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "YiChenInventory.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcMetaInventorySubsystem)

namespace
{
	static const FName EquipmentSlotComponentClassName(TEXT("YcEquipmentSlotComponent"));
	static const FName QuickBarComponentClassName(TEXT("YcQuickBarComponent"));

	static const FName Fn_GetOccupiedSlots(TEXT("GetOccupiedSlots"));
	static const FName Fn_GetItemInSlot(TEXT("GetItemInSlot"));
	static const FName Fn_UnequipSlot(TEXT("UnequipSlot"));
	static const FName Fn_EquipItem(TEXT("EquipItem"));

	static const FName Fn_GetSlots(TEXT("GetSlots"));
	static const FName Fn_RemoveItemFromSlot(TEXT("RemoveItemFromSlot"));
	static const FName Fn_AddItemToSlot(TEXT("AddItemToSlot"));
	static const FName Fn_OnRep_Slots(TEXT("OnRep_Slots"));
	static const FName Fn_GetGridItemsTileMap(TEXT("GetGridItemsTileMap"));
	static const FName Fn_GetGridItemRotationMap(TEXT("GetGridItemRotationMap"));
	static const FName Fn_OnRemoveGridItem(TEXT("OnRemoveGridItem"));
	static const FName Fn_OnGridItemInstanceAdded(TEXT("OnGridItemInstanceAdded"));
	static const FName Fn_FindFirstFitPosition(TEXT("FindFirstFitPosition"));

	static const FName OperationStateChangedTagName(TEXT("Yc.Inventory.Message.Operation.StateChanged"));
}

UYcMetaInventorySubsystem* UYcMetaInventorySubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<UYcMetaInventorySubsystem>();
		}
	}
	return nullptr;
}

void UYcMetaInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UWorld* World = GetWorld())
	{
		const FGameplayTag OperationStateChangedTag = FGameplayTag::RequestGameplayTag(OperationStateChangedTagName);
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(World);
		OperationStateListener = MessageSubsystem.RegisterListener<FYcInventoryOperationStateMessage>(
			OperationStateChangedTag,
			this,
			&ThisClass::OnOperationStateChanged);
	}

	EnsurePersistenceProvider();
}

void UYcMetaInventorySubsystem::Deinitialize()
{
	SaveDirtyProfiles();
	OperationStateListener.Unregister();
	DirtyProfiles.Empty();
	UnknownItemExtensionPayloads.Empty();
	SceneContexts.Empty();
	ContextByAccountId.Empty();
	PersistenceProvider = nullptr;

	Super::Deinitialize();
}

void UYcMetaInventorySubsystem::RegisterSceneContext(UYcInventorySceneContext* Context)
{
	if (!IsValid(Context))
	{
		return;
	}

	if (!OperationStateListener.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			const FGameplayTag OperationStateChangedTag = FGameplayTag::RequestGameplayTag(OperationStateChangedTagName);
			UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(World);
			OperationStateListener = MessageSubsystem.RegisterListener<FYcInventoryOperationStateMessage>(
				OperationStateChangedTag,
				this,
				&ThisClass::OnOperationStateChanged);
		}
	}

	SceneContexts.Remove(Context);
	SceneContexts.Add(Context);
	if (!Context->AccountId.IsEmpty())
	{
		ContextByAccountId.Add(Context->AccountId, Context);
	}
}

void UYcMetaInventorySubsystem::UnregisterSceneContext(UYcInventorySceneContext* Context)
{
	if (!Context)
	{
		return;
	}

	SceneContexts.Remove(Context);
	if (!Context->AccountId.IsEmpty())
	{
		if (const TObjectPtr<UYcInventorySceneContext>* Existing = ContextByAccountId.Find(Context->AccountId))
		{
			if (Existing->Get() == Context)
			{
				ContextByAccountId.Remove(Context->AccountId);
			}
		}
	}
}

bool UYcMetaInventorySubsystem::ValidateOutOfMatchContext(const UYcInventorySceneContext* Context, const TCHAR* Caller) const
{
	if (!IsValid(Context))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("%s: context is invalid."), Caller);
		return false;
	}

	if (!Context->IsValidForOutOfMatchPersistence())
	{
		UE_LOG(LogYcInventory, Warning, TEXT("%s: context is not eligible for out-of-match persistence. SceneType=%d, AccountId='%s', bRequirePersistenceCommit=%d"),
			Caller, static_cast<int32>(Context->SceneType), *Context->AccountId, Context->bRequirePersistenceCommit ? 1 : 0);
		return false;
	}

	return true;
}

bool UYcMetaInventorySubsystem::ValidateInMatchLoadoutRequest(const FString& AccountId, const AActor* ContextOwner, const UYcInventoryManagerComponent* InMatchPlayerInventory, const bool bRequireRuntimeObjects, const TCHAR* Caller) const
{
	if (AccountId.IsEmpty())
	{
		UE_LOG(LogYcInventory, Warning, TEXT("%s: invalid account id."), Caller);
		return false;
	}

	if (bRequireRuntimeObjects && (!IsValid(ContextOwner) || !IsValid(InMatchPlayerInventory)))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("%s: invalid runtime args."), Caller);
		return false;
	}

	return true;
}

bool UYcMetaInventorySubsystem::LoadOrInitializeProfile(UYcInventorySceneContext* Context)
{
	if (!ValidateOutOfMatchContext(Context, TEXT("LoadOrInitializeProfile")))
	{
		return false;
	}

	EnsurePersistenceProvider();
	if (!PersistenceProvider)
	{
		return false;
	}

	FYcMetaInventoryRootSnapshot Snapshot;
	if (!PersistenceProvider->LoadSnapshot(Context->AccountId, Snapshot))
	{
		Snapshot = YcMetaInventoryVersion::MakeEmptySnapshot(Context->AccountId);
		if (!ApplySnapshotToContext(Context, Snapshot))
		{
			return false;
		}
		return SaveProfile(Context);
	}

	if (!YcMetaInventoryVersion::IsSupportedVersion(Snapshot.SnapshotVersion))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("LoadOrInitializeProfile: snapshot version %d is deprecated, reinitializing new profile."), Snapshot.SnapshotVersion);
		FYcMetaInventoryRootSnapshot NewSnapshot = YcMetaInventoryVersion::MakeEmptySnapshot(Context->AccountId);
		if (!ApplySnapshotToContext(Context, NewSnapshot))
		{
			return false;
		}
		return SaveProfile(Context);
	}

	if (!ApplySnapshotToContext(Context, Snapshot))
	{
		return false;
	}

	ClearProfileDirty(Context->AccountId);
	return true;
}

bool UYcMetaInventorySubsystem::SaveProfile(UYcInventorySceneContext* Context)
{
	if (!ValidateOutOfMatchContext(Context, TEXT("SaveProfile")))
	{
		return false;
	}

	EnsurePersistenceProvider();
	if (!PersistenceProvider)
	{
		return false;
	}

	FYcMetaInventoryRootSnapshot Snapshot;
	if (!BuildSnapshotFromContext(Context, Snapshot))
	{
		return false;
	}

	YcMetaInventoryVersion::PrepareSnapshotForSave(Context->AccountId, Snapshot);

	const bool bSaved = PersistenceProvider->SaveSnapshot(Context->AccountId, Snapshot);
	if (bSaved)
	{
		ClearProfileDirty(Context->AccountId);
	}
	return bSaved;
}

bool UYcMetaInventorySubsystem::SaveDirtyProfiles()
{
	bool bAllSucceeded = true;

	for (int32 Index = SceneContexts.Num() - 1; Index >= 0; --Index)
	{
		UYcInventorySceneContext* Context = SceneContexts[Index];
		if (!IsValid(Context))
		{
			SceneContexts.RemoveAtSwap(Index);
			continue;
		}

		if (!Context->IsValidForOutOfMatchPersistence() || !DirtyProfiles.Contains(Context->AccountId))
		{
			continue;
		}

		if (!SaveProfile(Context))
		{
			bAllSucceeded = false;
		}
	}

	return bAllSucceeded;
}

bool UYcMetaInventorySubsystem::SetupOutOfMatchContextAndLoad(const FString& AccountId, AActor* ContextOwner, UYcInventoryManagerComponent* PlayerInventory, UYcInventoryManagerComponent* StashInventory)
{
	if (AccountId.IsEmpty() || !IsValid(ContextOwner) || !IsValid(PlayerInventory) || !IsValid(StashInventory))
	{
		return false;
	}

	UYcInventorySceneContext* Context = nullptr;
	if (TObjectPtr<UYcInventorySceneContext>* Existing = ContextByAccountId.Find(AccountId))
	{
		Context = Existing->Get();
	}
	if (!IsValid(Context))
	{
		Context = NewObject<UYcInventorySceneContext>(this);
	}

	Context->SceneType = EYcInventorySceneType::OutOfMatch;
	Context->AccountId = AccountId;
	Context->ContextOwner = ContextOwner;
	Context->PlayerInventoryRef = PlayerInventory;
	Context->ContainerInventoryRef = StashInventory;
	Context->bRequirePersistenceCommit = true;

	RegisterSceneContext(Context);
	ContextByAccountId.Add(AccountId, Context);
	return LoadOrInitializeProfile(Context);
}

bool UYcMetaInventorySubsystem::SaveOutOfMatchContext(const FString& AccountId)
{
	if (AccountId.IsEmpty())
	{
		return false;
	}

	TObjectPtr<UYcInventorySceneContext>* FoundContext = ContextByAccountId.Find(AccountId);
	if (!FoundContext || !IsValid(*FoundContext))
	{
		return false;
	}
	return SaveProfile(*FoundContext);
}

bool UYcMetaInventorySubsystem::LoadPlayerLoadoutToInMatch(const FString& AccountId, AActor* ContextOwner, UYcInventoryManagerComponent* InMatchPlayerInventory)
{
	if (!ValidateInMatchLoadoutRequest(AccountId, ContextOwner, InMatchPlayerInventory, true, TEXT("LoadPlayerLoadoutToInMatch")))
	{
		return false;
	}

	EnsurePersistenceProvider();
	if (!PersistenceProvider)
	{
		return false;
	}

	FYcMetaInventoryRootSnapshot Snapshot;
	if (!PersistenceProvider->LoadSnapshot(AccountId, Snapshot))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("LoadPlayerLoadoutToInMatch: snapshot not found for account=%s"), *AccountId);
		return false;
	}
	if (!YcMetaInventoryVersion::IsSupportedVersion(Snapshot.SnapshotVersion))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("LoadPlayerLoadoutToInMatch: unsupported snapshot version=%d"), Snapshot.SnapshotVersion);
		return false;
	}

	return ApplyPlayerSnapshot(ContextOwner, InMatchPlayerInventory, Snapshot.Player);
}

bool UYcMetaInventorySubsystem::CommitInMatchPlayerLoadoutToProfile(const FString& AccountId, AActor* ContextOwner, UYcInventoryManagerComponent* InMatchPlayerInventory, const bool bExtractionSucceeded)
{
	if (!ValidateInMatchLoadoutRequest(AccountId, ContextOwner, InMatchPlayerInventory, bExtractionSucceeded, TEXT("CommitInMatchPlayerLoadoutToProfile")))
	{
		return false;
	}

	EnsurePersistenceProvider();
	if (!PersistenceProvider)
	{
		return false;
	}

	FYcMetaInventoryRootSnapshot Snapshot;
	if (!PersistenceProvider->LoadSnapshot(AccountId, Snapshot) || !YcMetaInventoryVersion::IsSupportedVersion(Snapshot.SnapshotVersion))
	{
		Snapshot = YcMetaInventoryVersion::MakeEmptySnapshot(AccountId);
	}

	if (!bExtractionSucceeded)
	{
		// 撤离失败：清空玩家侧持久化负载，仓库(Stash)保持不变。
		Snapshot.Player = FYcMetaPlayerSnapshot();
		YcMetaInventoryVersion::PrepareSnapshotForSave(AccountId, Snapshot);
		return PersistenceProvider->SaveSnapshot(AccountId, Snapshot);
	}

	FYcMetaPlayerSnapshot PlayerSnapshot;
	if (!BuildPlayerSnapshot(ContextOwner, InMatchPlayerInventory, PlayerSnapshot))
	{
		return false;
	}

	Snapshot.Player = MoveTemp(PlayerSnapshot);
	YcMetaInventoryVersion::PrepareSnapshotForSave(AccountId, Snapshot);
	return PersistenceProvider->SaveSnapshot(AccountId, Snapshot);
}

bool UYcMetaInventorySubsystem::IsProfileDirty(const FString& AccountId) const
{
	return DirtyProfiles.Contains(AccountId);
}

void UYcMetaInventorySubsystem::MarkProfileDirty(const FString& AccountId)
{
	if (!AccountId.IsEmpty())
	{
		DirtyProfiles.Add(AccountId);
	}
}

void UYcMetaInventorySubsystem::ClearProfileDirty(const FString& AccountId)
{
	if (!AccountId.IsEmpty())
	{
		DirtyProfiles.Remove(AccountId);
	}
}

bool UYcMetaInventorySubsystem::BuildSnapshotFromContext(UYcInventorySceneContext* Context, FYcMetaInventoryRootSnapshot& OutSnapshot) const
{
	if (!ValidateOutOfMatchContext(Context, TEXT("BuildSnapshotFromContext")))
	{
		return false;
	}

	OutSnapshot = YcMetaInventoryVersion::MakeEmptySnapshot(Context->AccountId);

	if (!BuildPlayerSnapshot(Context->ContextOwner, Context->PlayerInventoryRef, OutSnapshot.Player))
	{
		return false;
	}
	if (!BuildInventoryRecords(Context->ContainerInventoryRef, OutSnapshot.Stash.InventoryItems, OutSnapshot.Stash.InventoryGridPlacements))
	{
		return false;
	}

	return true;
}

bool UYcMetaInventorySubsystem::ApplySnapshotToContext(UYcInventorySceneContext* Context, const FYcMetaInventoryRootSnapshot& Snapshot)
{
	if (!ValidateOutOfMatchContext(Context, TEXT("ApplySnapshotToContext")))
	{
		return false;
	}

	if (!ApplyPlayerSnapshot(Context->ContextOwner, Context->PlayerInventoryRef, Snapshot.Player))
	{
		return false;
	}

	TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>> StashItemMap;
	if (!RestoreInventoryRecords(Context->ContainerInventoryRef, Snapshot.Stash.InventoryItems, Snapshot.Stash.InventoryGridPlacements, StashItemMap))
	{
		return false;
	}

	return true;
}

bool UYcMetaInventorySubsystem::BuildPlayerSnapshot(const AActor* ContextOwner, UYcInventoryManagerComponent* PlayerInventory, FYcMetaPlayerSnapshot& OutPlayerSnapshot) const
{
	if (!IsValid(PlayerInventory))
	{
		return false;
	}

	OutPlayerSnapshot = FYcMetaPlayerSnapshot();
	if (!BuildInventoryRecords(PlayerInventory, OutPlayerSnapshot.InventoryItems, OutPlayerSnapshot.InventoryGridPlacements))
	{
		return false;
	}

	BuildEquipmentRecords(ContextOwner, PlayerInventory, nullptr, OutPlayerSnapshot.EquipmentSlots);
	BuildQuickBarRecords(ContextOwner, PlayerInventory, nullptr, OutPlayerSnapshot.QuickBarSlots);

	// 兜底：装备槽/快捷栏中可能存在不在PlayerInventory中的托管物品。
	TSet<FYcItemInstanceId> PlayerItemIds;
	for (const FYcMetaInventoryItemRecord& Record : OutPlayerSnapshot.InventoryItems)
	{
		if (Record.ItemInstId.IsValid())
		{
			PlayerItemIds.Add(Record.ItemInstId);
		}
	}

	auto MakeUniquePlayerItemId = [&](const FYcItemInstanceId& BaseId) -> FYcItemInstanceId
	{
		(void)BaseId;
		for (int32 Suffix = 0; Suffix < 1000; ++Suffix)
		{
			const FYcItemInstanceId Candidate = FYcItemInstanceId::NewId();
			if (!PlayerItemIds.Contains(Candidate))
			{
				return Candidate;
			}
		}
		return FYcItemInstanceId();
	};

	auto TryAddDetachedPlayerOwnedItem = [&](UYcInventoryItemInstance* ItemInstance, const TFunction<void(const FYcItemInstanceId&, const FYcItemInstanceId&)>& OnRemapSlotRef)
	{
		if (!IsValid(ItemInstance))
		{
			return;
		}

		const FYcItemInstanceId RawItemInstId = ItemInstance->GetItemInstId();
		if (!RawItemInstId.IsValid())
		{
			return;
		}

		UYcInventoryManagerComponent* ItemOwnerInventory = UYcInventoryManagerComponent::FindInventoryManagerByItem(ItemInstance);
		if (ItemOwnerInventory && ItemOwnerInventory != PlayerInventory)
		{
			return;
		}

		FYcItemInstanceId SavedItemInstId = RawItemInstId;
		if (PlayerItemIds.Contains(SavedItemInstId))
		{
			SavedItemInstId = MakeUniquePlayerItemId(RawItemInstId);
			OnRemapSlotRef(RawItemInstId, SavedItemInstId);
		}

		FYcMetaInventoryItemRecord NewRecord;
		NewRecord.ItemInstId = SavedItemInstId;
		NewRecord.ItemRegistryId = ItemInstance->GetItemRegistryId();
		NewRecord.StackCount = 1;
		const TArray<FYcMetaItemExtensionPayload>* UnknownPayloads = UnknownItemExtensionPayloads.Find(RawItemInstId);
		YcMetaInventoryItemRecordCodec::ExportFromItem(*ItemInstance, UnknownPayloads, NewRecord);
		OutPlayerSnapshot.InventoryItems.Add(NewRecord);
		PlayerItemIds.Add(SavedItemInstId);
	};

	if (UActorComponent* EquipmentComp = FindComponentAcrossOwnerChain(ContextOwner, EquipmentSlotComponentClassName))
	{
		if (UFunction* GetOccupiedSlotsFn = EquipmentComp->FindFunction(Fn_GetOccupiedSlots))
		{
			if (UFunction* GetItemInSlotFn = EquipmentComp->FindFunction(Fn_GetItemInSlot))
			{
				struct FGetOccupiedSlotsParams { TArray<FGameplayTag> ReturnValue; };
				struct FGetItemInSlotParams { FGameplayTag SlotTag; UYcInventoryItemInstance* ReturnValue; };

				FGetOccupiedSlotsParams OccupiedParams;
				EquipmentComp->ProcessEvent(GetOccupiedSlotsFn, &OccupiedParams);
				for (const FGameplayTag& SlotTag : OccupiedParams.ReturnValue)
				{
					FGetItemInSlotParams ItemParams;
					ItemParams.SlotTag = SlotTag;
					ItemParams.ReturnValue = nullptr;
					EquipmentComp->ProcessEvent(GetItemInSlotFn, &ItemParams);
					const FGameplayTag LocalSlotTag = SlotTag;
					TryAddDetachedPlayerOwnedItem(ItemParams.ReturnValue,
						[&](const FYcItemInstanceId& OldId, const FYcItemInstanceId& NewId)
						{
							for (FYcMetaEquipmentSlotRecord& SlotRecord : OutPlayerSnapshot.EquipmentSlots)
							{
								if (SlotRecord.SlotTag == LocalSlotTag && SlotRecord.ItemInstId == OldId)
								{
									SlotRecord.ItemInstId = NewId;
								}
							}
						});
				}
			}
		}
	}

	if (UActorComponent* QuickBarComp = FindComponentAcrossOwnerChain(ContextOwner, QuickBarComponentClassName))
	{
		if (UFunction* GetSlotsFn = QuickBarComp->FindFunction(Fn_GetSlots))
		{
			struct FGetQuickBarSlotsParams { TArray<TObjectPtr<UYcInventoryItemInstance>> ReturnValue; };

			FGetQuickBarSlotsParams SlotsParams;
			QuickBarComp->ProcessEvent(GetSlotsFn, &SlotsParams);
			for (int32 SlotIndex = 0; SlotIndex < SlotsParams.ReturnValue.Num(); ++SlotIndex)
			{
				UYcInventoryItemInstance* SlotItem = SlotsParams.ReturnValue[SlotIndex];
				const int32 LocalSlotIndex = SlotIndex;
				TryAddDetachedPlayerOwnedItem(SlotItem,
					[&](const FYcItemInstanceId& OldId, const FYcItemInstanceId& NewId)
					{
						for (FYcMetaQuickBarSlotRecord& SlotRecord : OutPlayerSnapshot.QuickBarSlots)
						{
							if (SlotRecord.SlotIndex == LocalSlotIndex && SlotRecord.ItemInstId == OldId)
							{
								SlotRecord.ItemInstId = NewId;
							}
						}
					});
			}
		}
	}

	OutPlayerSnapshot.InventoryItems.Sort([](const FYcMetaInventoryItemRecord& A, const FYcMetaInventoryItemRecord& B)
	{
		return A.ItemInstId.ToString() < B.ItemInstId.ToString();
	});

	return true;
}

bool UYcMetaInventorySubsystem::ApplyPlayerSnapshot(const AActor* ContextOwner, UYcInventoryManagerComponent* PlayerInventory, const FYcMetaPlayerSnapshot& PlayerSnapshot)
{
	if (!IsValid(PlayerInventory))
	{
		return false;
	}

	ClearEquipment(ContextOwner);
	ClearQuickBar(ContextOwner);

	TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>> PlayerItemMap;
	if (!RestoreInventoryRecords(PlayerInventory, PlayerSnapshot.InventoryItems, PlayerSnapshot.InventoryGridPlacements, PlayerItemMap))
	{
		return false;
	}

	TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>> EmptyStashMap;
	RestoreEquipment(ContextOwner, PlayerSnapshot.EquipmentSlots, PlayerItemMap, EmptyStashMap);
	RestoreQuickBar(ContextOwner, PlayerSnapshot.QuickBarSlots, PlayerItemMap, EmptyStashMap);

	// 强制广播一次当前槽位状态，确保跨场景持久化UI能立即刷新图标。
	if (UActorComponent* EquipmentComp = FindComponentAcrossOwnerChain(ContextOwner, EquipmentSlotComponentClassName))
	{
		if (UFunction* OnRepSlotsFn = EquipmentComp->FindFunction(Fn_OnRep_Slots))
		{
			EquipmentComp->ProcessEvent(OnRepSlotsFn, nullptr);
		}
	}
	if (UActorComponent* QuickBarComp = FindComponentAcrossOwnerChain(ContextOwner, QuickBarComponentClassName))
	{
		if (UFunction* OnRepSlotsFn = QuickBarComp->FindFunction(Fn_OnRep_Slots))
		{
			QuickBarComp->ProcessEvent(OnRepSlotsFn, nullptr);
		}
	}

	return true;
}

void UYcMetaInventorySubsystem::OnOperationStateChanged(FGameplayTag ActualTag, const FYcInventoryOperationStateMessage& Message)
{
	(void)ActualTag;

	if (Message.Event != EYcInventoryOperationEvent::Acked)
	{
		return;
	}

	for (int32 Index = SceneContexts.Num() - 1; Index >= 0; --Index)
	{
		UYcInventorySceneContext* Context = SceneContexts[Index];
		if (!IsValid(Context))
		{
			SceneContexts.RemoveAtSwap(Index);
			continue;
		}

		if (!Context->IsValidForOutOfMatchPersistence())
		{
			continue;
		}

		const bool bTouchesPlayer = (Message.Operation.SourceInventory == Context->PlayerInventoryRef || Message.Operation.TargetInventory == Context->PlayerInventoryRef);
		const bool bTouchesContainer = (Message.Operation.SourceInventory == Context->ContainerInventoryRef || Message.Operation.TargetInventory == Context->ContainerInventoryRef);
		if (bTouchesPlayer || bTouchesContainer)
		{
			MarkProfileDirty(Context->AccountId);
		}
	}
}

void UYcMetaInventorySubsystem::EnsurePersistenceProvider()
{
	if (!PersistenceProvider)
	{
		PersistenceProvider = NewObject<UYcInventoryPersistenceProvider_LocalSave>(this);
	}
}

bool UYcMetaInventorySubsystem::BuildInventoryRecords(UYcInventoryManagerComponent* Inventory, TArray<FYcMetaInventoryItemRecord>& OutItems, TArray<FYcMetaGridPlacementRecord>& OutPlacements) const
{
	if (!IsValid(Inventory))
	{
		return false;
	}

	OutItems.Empty();
	OutPlacements.Empty();
	const TArray<UYcInventoryItemInstance*> Items = Inventory->GetAllItemInstance();
	for (UYcInventoryItemInstance* Item : Items)
	{
		if (!IsValid(Item))
		{
			continue;
		}

		FYcMetaInventoryItemRecord Record;
		Record.ItemInstId = Item->GetItemInstId();
		Record.ItemRegistryId = Item->GetItemRegistryId();
		Record.StackCount = Inventory->GetStackCountByItemInstance(Item);

		const TArray<FYcMetaItemExtensionPayload>* UnknownPayloads = UnknownItemExtensionPayloads.Find(Record.ItemInstId);
		YcMetaInventoryItemRecordCodec::ExportFromItem(*Item, UnknownPayloads, Record);
		OutItems.Add(Record);
	}

	OutItems.Sort([](const FYcMetaInventoryItemRecord& A, const FYcMetaInventoryItemRecord& B)
	{
		return A.ItemInstId.ToString() < B.ItemInstId.ToString();
	});

	UFunction* TileMapFn = Inventory->FindFunction(Fn_GetGridItemsTileMap);
	if (TileMapFn)
	{
		struct FGetGridItemsTileMapParams
		{
			TMap<TObjectPtr<UYcInventoryItemInstance>, FIntPoint> ReturnValue;
		};
		FGetGridItemsTileMapParams TileMapParams;
		Inventory->ProcessEvent(TileMapFn, &TileMapParams);

		TMap<TObjectPtr<UYcInventoryItemInstance>, bool> RotationMap;
		UFunction* RotationMapFn = Inventory->FindFunction(Fn_GetGridItemRotationMap);
		if (RotationMapFn)
		{
			struct FGetGridItemRotationMapParams
			{
				TMap<TObjectPtr<UYcInventoryItemInstance>, bool> ReturnValue;
			};
			FGetGridItemRotationMapParams RotationMapParams;
			Inventory->ProcessEvent(RotationMapFn, &RotationMapParams);
			RotationMap = MoveTemp(RotationMapParams.ReturnValue);
		}

		for (const TPair<TObjectPtr<UYcInventoryItemInstance>, FIntPoint>& Pair : TileMapParams.ReturnValue)
		{
			if (!IsValid(Pair.Key))
			{
				continue;
			}

			FYcMetaGridPlacementRecord Placement;
			Placement.ItemInstId = Pair.Key->GetItemInstId();
			Placement.GridTile = Pair.Value;
			if (const bool* FoundRotated = RotationMap.Find(Pair.Key))
			{
				Placement.bRotated = *FoundRotated;
			}
			OutPlacements.Add(Placement);
		}

		OutPlacements.Sort([](const FYcMetaGridPlacementRecord& A, const FYcMetaGridPlacementRecord& B)
		{
			return A.ItemInstId.ToString() < B.ItemInstId.ToString();
		});
	}

	return true;
}

bool UYcMetaInventorySubsystem::RestoreInventoryRecords(UYcInventoryManagerComponent* Inventory, const TArray<FYcMetaInventoryItemRecord>& InItems, const TArray<FYcMetaGridPlacementRecord>& InPlacements, TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& OutItemMap)
{
	if (!IsValid(Inventory))
	{
		return false;
	}

	OutItemMap.Empty();

	const TArray<UYcInventoryItemInstance*> Existing = Inventory->GetAllItemInstance();
	for (UYcInventoryItemInstance* Item : Existing)
	{
		if (IsValid(Item))
		{
			UnknownItemExtensionPayloads.Remove(Item->GetItemInstId());
			Inventory->RemoveItemInstance(Item);
		}
	}

	for (const FYcMetaInventoryItemRecord& Record : InItems)
	{
		if (!Record.ItemRegistryId.IsValid() || Record.StackCount <= 0)
		{
			continue;
		}

		UYcInventoryItemInstance* CreatedItem = Inventory->AddItemWithInstanceId(Record.ItemRegistryId, Record.ItemInstId, Record.StackCount);
		if (!CreatedItem)
		{
			UE_LOG(LogYcInventory, Warning, TEXT("RestoreInventoryRecords: failed add %s (%s)."), *Record.ItemInstId.ToString(), *Record.ItemRegistryId.ToString());
			continue;
		}

		TArray<FYcMetaItemExtensionPayload> UnknownPayloads;
		YcMetaInventoryItemRecordCodec::ImportToItem(*CreatedItem, Record, UnknownPayloads);
		if (UnknownPayloads.IsEmpty())
		{
			UnknownItemExtensionPayloads.Remove(Record.ItemInstId);
		}
		else
		{
			UnknownItemExtensionPayloads.Add(Record.ItemInstId, MoveTemp(UnknownPayloads));
		}

		OutItemMap.Add(Record.ItemInstId, CreatedItem);
	}

	if (!InPlacements.IsEmpty())
	{
		UFunction* RemoveGridFn = Inventory->FindFunction(Fn_OnRemoveGridItem);
		UFunction* AddGridFn = Inventory->FindFunction(Fn_OnGridItemInstanceAdded);
		if (RemoveGridFn && AddGridFn)
		{
			struct FRemoveGridParams
			{
				UYcInventoryItemInstance* ItemInst;
			};

			for (const TPair<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& Pair : OutItemMap)
			{
				if (!IsValid(Pair.Value))
				{
					continue;
				}
				FRemoveGridParams RemoveParams;
				RemoveParams.ItemInst = Pair.Value;
				Inventory->ProcessEvent(RemoveGridFn, &RemoveParams);
			}

			struct FAddGridParams
			{
				UYcInventoryItemInstance* ItemInst;
				int32 StackCount;
				FIntPoint Tile;
				bool bRotated;
				bool ReturnValue;
			};

			UFunction* FindFirstFitFn = Inventory->FindFunction(Fn_FindFirstFitPosition);
			struct FFindFirstFitParams
			{
				FDataRegistryId ItemDefId;
				FIntPoint Tile;
				bool OutRotated;
				bool ReturnValue;
			};

			for (const FYcMetaGridPlacementRecord& Placement : InPlacements)
			{
				const TObjectPtr<UYcInventoryItemInstance>* FoundItem = OutItemMap.Find(Placement.ItemInstId);
				if (!FoundItem || !IsValid(*FoundItem))
				{
					continue;
				}

				FAddGridParams AddParams;
				AddParams.ItemInst = *FoundItem;
				AddParams.StackCount = FMath::Max(1, Inventory->GetStackCountByItemInstance(*FoundItem));
				AddParams.Tile = Placement.GridTile;
				AddParams.bRotated = Placement.bRotated;
				AddParams.ReturnValue = false;
				Inventory->ProcessEvent(AddGridFn, &AddParams);

				if (!AddParams.ReturnValue && FindFirstFitFn)
				{
					FFindFirstFitParams FitParams;
					FitParams.ItemDefId = (*FoundItem)->GetItemRegistryId();
					FitParams.Tile = FIntPoint::ZeroValue;
					FitParams.OutRotated = false;
					FitParams.ReturnValue = false;
					Inventory->ProcessEvent(FindFirstFitFn, &FitParams);

					if (FitParams.ReturnValue)
					{
						FAddGridParams FallbackAddParams;
						FallbackAddParams.ItemInst = *FoundItem;
						FallbackAddParams.StackCount = FMath::Max(1, Inventory->GetStackCountByItemInstance(*FoundItem));
						FallbackAddParams.Tile = FitParams.Tile;
						FallbackAddParams.bRotated = FitParams.OutRotated;
						FallbackAddParams.ReturnValue = false;
						Inventory->ProcessEvent(AddGridFn, &FallbackAddParams);
					}
				}
			}
		}
	}

	return true;
}

bool UYcMetaInventorySubsystem::BuildEquipmentRecords(const AActor* ContextOwner, UYcInventoryManagerComponent* PlayerInventory, UYcInventoryManagerComponent* StashInventory, TArray<FYcMetaEquipmentSlotRecord>& OutSlots) const
{
	OutSlots.Empty();
	UActorComponent* EquipmentComp = FindComponentAcrossOwnerChain(ContextOwner, EquipmentSlotComponentClassName);
	if (!EquipmentComp)
	{
		return false;
	}

	UFunction* GetOccupiedSlotsFn = EquipmentComp->FindFunction(Fn_GetOccupiedSlots);
	UFunction* GetItemInSlotFn = EquipmentComp->FindFunction(Fn_GetItemInSlot);
	if (!GetOccupiedSlotsFn || !GetItemInSlotFn)
	{
		return false;
	}

	struct FGetOccupiedSlotsParams
	{
		TArray<FGameplayTag> ReturnValue;
	};

	FGetOccupiedSlotsParams OccupiedParams;
	EquipmentComp->ProcessEvent(GetOccupiedSlotsFn, &OccupiedParams);

	struct FGetItemInSlotParams
	{
		FGameplayTag SlotTag;
		UYcInventoryItemInstance* ReturnValue;
	};

	for (const FGameplayTag& SlotTag : OccupiedParams.ReturnValue)
	{
		FGetItemInSlotParams ItemParams;
		ItemParams.SlotTag = SlotTag;
		ItemParams.ReturnValue = nullptr;
		EquipmentComp->ProcessEvent(GetItemInSlotFn, &ItemParams);

		if (!IsValid(ItemParams.ReturnValue))
		{
			continue;
		}
		FYcMetaEquipmentSlotRecord SlotRecord;
		SlotRecord.SlotTag = SlotTag;
		SlotRecord.ItemInstId = ItemParams.ReturnValue->GetItemInstId();
		const UYcInventoryManagerComponent* ItemOwnerInventory = UYcInventoryManagerComponent::FindInventoryManagerByItem(ItemParams.ReturnValue);
		if (ItemOwnerInventory == StashInventory)
		{
			SlotRecord.SourceScope = EYcMetaItemSourceScope::StashInventory;
		}
		else if (ItemOwnerInventory == nullptr || ItemOwnerInventory == PlayerInventory)
		{
			SlotRecord.SourceScope = EYcMetaItemSourceScope::PlayerInventory;
		}
		else
		{
			SlotRecord.SourceScope = EYcMetaItemSourceScope::Unknown;
		}
		OutSlots.Add(SlotRecord);
	}

	return true;
}

bool UYcMetaInventorySubsystem::BuildQuickBarRecords(const AActor* ContextOwner, UYcInventoryManagerComponent* PlayerInventory, UYcInventoryManagerComponent* StashInventory, TArray<FYcMetaQuickBarSlotRecord>& OutSlots) const
{
	OutSlots.Empty();
	UActorComponent* QuickBarComp = FindComponentAcrossOwnerChain(ContextOwner, QuickBarComponentClassName);
	if (!QuickBarComp)
	{
		return false;
	}

	UFunction* GetSlotsFn = QuickBarComp->FindFunction(Fn_GetSlots);
	if (!GetSlotsFn)
	{
		return false;
	}

	struct FGetQuickBarSlotsParams
	{
		TArray<TObjectPtr<UYcInventoryItemInstance>> ReturnValue;
	};

	FGetQuickBarSlotsParams SlotsParams;
	QuickBarComp->ProcessEvent(GetSlotsFn, &SlotsParams);

	for (int32 SlotIndex = 0; SlotIndex < SlotsParams.ReturnValue.Num(); ++SlotIndex)
	{
		UYcInventoryItemInstance* SlotItem = SlotsParams.ReturnValue[SlotIndex];
		if (!IsValid(SlotItem))
		{
			continue;
		}
		FYcMetaQuickBarSlotRecord SlotRecord;
		SlotRecord.SlotIndex = SlotIndex;
		SlotRecord.ItemInstId = SlotItem->GetItemInstId();
		const UYcInventoryManagerComponent* ItemOwnerInventory = UYcInventoryManagerComponent::FindInventoryManagerByItem(SlotItem);
		if (ItemOwnerInventory == StashInventory)
		{
			SlotRecord.SourceScope = EYcMetaItemSourceScope::StashInventory;
		}
		else if (ItemOwnerInventory == nullptr || ItemOwnerInventory == PlayerInventory)
		{
			SlotRecord.SourceScope = EYcMetaItemSourceScope::PlayerInventory;
		}
		else
		{
			SlotRecord.SourceScope = EYcMetaItemSourceScope::Unknown;
		}
		OutSlots.Add(SlotRecord);
	}

	return true;
}

void UYcMetaInventorySubsystem::ClearEquipment(const AActor* ContextOwner) const
{
	UActorComponent* EquipmentComp = FindComponentAcrossOwnerChain(ContextOwner, EquipmentSlotComponentClassName);
	if (!EquipmentComp)
	{
		return;
	}

	UFunction* GetOccupiedSlotsFn = EquipmentComp->FindFunction(Fn_GetOccupiedSlots);
	UFunction* UnequipFn = EquipmentComp->FindFunction(Fn_UnequipSlot);
	if (!GetOccupiedSlotsFn || !UnequipFn)
	{
		return;
	}

	struct FGetOccupiedSlotsParams
	{
		TArray<FGameplayTag> ReturnValue;
	};
	FGetOccupiedSlotsParams OccupiedParams;
	EquipmentComp->ProcessEvent(GetOccupiedSlotsFn, &OccupiedParams);

	struct FUnequipParams
	{
		FGameplayTag SlotTag;
		UYcInventoryItemInstance* ReturnValue;
	};

	for (const FGameplayTag& SlotTag : OccupiedParams.ReturnValue)
	{
		FUnequipParams UnequipParams;
		UnequipParams.SlotTag = SlotTag;
		UnequipParams.ReturnValue = nullptr;
		EquipmentComp->ProcessEvent(UnequipFn, &UnequipParams);
	}
}

void UYcMetaInventorySubsystem::ClearQuickBar(const AActor* ContextOwner) const
{
	UActorComponent* QuickBarComp = FindComponentAcrossOwnerChain(ContextOwner, QuickBarComponentClassName);
	if (!QuickBarComp)
	{
		return;
	}

	UFunction* GetSlotsFn = QuickBarComp->FindFunction(Fn_GetSlots);
	UFunction* RemoveFn = QuickBarComp->FindFunction(Fn_RemoveItemFromSlot);
	if (!GetSlotsFn || !RemoveFn)
	{
		return;
	}

	struct FGetQuickBarSlotsParams
	{
		TArray<TObjectPtr<UYcInventoryItemInstance>> ReturnValue;
	};
	FGetQuickBarSlotsParams SlotsParams;
	QuickBarComp->ProcessEvent(GetSlotsFn, &SlotsParams);

	struct FRemoveParams
	{
		int32 SlotIndex;
		UYcInventoryItemInstance* ReturnValue;
	};

	for (int32 SlotIndex = 0; SlotIndex < SlotsParams.ReturnValue.Num(); ++SlotIndex)
	{
		if (SlotsParams.ReturnValue[SlotIndex] == nullptr)
		{
			continue;
		}

		FRemoveParams RemoveParams;
		RemoveParams.SlotIndex = SlotIndex;
		RemoveParams.ReturnValue = nullptr;
		QuickBarComp->ProcessEvent(RemoveFn, &RemoveParams);
	}
}

void UYcMetaInventorySubsystem::RestoreEquipment(const AActor* ContextOwner, const TArray<FYcMetaEquipmentSlotRecord>& InSlots, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& PlayerItemMap, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& StashItemMap) const
{
	UActorComponent* EquipmentComp = FindComponentAcrossOwnerChain(ContextOwner, EquipmentSlotComponentClassName);
	if (!EquipmentComp)
	{
		return;
	}

	UFunction* EquipFn = EquipmentComp->FindFunction(Fn_EquipItem);
	if (!EquipFn)
	{
		return;
	}

	struct FEquipParams
	{
		UYcInventoryItemInstance* ItemInstance;
		bool ReturnValue;
	};

	for (const FYcMetaEquipmentSlotRecord& Slot : InSlots)
	{
		const TObjectPtr<UYcInventoryItemInstance>* FoundItem = nullptr;
		switch (Slot.SourceScope)
		{
		case EYcMetaItemSourceScope::PlayerInventory:
			FoundItem = PlayerItemMap.Find(Slot.ItemInstId);
			break;
		case EYcMetaItemSourceScope::StashInventory:
			FoundItem = StashItemMap.Find(Slot.ItemInstId);
			break;
		case EYcMetaItemSourceScope::Unknown:
		default:
			FoundItem = PlayerItemMap.Find(Slot.ItemInstId);
			if (!FoundItem)
			{
				FoundItem = StashItemMap.Find(Slot.ItemInstId);
			}
			break;
		}
		if (!FoundItem || !IsValid(*FoundItem))
		{
			continue;
		}

		FEquipParams EquipParams;
		EquipParams.ItemInstance = *FoundItem;
		EquipParams.ReturnValue = false;
		EquipmentComp->ProcessEvent(EquipFn, &EquipParams);
	}
}

void UYcMetaInventorySubsystem::RestoreQuickBar(const AActor* ContextOwner, const TArray<FYcMetaQuickBarSlotRecord>& InSlots, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& PlayerItemMap, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& StashItemMap) const
{
	UActorComponent* QuickBarComp = FindComponentAcrossOwnerChain(ContextOwner, QuickBarComponentClassName);
	if (!QuickBarComp)
	{
		return;
	}

	UFunction* AddFn = QuickBarComp->FindFunction(Fn_AddItemToSlot);
	if (!AddFn)
	{
		return;
	}

	struct FAddParams
	{
		int32 SlotIndex;
		UYcInventoryItemInstance* Item;
		bool ReturnValue;
	};

	for (const FYcMetaQuickBarSlotRecord& Slot : InSlots)
	{
		const TObjectPtr<UYcInventoryItemInstance>* FoundItem = nullptr;
		switch (Slot.SourceScope)
		{
		case EYcMetaItemSourceScope::PlayerInventory:
			FoundItem = PlayerItemMap.Find(Slot.ItemInstId);
			break;
		case EYcMetaItemSourceScope::StashInventory:
			FoundItem = StashItemMap.Find(Slot.ItemInstId);
			break;
		case EYcMetaItemSourceScope::Unknown:
		default:
			FoundItem = PlayerItemMap.Find(Slot.ItemInstId);
			if (!FoundItem)
			{
				FoundItem = StashItemMap.Find(Slot.ItemInstId);
			}
			break;
		}
		if (!FoundItem || !IsValid(*FoundItem))
		{
			continue;
		}

		FAddParams Params;
		Params.SlotIndex = Slot.SlotIndex;
		Params.Item = *FoundItem;
		Params.ReturnValue = false;
		QuickBarComp->ProcessEvent(AddFn, &Params);
	}
}

UActorComponent* UYcMetaInventorySubsystem::FindComponentAcrossOwnerChain(const AActor* Owner, const FName ClassName)
{
	if (!Owner)
	{
		return nullptr;
	}

	auto FindByName = [ClassName](const AActor* Actor) -> UActorComponent*
	{
		if (!Actor)
		{
			return nullptr;
		}

		TInlineComponentArray<UActorComponent*> Components;
		Actor->GetComponents(Components);
		for (UActorComponent* Component : Components)
		{
			if (Component && Component->GetClass() && Component->GetClass()->GetFName() == ClassName)
			{
				return Component;
			}
		}
		return nullptr;
	};

	if (UActorComponent* Found = FindByName(Owner))
	{
		return Found;
	}

	if (const APawn* Pawn = Cast<APawn>(Owner))
	{
		if (UActorComponent* Found = FindByName(Pawn->GetController()))
		{
			return Found;
		}
		if (UActorComponent* Found = FindByName(Pawn->GetPlayerState()))
		{
			return Found;
		}
	}

	if (const AController* Controller = Cast<AController>(Owner))
	{
		if (UActorComponent* Found = FindByName(Controller->GetPawn()))
		{
			return Found;
		}
		if (UActorComponent* Found = FindByName(Controller->PlayerState))
		{
			return Found;
		}
	}

	if (const APlayerState* PlayerState = Cast<APlayerState>(Owner))
	{
		if (UActorComponent* Found = FindByName(PlayerState->GetPawn()))
		{
			return Found;
		}
		if (UActorComponent* Found = FindByName(Cast<AController>(PlayerState->GetOwner())))
		{
			return Found;
		}
	}

	return nullptr;
}
