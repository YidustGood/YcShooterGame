// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "GridInventoryManagerComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "ContextAction/YcGridInventoryContextActionLibrary.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "YcGridInventoryPlacementRuleLibrary.h"
#include "YcInventoryItemInstance.h"
#include "YcInventoryLibrary.h"
#include "YcInventoryOperationRouterComponent.h"
#include "YcEquipmentSlotComponent.h"
#include "NativeGameplayTags.h"

DEFINE_LOG_CATEGORY_STATIC(LogYcGridInventory, Log, All);

namespace
{
	static const FName OpType_Inventory_SwapGrid(TEXT("Inventory.SwapGrid"));
	static const FName OpTypePrefix_Container(TEXT("Container."));
	static const FName OpTypePrefix_Search(TEXT("Search."));

	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Yc_Inventory_Message_StackChanged, "Yc.Inventory.Message.StackChanged");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Yc_EquipmentSlot_Message_SlotChanged, "Yc.EquipmentSlot.Message.SlotChanged");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Inventory_Region_Main, "Inventory.Region.Main");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Yc_Inventory_Message_Grid_ContextAction_Request, "Yc.Inventory.Message.Grid.ContextAction.Request");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Yc_Inventory_ContextAction_Executor_Message, "Yc.Inventory.ContextAction.Executor.Message");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Yc_Inventory_ContextAction_Executor_GAS, "Yc.Inventory.ContextAction.Executor.GAS");
}

void FGridInventorySlot::Reset()
{
	bOccupied = false;
	OccupyingItemID = FYcItemInstanceId();
	ItemRelativeX = 0;
	ItemRelativeY = 0;
	ItemInstance = nullptr;
}

FItemGridInfo::FItemGridInfo(const FIntPoint InTilePos, const FIntPoint InItemSize)
	: TilePos(InTilePos)
	, ItemSize(InItemSize)
{
}

UGridInventoryManagerComponent::UGridInventoryManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UGridInventoryManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UGridInventoryManagerComponent, bEnableContainerSearch);
	DOREPLIFETIME(UGridInventoryManagerComponent, InventorySlots);
	DOREPLIFETIME(UGridInventoryManagerComponent, InventoryGridRevision);
	DOREPLIFETIME(UGridInventoryManagerComponent, RegionStates);
	DOREPLIFETIME(UGridInventoryManagerComponent, bSearchSessionActive);
	DOREPLIFETIME(UGridInventoryManagerComponent, SearchContainerInventory);
	DOREPLIFETIME(UGridInventoryManagerComponent, RevealedSearchItems);
	DOREPLIFETIME(UGridInventoryManagerComponent, KnownSearchItems);
	DOREPLIFETIME(UGridInventoryManagerComponent, CurrentSearchingItem);
	DOREPLIFETIME(UGridInventoryManagerComponent, CurrentSearchProgress01);
	DOREPLIFETIME(UGridInventoryManagerComponent, SearchTotalItemCount);
	DOREPLIFETIME(UGridInventoryManagerComponent, SearchRevealedItemCount);
	DOREPLIFETIME(UGridInventoryManagerComponent, SearchSessionRevision);
	DOREPLIFETIME(UGridInventoryManagerComponent, SearchSpeedMultiplier);
}

void UGridInventoryManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// 初始化运行时网格状态。
	InitializeInventory();
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		FYcInventoryOperationHandler SwapHandler;
		SwapHandler.Validate.BindUObject(this, &ThisClass::ValidateSwapLikeOperation);
		SwapHandler.Execute.BindUObject(this, &ThisClass::ExecuteSwapLikeOperation);
		SwapHandler.BuildDelta.BindUObject(this, &ThisClass::BuildSwapLikeOperationDelta);

		FYcInventoryOperationHandler PrefixHandler = SwapHandler;
		PrefixHandler.bPrefixMatch = true;
		PrefixHandler.Priority = -1;

		// 注册网格相关操作处理器。
		if (UYcInventoryOperationRouterComponent* Router = UYcInventoryOperationRouterComponent::FindOrCreateRouter(GetOwner()))
		{
			Router->RegisterOperationHandler(OpType_Inventory_SwapGrid, SwapHandler);
			Router->RegisterOperationHandler(OpTypePrefix_Container, PrefixHandler);
			Router->RegisterOperationHandler(OpTypePrefix_Search, PrefixHandler);
			bOperationHandlersRegistered = true;
		}

		InventoryChangedHandle = MessageSubsystem.RegisterListener(TAG_Yc_Inventory_Message_StackChanged, this, &ThisClass::OnInventoryChanged);
	}

	EquipmentSlotChangedHandle = MessageSubsystem.RegisterListener(TAG_Yc_EquipmentSlot_Message_SlotChanged, this, &ThisClass::OnEquipmentSlotChanged);
}

void UGridInventoryManagerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	InventoryChangedHandle.Unregister();
	EquipmentSlotChangedHandle.Unregister();

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		if (UYcInventoryOperationRouterComponent* Router = UYcInventoryOperationRouterComponent::FindRouter(GetOwner()))
		{
			Router->UnregisterOperationHandler(OpType_Inventory_SwapGrid);
			Router->UnregisterOperationHandler(OpTypePrefix_Container);
			Router->UnregisterOperationHandler(OpTypePrefix_Search);
		}
		bOperationHandlersRegistered = false;
		ResetSearchSession_Internal();
	}

	Super::EndPlay(EndPlayReason);
}

void UGridInventoryManagerComponent::InitializeInventory()
{
	BuildRegionsFromCurrentLoadout();
	InventorySlots.SetNum(FMath::Max(1, InventoryColumns * InventoryRows));
	for (FGridInventorySlot& Slot : InventorySlots)
	{
		Slot.Reset();
	}
	SyncPrimaryRegionToLegacySlots();
}

void UGridInventoryManagerComponent::OnEquipmentSlotChanged(const FGameplayTag ActualTag, const FYcEquipmentSlotChangedMessage& Data)
{
	if (!IsOwnerActorMatched(Data.Owner, GetOwner()))
	{
		return;
	}

	BuildRegionsFromCurrentLoadout();
	SyncPrimaryRegionToLegacySlots();
	++InventoryGridRevision;
	OnInventoryGridChanged.Broadcast();
}

bool UGridInventoryManagerComponent::IsOwnerActorMatched(const AActor* MessageOwner, const AActor* LocalOwner) const
{
	if (!IsValid(MessageOwner) || !IsValid(LocalOwner))
	{
		return false;
	}
	if (MessageOwner == LocalOwner)
	{
		return true;
	}

	const APawn* MessagePawn = Cast<APawn>(MessageOwner);
	const AController* MessageController = Cast<AController>(MessageOwner);
	if (MessageController)
	{
		MessagePawn = MessageController->GetPawn();
	}
	else if (MessagePawn)
	{
		MessageController = Cast<AController>(MessagePawn->GetController());
	}

	const APawn* LocalPawn = Cast<APawn>(LocalOwner);
	const AController* LocalController = Cast<AController>(LocalOwner);
	if (LocalController)
	{
		LocalPawn = LocalController->GetPawn();
	}
	else if (LocalPawn)
	{
		LocalController = Cast<AController>(LocalPawn->GetController());
	}

	return (MessagePawn && LocalPawn && MessagePawn == LocalPawn) || (MessageController && LocalController && MessageController == LocalController);
}

void UGridInventoryManagerComponent::OnInventoryChanged(const FGameplayTag ActualTag, const FYcInventoryItemChangeMessage& Data)
{
	if (Data.InventoryOwner == this)
	{
		if (Data.Delta > 0)
		{
			if (CachedTile.IsSet())
			{
				// 按缓存坐标落位新增物品。
				OnGridItemInstanceAdded(Data.ItemInstance, Data.NewCount, CachedTile.GetValue(), false, CachedRegionId.IsSet() ? CachedRegionId.GetValue() : FGameplayTag(), CachedPocketIndex.IsSet() ? CachedPocketIndex.GetValue() : 0);
				CachedTile.Reset();
				CachedRegionId.Reset();
				CachedPocketIndex.Reset();
				return;
			}

			bool bRotated = false;
			FIntPoint Tile = FIntPoint::ZeroValue;
			FGameplayTag RegionId;
			int32 PocketIndex = 0;
			if (FindFirstFitPlacementForItemInst(Data.ItemInstance, RegionId, PocketIndex, Tile, bRotated))
			{
				OnGridItemInstanceAdded(Data.ItemInstance, Data.NewCount, Tile, bRotated, RegionId, PocketIndex);
			}
			else
			{
				UE_LOG(LogYcGridInventory, Warning, TEXT("OnInventoryChanged add failed: no fit location."));
			}
		}
		else if (Data.NewCount == 0)
		{
			OnRemoveGridItem(Data.ItemInstance);
		}
		return;
	}

	if (bSearchSessionActive && SearchContainerInventory && Data.InventoryOwner == SearchContainerInventory)
	{
		// 并发变更时重建搜索队列。
		RebuildSearchQueueFromContainerPreserveCurrent();
	}
}

bool UGridInventoryManagerComponent::TryAddGridItemByDefinition(const FDataRegistryId ItemDefId, const int32 StackCount, const FIntPoint Tile, const bool bRotated, const FGameplayTag RegionId, const int32 PocketIndex)
{
	if (!CanPlaceGridItem(ItemDefId, Tile, bRotated, RegionId, PocketIndex))
	{
		return false;
	}

	CachedTile = Tile;
	const FGameplayTag FinalRegionId = ResolvePlacementRegionId(RegionId);
	CachedRegionId = FinalRegionId;
	CachedPocketIndex = ResolvePlacementPocketIndex(FinalRegionId, PocketIndex);
	UYcInventoryItemInstance* ItemInstance = AddItem(ItemDefId, StackCount);
	if (!ItemInstance)
	{
		CachedTile.Reset();
		CachedRegionId.Reset();
		CachedPocketIndex.Reset();
		UE_LOG(LogYcGridInventory, Error, TEXT("TryAddGridItemByDefinition failed: AddItem returned nullptr."));
		return false;
	}
	return true;
}

bool UGridInventoryManagerComponent::TryAddGridItemInstance(UYcInventoryItemInstance* ItemInst, const int32 StackCount, const FIntPoint Tile, const bool bRotated, const FGameplayTag RegionId, const int32 PocketIndex)
{
	if (!IsValid(ItemInst))
	{
		return false;
	}

	if (!CanPlaceGridItem(ItemInst->GetItemRegistryId(), Tile, bRotated, RegionId, PocketIndex))
	{
		return false;
	}

	CachedTile = Tile;
	const FGameplayTag FinalRegionId = ResolvePlacementRegionId(RegionId);
	CachedRegionId = FinalRegionId;
	CachedPocketIndex = ResolvePlacementPocketIndex(FinalRegionId, PocketIndex);
	return AddItemInstance(ItemInst, StackCount);
}

bool UGridInventoryManagerComponent::RemoveGridItem(UYcInventoryItemInstance* ItemInst)
{
	const bool bValidRemove = ItemInstanceToTileMap.Contains(ItemInst);
	OnRemoveGridItem(ItemInst);
	RemoveItemInstance(ItemInst);
	return bValidRemove;
}

bool UGridInventoryManagerComponent::OnGridItemInstanceAdded(UYcInventoryItemInstance* ItemInst, const int32 StackCount, const FIntPoint Tile, const bool bRotated, const FGameplayTag RegionId, const int32 PocketIndex)
{
	if (!IsValid(ItemInst))
	{
		return false;
	}

	const FGameplayTag FinalRegionId = ResolvePlacementRegionId(RegionId);
	const int32 FinalPocketIndex = ResolvePlacementPocketIndex(FinalRegionId, PocketIndex);
	if (!CanPlaceGridItemInst(ItemInst, Tile, bRotated, FinalRegionId, FinalPocketIndex))
	{
		return false;
	}

	const FItemFragment_GridItem GridFragment = GetItemFragmentGrid(ItemInst->GetItemRegistryId());
	const int32 ItemWidth = bRotated ? GridFragment.Dimensions.Y : GridFragment.Dimensions.X;
	const int32 ItemHeight = bRotated ? GridFragment.Dimensions.X : GridFragment.Dimensions.Y;

	if (ItemInstanceToTileMap.Contains(ItemInst))
	{
		OnRemoveGridItem(ItemInst);
	}

	FItemGridInfo ItemInfo(Tile, FIntPoint(ItemWidth, ItemHeight));
	ItemInfo.RegionId = FinalRegionId;
	ItemInfo.PocketIndex = FinalPocketIndex;
	ItemInstanceToTileMap.Add(ItemInst, ItemInfo);

	TArray<FGridInventorySlot> RegionSlots = GetRegionSlots(FinalRegionId, FinalPocketIndex);
	const int32 Columns = GetRegionColumns(FinalRegionId, FinalPocketIndex);
	for (int32 Y = Tile.Y; Y < Tile.Y + ItemHeight; ++Y)
	{
		for (int32 X = Tile.X; X < Tile.X + ItemWidth; ++X)
		{
			const int32 Index = TileToIndexInRegion(FIntPoint(X, Y), Columns);
			if (!RegionSlots.IsValidIndex(Index))
			{
				continue;
			}
			RegionSlots[Index].bOccupied = true;
			RegionSlots[Index].OccupyingItemID = ItemInst->GetItemInstId();
			RegionSlots[Index].ItemInstance = ItemInst;
			RegionSlots[Index].ItemRelativeX = X - Tile.X;
			RegionSlots[Index].ItemRelativeY = Y - Tile.Y;
		}
	}
	SetRegionSlots(FinalRegionId, FinalPocketIndex, RegionSlots);
	++InventoryGridRevision;
	SyncPrimaryRegionToLegacySlots();
	OnInventoryGridChanged.Broadcast();
	return true;
}

void UGridInventoryManagerComponent::OnRemoveGridItem(UYcInventoryItemInstance* ItemInst)
{
	FItemGridInfo* ItemGridInfo = ItemInstanceToTileMap.Find(ItemInst);
	if (!ItemGridInfo)
	{
		return;
	}

	TArray<FGridInventorySlot> RegionSlots = GetRegionSlots(ItemGridInfo->RegionId, ItemGridInfo->PocketIndex);
	const int32 Columns = GetRegionColumns(ItemGridInfo->RegionId, ItemGridInfo->PocketIndex);
	for (int32 X = ItemGridInfo->TilePos.X; X < ItemGridInfo->TilePos.X + ItemGridInfo->ItemSize.X; ++X)
	{
		for (int32 Y = ItemGridInfo->TilePos.Y; Y < ItemGridInfo->TilePos.Y + ItemGridInfo->ItemSize.Y; ++Y)
		{
			const int32 Index = TileToIndexInRegion(FIntPoint(X, Y), Columns);
			if (RegionSlots.IsValidIndex(Index))
			{
				RegionSlots[Index].Reset();
			}
		}
	}

	SetRegionSlots(ItemGridInfo->RegionId, ItemGridInfo->PocketIndex, RegionSlots);
	ItemInstanceToTileMap.Remove(ItemInst);
	++InventoryGridRevision;
	SyncPrimaryRegionToLegacySlots();
	OnInventoryGridChanged.Broadcast();
}

bool UGridInventoryManagerComponent::TryGetItemPlacementInfo(UYcInventoryItemInstance* ItemInst, FIntPoint& OutTile, bool& bOutRotated) const
{
	FGameplayTag UnusedRegionId;
	int32 UnusedPocketIndex = -1;
	return TryGetItemPlacementInfoWithRegion(ItemInst, UnusedRegionId, UnusedPocketIndex, OutTile, bOutRotated);
}

bool UGridInventoryManagerComponent::TryGetItemPlacementInfoWithRegion(UYcInventoryItemInstance* ItemInst, FGameplayTag& OutRegionId, int32& OutPocketIndex, FIntPoint& OutTile, bool& bOutRotated) const
{
	OutRegionId = FGameplayTag();
	OutPocketIndex = -1;
	const FItemGridInfo* ItemGridInfo = ItemInstanceToTileMap.Find(ItemInst);
	if (!IsValid(ItemInst) || !ItemGridInfo)
	{
		return false;
	}

	OutRegionId = ItemGridInfo->RegionId;
	OutPocketIndex = ItemGridInfo->PocketIndex;
	OutTile = ItemGridInfo->TilePos;
	const FItemFragment_GridItem GridFragment = GetItemFragmentGrid(ItemInst->GetItemRegistryId());
	bOutRotated = (ItemGridInfo->ItemSize.X == GridFragment.Dimensions.Y && ItemGridInfo->ItemSize.Y == GridFragment.Dimensions.X);
	return true;
}

bool UGridInventoryManagerComponent::IsRegionStateEnabled(const FGameplayTag RegionId, const int32 PocketIndex) const
{
	for (const FGridInventoryRegionRuntimeState& State : RegionStates)
	{
		if (State.bEnabled && State.RegionId == RegionId && State.PocketIndex == PocketIndex)
		{
			return true;
		}
	}
	return false;
}

bool UGridInventoryManagerComponent::CommitPlacementWithoutBroadcast(UYcInventoryItemInstance* ItemInst, const FIntPoint Tile, const bool bRotated, const FGameplayTag RegionId, const int32 PocketIndex)
{
	if (!IsValid(ItemInst) || !CanPlaceGridItemInst(ItemInst, Tile, bRotated, RegionId, PocketIndex))
	{
		return false;
	}

	const FItemFragment_GridItem GridFragment = GetItemFragmentGrid(ItemInst->GetItemRegistryId());
	const int32 ItemWidth = bRotated ? GridFragment.Dimensions.Y : GridFragment.Dimensions.X;
	const int32 ItemHeight = bRotated ? GridFragment.Dimensions.X : GridFragment.Dimensions.Y;
	FItemGridInfo ItemInfo(Tile, FIntPoint(ItemWidth, ItemHeight));
	ItemInfo.RegionId = RegionId;
	ItemInfo.PocketIndex = PocketIndex;
	ItemInstanceToTileMap.Add(ItemInst, ItemInfo);

	TArray<FGridInventorySlot> RegionSlots = GetRegionSlots(RegionId, PocketIndex);
	const int32 Columns = GetRegionColumns(RegionId, PocketIndex);
	for (int32 Y = Tile.Y; Y < Tile.Y + ItemHeight; ++Y)
	{
		for (int32 X = Tile.X; X < Tile.X + ItemWidth; ++X)
		{
			const int32 Index = TileToIndexInRegion(FIntPoint(X, Y), Columns);
			if (!RegionSlots.IsValidIndex(Index))
			{
				continue;
			}
			RegionSlots[Index].bOccupied = true;
			RegionSlots[Index].OccupyingItemID = ItemInst->GetItemInstId();
			RegionSlots[Index].ItemInstance = ItemInst;
			RegionSlots[Index].ItemRelativeX = X - Tile.X;
			RegionSlots[Index].ItemRelativeY = Y - Tile.Y;
		}
	}
	SetRegionSlots(RegionId, PocketIndex, RegionSlots);
	return true;
}

void UGridInventoryManagerComponent::RebuildRegionSlotsFromPlacementMap()
{
	// 清空旧槽位缓存并准备重建。
	RegionSlotsStorage.Empty();

	TArray<TObjectPtr<UYcInventoryItemInstance>> Items;
	ItemInstanceToTileMap.GenerateKeyArray(Items);
	TArray<TObjectPtr<UYcInventoryItemInstance>> ItemsToRemove;
	for (UYcInventoryItemInstance* ItemInst : Items)
	{
		const FItemGridInfo* OldInfo = ItemInstanceToTileMap.Find(ItemInst);
		if (!IsValid(ItemInst) || !OldInfo)
		{
			continue;
		}

		const FItemFragment_GridItem GridFragment = GetItemFragmentGrid(ItemInst->GetItemRegistryId());
		const bool bOldRotated = (OldInfo->ItemSize.X == GridFragment.Dimensions.Y && OldInfo->ItemSize.Y == GridFragment.Dimensions.X);
		bool bPlaced = false;
		if (IsRegionStateEnabled(OldInfo->RegionId, OldInfo->PocketIndex))
		{
			bPlaced = CommitPlacementWithoutBroadcast(ItemInst, OldInfo->TilePos, bOldRotated, OldInfo->RegionId, OldInfo->PocketIndex);
		}

		if (!bPlaced)
		{
			FGameplayTag NewRegionId;
			int32 NewPocketIndex = -1;
			FIntPoint NewTile = FIntPoint::ZeroValue;
			bool bNewRotated = false;
			if (FindFirstFitPlacementForItemInst(ItemInst, NewRegionId, NewPocketIndex, NewTile, bNewRotated))
			{
				bPlaced = CommitPlacementWithoutBroadcast(ItemInst, NewTile, bNewRotated, NewRegionId, NewPocketIndex);
			}
		}

		if (!bPlaced)
		{
			ItemsToRemove.Add(ItemInst);
			UE_LOG(LogYcGridInventory, Warning, TEXT("RebuildRegionSlotsFromPlacementMap: item removed due no fit."));
		}
	}

	for (UYcInventoryItemInstance* ItemInst : ItemsToRemove)
	{
		ItemInstanceToTileMap.Remove(ItemInst);
	}
}

float UGridInventoryManagerComponent::GetSearchSpeedMultiplier() const
{
	return FMath::Max(0.01f, SearchSpeedMultiplier);
}

void UGridInventoryManagerComponent::SetSearchSpeedMultiplier(const float NewValue)
{
	SearchSpeedMultiplier = FMath::Max(0.01f, NewValue);
}

bool UGridInventoryManagerComponent::IsItemRevealedForCurrentSession(UYcInventoryItemInstance* ItemInst) const
{
	if (!IsValid(ItemInst))
	{
		return false;
	}

	AActor* OuterActor = Cast<AActor>(ItemInst->GetOuter());
	UGridInventoryManagerComponent* ItemOuterInventory = OuterActor ? OuterActor->FindComponentByClass<UGridInventoryManagerComponent>() : nullptr;
	if (!ItemOuterInventory)
	{
		return true;
	}
	return IsItemRevealedForContainerSession(ItemOuterInventory, ItemInst);
}

bool UGridInventoryManagerComponent::IsItemOperableForCurrentSession(UYcInventoryItemInstance* ItemInst) const
{
	return IsItemRevealedForCurrentSession(ItemInst);
}

bool UGridInventoryManagerComponent::GetCurrentSearchProgress(float& OutProgress01, float& OutRemainingSeconds, int32& OutRevealedCount, int32& OutTotalCount) const
{
	OutProgress01 = CurrentSearchProgress01;
	OutRemainingSeconds = 0.0f;
	OutRevealedCount = SearchRevealedItemCount;
	OutTotalCount = SearchTotalItemCount;

	if (!bSearchSessionActive || !SearchContainerInventory || SearchTotalItemCount <= 0)
	{
		return false;
	}
	if (CurrentSearchingItem)
	{
		OutRemainingSeconds = (1.0f - FMath::Clamp(CurrentSearchProgress01, 0.0f, 1.0f)) * CurrentSearchTargetDuration;
	}
	return true;
}

bool UGridInventoryManagerComponent::GetSearchSessionRevisionForContainer(UGridInventoryManagerComponent* ContainerInventory, int32& OutRevision) const
{
	OutRevision = -1;
	if (!bSearchSessionActive || !SearchContainerInventory || SearchContainerInventory != ContainerInventory)
	{
		return false;
	}
	OutRevision = SearchSessionRevision;
	return true;
}

int32 UGridInventoryManagerComponent::GetInventoryGridRevision() const
{
	return InventoryGridRevision;
}

bool UGridInventoryManagerComponent::GetContextMenuActionsForItem(UYcInventoryItemInstance* ItemInst, TArray<FGridItemContextMenuAction>& OutActions)
{
	OutActions.Reset();
	if (!IsValid(ItemInst))
	{
		return false;
	}

	TInstancedStruct<FYcInventoryItemFragment> Result = ItemInst->FindItemFragment(FItemFragment_ContextMenu::StaticStruct());
	const FItemFragment_ContextMenu* MenuFragment = Result.GetPtr<FItemFragment_ContextMenu>();
	if (!MenuFragment || MenuFragment->Actions.IsEmpty())
	{
		return false;
	}

	TArray<FGameplayTag> SeenActionTags;
	for (const FGridItemContextMenuAction& RawActionDef : MenuFragment->Actions)
	{
		if (!RawActionDef.ActionTag.IsValid())
		{
			continue;
		}
		if (SeenActionTags.Contains(RawActionDef.ActionTag))
		{
			UE_LOG(LogYcGridInventory, Warning, TEXT("ContextMenu action duplicated on same item: %s"), *RawActionDef.ActionTag.ToString());
			continue;
		}
		SeenActionTags.Add(RawActionDef.ActionTag);

		FGridItemContextMenuAction ActionDef = RawActionDef;
		FText DisabledReason;
		const bool bCanExecute = EvaluateContextActionExecutability(ItemInst, ActionDef, DisabledReason);
		ActionDef.bRuntimeCanExecute = bCanExecute;
		ActionDef.RuntimeDisabledReason = DisabledReason;
		if (!bCanExecute && !ActionDef.bShowWhenDisabled)
		{
			continue;
		}

		int32 InsertIndex = OutActions.Num();
		for (int32 ExistingIndex = 0; ExistingIndex < OutActions.Num(); ++ExistingIndex)
		{
			if (ActionDef.Order < OutActions[ExistingIndex].Order)
			{
				InsertIndex = ExistingIndex;
				break;
			}
		}
		OutActions.Insert(ActionDef, InsertIndex);
	}

	return OutActions.Num() > 0;
}

bool UGridInventoryManagerComponent::CanExecuteContextAction(UYcInventoryItemInstance* ItemInst, const FGameplayTag ActionTag, FGridItemContextMenuAction& OutActionDef)
{
	if (!TryFindContextMenuActionDef(ItemInst, ActionTag, OutActionDef))
	{
		return false;
	}

	FText DisabledReason;
	const bool bCanExecute = EvaluateContextActionExecutability(ItemInst, OutActionDef, DisabledReason);
	OutActionDef.bRuntimeCanExecute = bCanExecute;
	OutActionDef.RuntimeDisabledReason = DisabledReason;
	return bCanExecute;
}

void UGridInventoryManagerComponent::ServerRequestExecuteItemContextAction_Implementation(UYcInventoryItemInstance* ItemInst, const FGameplayTag ActionTag)
{
	if (!IsValid(ItemInst) || !ActionTag.IsValid())
	{
		return;
	}
	if (!CanExecuteContextActionInAuthoritativeState(ItemInst))
	{
		UE_LOG(LogYcGridInventory, Warning, TEXT("Reject context action: authoritative check failed."));
		return;
	}

	FGridItemContextMenuAction ActionDef;
	if (!CanExecuteContextAction(ItemInst, ActionTag, ActionDef))
	{
		UE_LOG(LogYcGridInventory, Warning, TEXT("Reject context action: policy check failed."));
		return;
	}

	FGridItemContextActionRequest Request;
	Request.Player = GetOwner();
	Request.ItemInst = ItemInst;
	Request.ActionTag = ActionDef.ActionTag;
	Request.ExecutorTag = ActionDef.ExecutorTag;
	Request.EventTag = ActionDef.EventTag;

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.BroadcastMessage(TAG_Yc_Inventory_Message_Grid_ContextAction_Request, Request);
	if (ActionDef.ExecutorTag == TAG_Yc_Inventory_ContextAction_Executor_Message)
	{
		if (ActionDef.EventTag.IsValid())
		{
			MessageSubsystem.BroadcastMessage(ActionDef.EventTag, Request);
		}
		return;
	}

	if (ActionDef.ExecutorTag == TAG_Yc_Inventory_ContextAction_Executor_GAS)
	{
		if (!ActionDef.EventTag.IsValid())
		{
			UE_LOG(LogYcGridInventory, Warning, TEXT("Reject context action: GAS executor missing EventTag."));
			return;
		}

		FGameplayEventData Payload;
		Payload.EventTag = ActionDef.EventTag;
		Payload.Instigator = GetOwner();
		Payload.Target = ItemInst->GetInventoryManager() ? ItemInst->GetInventoryManager()->GetOwner() : nullptr;
		Payload.OptionalObject = ItemInst;
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetOwner(), ActionDef.EventTag, Payload);
		return;
	}

	UE_LOG(LogYcGridInventory, Warning, TEXT("Unknown context action executor: %s"), *ActionDef.ExecutorTag.ToString());
}

bool UGridInventoryManagerComponent::CanExecuteContextActionInAuthoritativeState(UYcInventoryItemInstance* ItemInst) const
{
	if (!IsValid(ItemInst))
	{
		return false;
	}

	AActor* OuterActor = Cast<AActor>(ItemInst->GetOuter());
	UGridInventoryManagerComponent* ItemOuterInventory = OuterActor ? OuterActor->FindComponentByClass<UGridInventoryManagerComponent>() : nullptr;
	if (!ItemOuterInventory)
	{
		return false;
	}

	if (ItemOuterInventory == this)
	{
		return GetStackCountByItemInstance(ItemInst) > 0;
	}

	if (bSearchSessionActive && SearchContainerInventory && ItemOuterInventory == SearchContainerInventory)
	{
		if (!IsItemStillInContainer(ItemInst, SearchContainerInventory))
		{
			return false;
		}
		return IsItemRevealedForContainerSession(ItemOuterInventory, ItemInst);
	}

	return false;
}

void UGridInventoryManagerComponent::ServerStartContainerSearchSession_Implementation(UGridInventoryManagerComponent* ContainerInventory)
{
	ResetSearchSession_Internal();
	if (!ContainerInventory || !ContainerInventory->bEnableContainerSearch)
	{
		return;
	}

	bSearchSessionActive = true;
	SearchContainerInventory = ContainerInventory;
	BuildSearchQueue(ContainerInventory);
	++SearchSessionRevision;
	StartNextSearchItem();
}

void UGridInventoryManagerComponent::ServerResetSearchSession_Implementation()
{
	ResetSearchSession_Internal();
}

void UGridInventoryManagerComponent::TickSearchSession()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (!bSearchSessionActive || !SearchContainerInventory || !CurrentSearchingItem)
	{
		return;
	}
	if (!IsItemStillInContainer(CurrentSearchingItem, SearchContainerInventory))
	{
		StartNextSearchItem();
		return;
	}

	const FItemFragment_GridItem GridFragment = GetItemFragmentGrid(CurrentSearchingItem->GetItemRegistryId());
	const float BaseDuration = FMath::Max(0.0f, GridFragment.SearchDuration);
	if (BaseDuration <= 0.0f)
	{
		RevealItem(CurrentSearchingItem);
		StartNextSearchItem();
		return;
	}

	CurrentSearchProgress01 += (SearchTickInterval * GetSearchSpeedMultiplier()) / BaseDuration;
	CurrentSearchProgress01 = FMath::Clamp(CurrentSearchProgress01, 0.0f, 1.0f);
	if (CurrentSearchProgress01 >= 1.0f)
	{
		RevealItem(CurrentSearchingItem);
		StartNextSearchItem();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(SearchTickTimerHandle, this, &ThisClass::TickSearchSession, SearchTickInterval, false);
	}
}

void UGridInventoryManagerComponent::ResetSearchSession_Internal()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SearchTickTimerHandle);
	}

	bSearchSessionActive = false;
	SearchContainerInventory = nullptr;
	RevealedSearchItems.Reset();
	PendingSearchQueue.Reset();
	CurrentSearchingItem = nullptr;
	CurrentSearchTargetDuration = 0.0f;
	CurrentSearchProgress01 = 0.0f;
	SearchTotalItemCount = 0;
	SearchRevealedItemCount = 0;
	++SearchSessionRevision;
}

bool UGridInventoryManagerComponent::IsItemKnownInMatch(UYcInventoryItemInstance* ItemInst) const
{
	return IsValid(ItemInst) && KnownSearchItems.Contains(ItemInst);
}

void UGridInventoryManagerComponent::MarkItemKnownInMatch(UYcInventoryItemInstance* ItemInst)
{
	if (IsValid(ItemInst) && !KnownSearchItems.Contains(ItemInst))
	{
		KnownSearchItems.Add(ItemInst);
	}
}

bool UGridInventoryManagerComponent::IsItemStillInContainer(UYcInventoryItemInstance* ItemInst, UGridInventoryManagerComponent* ContainerInventory) const
{
	if (!IsValid(ItemInst) || !ContainerInventory)
	{
		return false;
	}

	for (const FGridInventorySlot& Slot : ContainerInventory->InventorySlots)
	{
		if (Slot.bOccupied && Slot.ItemInstance == ItemInst)
		{
			return true;
		}
	}
	return false;
}

void UGridInventoryManagerComponent::RebuildSearchQueueFromContainerPreserveCurrent()
{
	if (!bSearchSessionActive || !SearchContainerInventory)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SearchTickTimerHandle);
	}

	const bool bCurrentStillValid = IsValid(CurrentSearchingItem) && IsItemStillInContainer(CurrentSearchingItem, SearchContainerInventory) && !IsItemKnownInMatch(CurrentSearchingItem);
	TArray<TObjectPtr<UYcInventoryItemInstance>> NewPendingQueue;
	TArray<TObjectPtr<UYcInventoryItemInstance>> NewRevealedItems;

	// 仅遍历顶点格避免多格物品重复入队。
	for (const FGridInventorySlot& Slot : SearchContainerInventory->InventorySlots)
	{
		if (!Slot.bOccupied || !IsValid(Slot.ItemInstance))
		{
			continue;
		}
		if (Slot.ItemRelativeX != 0 || Slot.ItemRelativeY != 0)
		{
			continue;
		}

		UYcInventoryItemInstance* ItemInst = Slot.ItemInstance;
		if (IsItemKnownInMatch(ItemInst))
		{
			NewRevealedItems.AddUnique(ItemInst);
			continue;
		}
		if (bCurrentStillValid && ItemInst == CurrentSearchingItem)
		{
			continue;
		}
		NewPendingQueue.AddUnique(ItemInst);
	}

	RevealedSearchItems = NewRevealedItems;
	SearchRevealedItemCount = RevealedSearchItems.Num();
	PendingSearchQueue = NewPendingQueue;
	SearchTotalItemCount = PendingSearchQueue.Num() + (bCurrentStillValid ? 1 : 0);
	if (!bCurrentStillValid)
	{
		// 当前搜索目标失效时清空。
		CurrentSearchingItem = nullptr;
		CurrentSearchTargetDuration = 0.0f;
		CurrentSearchProgress01 = 0.0f;
		++SearchSessionRevision;
		StartNextSearchItem();
		return;
	}

	++SearchSessionRevision;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(SearchTickTimerHandle, this, &ThisClass::TickSearchSession, SearchTickInterval, false);
	}
}

void UGridInventoryManagerComponent::BuildSearchQueue(UGridInventoryManagerComponent* ContainerInventory)
{
	PendingSearchQueue.Reset();
	SearchTotalItemCount = 0;
	SearchRevealedItemCount = 0;
	RevealedSearchItems.Reset();
	if (!ContainerInventory)
	{
		return;
	}

	for (const FGridInventorySlot& Slot : ContainerInventory->InventorySlots)
	{
		if (!Slot.bOccupied || !IsValid(Slot.ItemInstance))
		{
			continue;
		}
		if (Slot.ItemRelativeX != 0 || Slot.ItemRelativeY != 0)
		{
			continue;
		}
		if (IsItemKnownInMatch(Slot.ItemInstance))
		{
			RevealedSearchItems.AddUnique(Slot.ItemInstance);
			SearchRevealedItemCount = RevealedSearchItems.Num();
			continue;
		}
		PendingSearchQueue.AddUnique(Slot.ItemInstance);
	}
	SearchTotalItemCount = PendingSearchQueue.Num();
}

void UGridInventoryManagerComponent::StartNextSearchItem()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SearchTickTimerHandle);
	}

	CurrentSearchingItem = nullptr;
	CurrentSearchTargetDuration = 0.0f;
	CurrentSearchProgress01 = 0.0f;
	while (PendingSearchQueue.Num() > 0)
	{
		UYcInventoryItemInstance* NextItem = PendingSearchQueue[0];
		PendingSearchQueue.RemoveAt(0);
		// 搜索队列可能含过期项，逐项清洗。
		if (!IsValid(NextItem))
		{
			continue;
		}
		if (!IsItemStillInContainer(NextItem, SearchContainerInventory))
		{
			continue;
		}
		if (IsItemKnownInMatch(NextItem))
		{
			RevealItem(NextItem);
			continue;
		}

		const FItemFragment_GridItem GridFragment = GetItemFragmentGrid(NextItem->GetItemRegistryId());
		const float BaseDuration = FMath::Max(0.0f, GridFragment.SearchDuration);
		if (BaseDuration <= 0.0f)
		{
			RevealItem(NextItem);
			continue;
		}

		CurrentSearchingItem = NextItem;
		CurrentSearchTargetDuration = BaseDuration / GetSearchSpeedMultiplier();
		CurrentSearchProgress01 = 0.0f;
		// 推进搜索会话修订号供UI刷新。
		++SearchSessionRevision;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(SearchTickTimerHandle, this, &ThisClass::TickSearchSession, SearchTickInterval, false);
		}
		return;
	}

	CurrentSearchProgress01 = SearchTotalItemCount > 0 ? 1.0f : 0.0f;
	++SearchSessionRevision;
}

void UGridInventoryManagerComponent::RevealItem(UYcInventoryItemInstance* ItemInst)
{
	if (!IsValid(ItemInst))
	{
		return;
	}
	MarkItemKnownInMatch(ItemInst);
	if (RevealedSearchItems.Contains(ItemInst))
	{
		return;
	}
	RevealedSearchItems.Add(ItemInst);
	SearchRevealedItemCount = RevealedSearchItems.Num();
	++SearchSessionRevision;
}

bool UGridInventoryManagerComponent::IsItemRevealedForContainerSession(UGridInventoryManagerComponent* ItemOuterInventory, UYcInventoryItemInstance* ItemInst) const
{
	if (!ItemOuterInventory || !ItemOuterInventory->bEnableContainerSearch)
	{
		return true;
	}
	if (IsItemKnownInMatch(ItemInst))
	{
		return true;
	}
	if (!bSearchSessionActive || !SearchContainerInventory || SearchContainerInventory != ItemOuterInventory)
	{
		return false;
	}
	return RevealedSearchItems.Contains(ItemInst);
}

bool UGridInventoryManagerComponent::EvaluateContextActionExecutability(UYcInventoryItemInstance* ItemInst, const FGridItemContextMenuAction& ActionDef, FText& OutDisabledReason)
{
	if (!IsValid(ItemInst))
	{
		OutDisabledReason = FText::FromString(TEXT("Invalid item."));
		return false;
	}
	if (!ActionDef.ActionTag.IsValid())
	{
		OutDisabledReason = FText::FromString(TEXT("Invalid action tag."));
		return false;
	}
	if (!IsValidContextActionExecutorTag(ActionDef.ExecutorTag))
	{
		OutDisabledReason = FText::FromString(TEXT("Invalid executor tag namespace."));
		return false;
	}
	if (!IsItemOperableForCurrentSession(ItemInst))
	{
		OutDisabledReason = FText::FromString(TEXT("Item is not revealed yet."));
		return false;
	}

	FYcContextActionEvalResult EvalResult;
	const bool bPassed = UYcGridInventoryContextActionLibrary::EvaluateContextActionPolicyForItem(GetOwner(), this, ItemInst, ActionDef, true, EvalResult);
	if (!bPassed)
	{
		OutDisabledReason = EvalResult.FailReason.IsEmpty() ? FText::FromString(TEXT("Context action policy blocked.")) : EvalResult.FailReason;
		return false;
	}
	return true;
}

bool UGridInventoryManagerComponent::IsValidContextActionExecutorTag(const FGameplayTag ExecutorTag) const
{
	return UYcGridInventoryContextActionLibrary::IsExecutorTagAllowed(ExecutorTag);
}

bool UGridInventoryManagerComponent::TryFindContextMenuActionDef(UYcInventoryItemInstance* ItemInst, const FGameplayTag ActionTag, FGridItemContextMenuAction& OutActionDef) const
{
	OutActionDef = FGridItemContextMenuAction();
	if (!IsValid(ItemInst) || !ActionTag.IsValid())
	{
		return false;
	}

	TInstancedStruct<FYcInventoryItemFragment> Result = ItemInst->FindItemFragment(FItemFragment_ContextMenu::StaticStruct());
	const FItemFragment_ContextMenu* MenuFragment = Result.GetPtr<FItemFragment_ContextMenu>();
	if (!MenuFragment)
	{
		return false;
	}

	TArray<FGameplayTag> SeenActionTags;
	for (const FGridItemContextMenuAction& ActionDef : MenuFragment->Actions)
	{
		if (!ActionDef.ActionTag.IsValid())
		{
			continue;
		}
		if (SeenActionTags.Contains(ActionDef.ActionTag))
		{
			UE_LOG(LogYcGridInventory, Warning, TEXT("ContextMenu action duplicated on same item: %s"), *ActionDef.ActionTag.ToString());
			continue;
		}
		SeenActionTags.Add(ActionDef.ActionTag);
		if (ActionDef.ActionTag == ActionTag)
		{
			OutActionDef = ActionDef;
			return true;
		}
	}
	return false;
}

bool UGridInventoryManagerComponent::ValidateSwapLikeOperation(const FYcInventoryOperation& InOperation, FString& OutReason)
{
	OutReason = TEXT("");
	if (!InOperation.SourceInventory || !InOperation.TargetInventory || !InOperation.ItemInstance)
	{
		OutReason = TEXT("Swap-like op missing source/target/item.");
		return false;
	}
	return true;
}

bool UGridInventoryManagerComponent::ExecuteSwapLikeOperation(const FYcInventoryOperation& InOperation, FString& OutReason)
{
	OutReason = TEXT("");
	FString OutSummary;
	CurrentHandleRegionId = InOperation.GridRegionId;
	CurrentExpectedSourceRegionId = InOperation.SourceGridRegionId;
	CurrentHandlePocketIndex = InOperation.GridPocketIndex;
	CurrentExpectedSourcePocketIndex = InOperation.SourceGridPocketIndex;
	const bool bSuccess = ExecuteSwapGridOperation(InOperation.TargetInventory, InOperation.ItemInstance, InOperation.StackCount, InOperation.GridTile, InOperation.bRotated, InOperation.SourceInventory, OutReason, OutSummary);
	CurrentHandleRegionId = FGameplayTag();
	CurrentExpectedSourceRegionId = FGameplayTag();
	CurrentHandlePocketIndex = -1;
	CurrentExpectedSourcePocketIndex = -1;
	if (bSuccess && !OutSummary.IsEmpty())
	{
		OutReason = OutSummary;
	}
	return bSuccess;
}

void UGridInventoryManagerComponent::BuildSwapLikeOperationDelta(const FYcInventoryOperation& InOperation, const bool bSuccess, FYcInventoryOperationDelta& OutDelta)
{
	if (!bSuccess)
	{
		OutDelta.Summary = TEXT("Swap-like operation failed.");
	}
	if (InOperation.ItemInstance)
	{
		OutDelta.AffectedItemIds.Add(InOperation.ItemInstance->GetItemInstId());
	}
}

bool UGridInventoryManagerComponent::ExecuteSwapGridOperation(UYcInventoryManagerComponent* OnDropInventory, UYcInventoryItemInstance* ItemInst, const int32 StackCount, const FIntPoint Tile, const bool bRotated, UYcInventoryManagerComponent* ExpectedSourceInventory, FString& OutReason, FString& OutSummary)
{
	OutReason = TEXT("");
	OutSummary = TEXT("");
	if (!IsValid(ItemInst) || !OnDropInventory || !ExpectedSourceInventory)
	{
		OutReason = TEXT("SwapGrid invalid input.");
		return false;
	}

	UGridInventoryManagerComponent* TargetInventory = Cast<UGridInventoryManagerComponent>(OnDropInventory);
	UGridInventoryManagerComponent* ExpectedSourceGrid = Cast<UGridInventoryManagerComponent>(ExpectedSourceInventory);
	if (!TargetInventory || !ExpectedSourceGrid)
	{
		OutReason = TEXT("SwapGrid source/target inventory type invalid.");
		return false;
	}

	UGridInventoryManagerComponent* SourceInventory = Cast<UGridInventoryManagerComponent>(ItemInst->GetInventoryManager());
	if (!SourceInventory)
	{
		OutReason = TEXT("SwapGrid source inventory missing.");
		return false;
	}
	if (SourceInventory != ExpectedSourceGrid)
	{
		// CAS 源库存变化拦截。
		OutReason = TEXT("SwapGrid source changed.");
		return false;
	}
	if (SourceInventory != this && !SourceInventory->bEnableContainerSearch && !SourceInventory->bAllowDirectContainerInteraction)
	{
		OutReason = TEXT("SwapGrid source not interactable.");
		return false;
	}
	if (TargetInventory != this && TargetInventory != SourceInventory && !TargetInventory->bEnableContainerSearch && !TargetInventory->bAllowDirectContainerInteraction)
	{
		OutReason = TEXT("SwapGrid target not interactable.");
		return false;
	}
	if (!IsItemRevealedForContainerSession(SourceInventory, ItemInst))
	{
		OutReason = TEXT("SwapGrid source item not revealed.");
		return false;
	}

	const int32 RealStackCount = SourceInventory->GetStackCountByItemInstance(ItemInst);
	if (RealStackCount <= 0)
	{
		OutReason = TEXT("SwapGrid invalid stack.");
		return false;
	}

	const FGameplayTag TargetRegionId = TargetInventory->ResolvePlacementRegionId(CurrentHandleRegionId);
	const int32 TargetPocketIndex = TargetInventory->ResolvePlacementPocketIndex(TargetRegionId, CurrentHandlePocketIndex);
	if (!TargetInventory->CanPlaceGridItemInst(ItemInst, Tile, bRotated, TargetRegionId, TargetPocketIndex))
	{
		// 目标库存不可放置。
		OutReason = TEXT("SwapGrid target blocked.");
		return false;
	}
	if (SourceInventory == TargetInventory)
	{
		SourceInventory->InnerSwapItemPosition(ItemInst, RealStackCount, Tile, bRotated, TargetRegionId, TargetPocketIndex);
		OutSummary = TEXT("SwapGrid same inventory.");
		return true;
	}

	FGameplayTag SourceRegionId;
	int32 SourcePocketIndex = -1;
	FIntPoint SourceTile = FIntPoint::ZeroValue;
	bool bSourceRotated = false;
	const bool bHasSourcePlacement = SourceInventory->TryGetItemPlacementInfoWithRegion(ItemInst, SourceRegionId, SourcePocketIndex, SourceTile, bSourceRotated);
	if (CurrentExpectedSourceRegionId.IsValid() && SourceRegionId != CurrentExpectedSourceRegionId)
	{
		OutReason = TEXT("SwapGrid source region changed.");
		return false;
	}
	if (CurrentExpectedSourcePocketIndex >= 0 && SourcePocketIndex != CurrentExpectedSourcePocketIndex)
	{
		OutReason = TEXT("SwapGrid source pocket changed.");
		return false;
	}

	if (!SourceInventory->RemoveItemInstance(ItemInst))
	{
		OutReason = TEXT("SwapGrid remove source failed.");
		return false;
	}

	TargetInventory->CachedTile = Tile;
	TargetInventory->CachedRegionId = TargetRegionId;
	TargetInventory->CachedPocketIndex = TargetPocketIndex;
	if (!TargetInventory->AddItemInstance(ItemInst, RealStackCount))
	{
		// 目标添加失败时优先原位回滚。
		if (bHasSourcePlacement && SourceInventory->CanPlaceGridItemInst(ItemInst, SourceTile, bSourceRotated, SourceRegionId, SourcePocketIndex))
		{
			SourceInventory->CachedTile = SourceTile;
			SourceInventory->CachedRegionId = SourceRegionId;
			SourceInventory->CachedPocketIndex = SourcePocketIndex;
			SourceInventory->AddItemInstance(ItemInst, RealStackCount);
		}
		else if (bHasSourcePlacement && SourceInventory->CanPlaceGridItemInst(ItemInst, SourceTile, false, SourceRegionId, SourcePocketIndex))
		{
			SourceInventory->CachedTile = SourceTile;
			SourceInventory->CachedRegionId = SourceRegionId;
			SourceInventory->CachedPocketIndex = SourcePocketIndex;
			SourceInventory->AddItemInstance(ItemInst, RealStackCount);
		}
		else
		{
			FGameplayTag FallbackRegionId;
			int32 FallbackPocketIndex = -1;
			bool bFitRotated = false;
			FIntPoint FallbackTile = FIntPoint::ZeroValue;
			if (SourceInventory->FindFirstFitPlacementForItemInst(ItemInst, FallbackRegionId, FallbackPocketIndex, FallbackTile, bFitRotated))
			{
				SourceInventory->CachedTile = FallbackTile;
				SourceInventory->CachedRegionId = FallbackRegionId;
				SourceInventory->CachedPocketIndex = FallbackPocketIndex;
				SourceInventory->AddItemInstance(ItemInst, RealStackCount);
			}
			else
			{
				UE_LOG(LogYcGridInventory, Error, TEXT("Swap rollback failed: no fit in source inventory."));
			}
		}

		OutReason = TEXT("SwapGrid add target failed, rolled back.");
		return false;
	}

	// 已知物品不应在放入可搜索容器后再次进入“待搜索”队列：
	// 当物品来自非搜索库存并成功放入可搜索容器时，直接标记为已知。
	const bool bMovedIntoSearchableContainerFromNonSearch =
		(SourceInventory != TargetInventory) &&
		TargetInventory->bEnableContainerSearch &&
		!SourceInventory->bEnableContainerSearch;
	if (bMovedIntoSearchableContainerFromNonSearch)
	{
		MarkItemKnownInMatch(ItemInst);
		if (bSearchSessionActive && SearchContainerInventory == TargetInventory)
		{
			RebuildSearchQueueFromContainerPreserveCurrent();
		}
	}

	OutSummary = TEXT("SwapGrid completed.");
	return true;
}

void UGridInventoryManagerComponent::InnerSwapItemPosition(UYcInventoryItemInstance* ItemInst, const int32 StackCount, const FIntPoint Tile, const bool bRotated, const FGameplayTag RegionId, const int32 PocketIndex)
{
	OnGridItemInstanceAdded(ItemInst, StackCount, Tile, bRotated, RegionId, PocketIndex);
}

bool UGridInventoryManagerComponent::GetItemLeftTopPosition(const FYcItemInstanceId ItemID, FIntPoint& OutPoint)
{
	for (const TPair<TObjectPtr<UYcInventoryItemInstance>, FItemGridInfo>& Entry : ItemInstanceToTileMap)
	{
		if (Entry.Key && Entry.Key->GetItemInstId() == ItemID)
		{
			OutPoint = Entry.Value.TilePos;
			return true;
		}
	}

	return false;
}

TMap<UYcInventoryItemInstance*, FIntPoint> UGridInventoryManagerComponent::GetGridItemsTileMap()
{
	TMap<UYcInventoryItemInstance*, FIntPoint> Items;
	const FGameplayTag PrimaryRegionId = GetPrimaryRegionId();
	const int32 PrimaryPocketIndex = GetPrimaryPocketIndex(PrimaryRegionId);
	for (const TPair<TObjectPtr<UYcInventoryItemInstance>, FItemGridInfo>& It : ItemInstanceToTileMap)
	{
		if (PrimaryRegionId.IsValid() && (It.Value.RegionId != PrimaryRegionId || It.Value.PocketIndex != PrimaryPocketIndex))
		{
			continue;
		}
		Items.Add(It.Key, It.Value.TilePos);
	}
	return Items;
}

TMap<UYcInventoryItemInstance*, FIntPoint> UGridInventoryManagerComponent::GetGridItemsTileMapByRegion(const FGameplayTag RegionId, const int32 PocketIndex)
{
	TMap<UYcInventoryItemInstance*, FIntPoint> Items;
	for (const TPair<TObjectPtr<UYcInventoryItemInstance>, FItemGridInfo>& It : ItemInstanceToTileMap)
	{
		const bool bRegionMatch = !RegionId.IsValid() || It.Value.RegionId == RegionId;
		const bool bPocketMatch = PocketIndex < 0 || It.Value.PocketIndex == PocketIndex;
		if (bRegionMatch && bPocketMatch)
		{
			Items.Add(It.Key, It.Value.TilePos);
		}
	}
	return Items;
}

TMap<UYcInventoryItemInstance*, bool> UGridInventoryManagerComponent::GetGridItemRotationMap()
{
	TMap<UYcInventoryItemInstance*, bool> Rotations;
	for (const TPair<TObjectPtr<UYcInventoryItemInstance>, FItemGridInfo>& It : ItemInstanceToTileMap)
	{
		if (!It.Key)
		{
			continue;
		}

		const FItemFragment_GridItem GridFragment = GetItemFragmentGrid(It.Key->GetItemRegistryId());
		const int32 PlacedWidth = It.Value.ItemSize.X;
		const int32 PlacedHeight = It.Value.ItemSize.Y;
		const bool bRotated = PlacedWidth == GridFragment.Dimensions.Y && PlacedHeight == GridFragment.Dimensions.X &&
			(PlacedWidth != GridFragment.Dimensions.X || PlacedHeight != GridFragment.Dimensions.Y);
		Rotations.Add(It.Key, bRotated);
	}
	return Rotations;
}

bool UGridInventoryManagerComponent::FindFirstFitPosition(const FDataRegistryId ItemDefId, FIntPoint& Tile, bool& OutRotated)
{
	FGameplayTag RegionId;
	int32 PocketIndex = -1;
	return FindFirstFitPlacement(ItemDefId, RegionId, PocketIndex, Tile, OutRotated);
}

bool UGridInventoryManagerComponent::FindFirstFitPlacement(const FDataRegistryId ItemDefId, FGameplayTag& OutRegionId, int32& OutPocketIndex, FIntPoint& Tile, bool& OutRotated)
{
	for (const FGridInventoryRegionRuntimeState& Region : RegionStates)
	{
		if (!Region.bEnabled)
		{
			continue;
		}
		if (FindFirstFitPositionInRegion(ItemDefId, Region.RegionId, Region.PocketIndex, Tile, OutRotated))
		{
			OutRegionId = Region.RegionId;
			OutPocketIndex = Region.PocketIndex;
			return true;
		}
	}
	OutRegionId = FGameplayTag();
	OutPocketIndex = -1;
	return false;
}

bool UGridInventoryManagerComponent::FindFirstFitPlacementForItemInst(UYcInventoryItemInstance* ItemInst, FGameplayTag& OutRegionId, int32& OutPocketIndex, FIntPoint& Tile, bool& OutRotated)
{
	if (!IsValid(ItemInst))
	{
		OutRegionId = FGameplayTag();
		OutPocketIndex = -1;
		return false;
	}

	for (const FGridInventoryRegionRuntimeState& Region : RegionStates)
	{
		if (!Region.bEnabled)
		{
			continue;
		}
		if (FindFirstFitPositionInRegionForItemInst(ItemInst, Region.RegionId, Region.PocketIndex, Tile, OutRotated))
		{
			OutRegionId = Region.RegionId;
			OutPocketIndex = Region.PocketIndex;
			return true;
		}
	}

	OutRegionId = FGameplayTag();
	OutPocketIndex = -1;
	return false;
}

bool UGridInventoryManagerComponent::FindFirstFitPlacementLegacy(const FDataRegistryId ItemDefId, FGameplayTag& OutRegionId, FIntPoint& Tile, bool& OutRotated)
{
	int32 OutPocketIndex = -1;
	return FindFirstFitPlacement(ItemDefId, OutRegionId, OutPocketIndex, Tile, OutRotated);
}

bool UGridInventoryManagerComponent::FindFirstFitPositionInRegion(const FDataRegistryId ItemDefId, const FGameplayTag RegionId, const int32 PocketIndex, FIntPoint& Tile, bool& OutRotated)
{
	if (!PassesRegionTagConstraintForItemDef(ItemDefId, RegionId))
	{
		return false;
	}

	const FItemFragment_GridItem GridFragment = GetItemFragmentGrid(ItemDefId);
	const int32 Columns = GetRegionColumns(RegionId, PocketIndex);
	const int32 Rows = GetRegionRows(RegionId, PocketIndex);
	for (int32 Y = 0; Y < Rows; ++Y)
	{
		for (int32 X = 0; X < Columns; ++X)
		{
			if (CanPlaceGridItem(ItemDefId, FIntPoint(X, Y), false, RegionId, PocketIndex))
			{
				Tile = FIntPoint(X, Y);
				OutRotated = false;
				return true;
			}
		}
	}
	if (!GridFragment.bCanRotate)
	{
		return false;
	}
	for (int32 Y = 0; Y < Rows; ++Y)
	{
		for (int32 X = 0; X < Columns; ++X)
		{
			if (CanPlaceGridItem(ItemDefId, FIntPoint(X, Y), true, RegionId, PocketIndex))
			{
				Tile = FIntPoint(X, Y);
				OutRotated = true;
				return true;
			}
		}
	}
	return false;
}

bool UGridInventoryManagerComponent::FindFirstFitPositionInRegionLegacy(const FDataRegistryId ItemDefId, const FGameplayTag RegionId, FIntPoint& Tile, bool& OutRotated)
{
	return FindFirstFitPositionInRegion(ItemDefId, RegionId, -1, Tile, OutRotated);
}

bool UGridInventoryManagerComponent::PassesRegionTagConstraintForItemDef(const FDataRegistryId ItemDefId, const FGameplayTag RegionId)
{
	const FGameplayTag FinalRegionId = ResolvePlacementRegionId(RegionId);
	return UYcGridInventoryPlacementRuleLibrary::PassesTagConstraintForItemDef(ItemDefId, GetRegionTagConstraint(FinalRegionId));
}

bool UGridInventoryManagerComponent::PassesRegionTagConstraintForItemInst(UYcInventoryItemInstance* ItemInst, const FGameplayTag RegionId)
{
	const FGameplayTag FinalRegionId = ResolvePlacementRegionId(RegionId);
	return UYcGridInventoryPlacementRuleLibrary::PassesTagConstraintForItemInstance(ItemInst, GetRegionTagConstraint(FinalRegionId));
}

bool UGridInventoryManagerComponent::CanPlaceGridItem(const FDataRegistryId ItemDefId, const FIntPoint Tile, const bool bRotated, const FGameplayTag RegionId, const int32 PocketIndex)
{
	const FGameplayTag FinalRegionId = ResolvePlacementRegionId(RegionId);
	const int32 FinalPocketIndex = ResolvePlacementPocketIndex(FinalRegionId, PocketIndex);
	if (!PassesRegionTagConstraintForItemDef(ItemDefId, FinalRegionId))
	{
		return false;
	}

	const FItemFragment_GridItem GridFragment = GetItemFragmentGrid(ItemDefId);
	const int32 ItemWidth = bRotated ? GridFragment.Dimensions.Y : GridFragment.Dimensions.X;
	const int32 ItemHeight = bRotated ? GridFragment.Dimensions.X : GridFragment.Dimensions.Y;
	const int32 Columns = GetRegionColumns(FinalRegionId, FinalPocketIndex);
	const int32 Rows = GetRegionRows(FinalRegionId, FinalPocketIndex);
	if (Tile.X < 0 || Tile.Y < 0 || Tile.X + ItemWidth > Columns || Tile.Y + ItemHeight > Rows)
	{
		return false;
	}

	const TArray<FGridInventorySlot> RegionSlots = GetRegionSlots(FinalRegionId, FinalPocketIndex);
	for (int32 Y = Tile.Y; Y < Tile.Y + ItemHeight; ++Y)
	{
		for (int32 X = Tile.X; X < Tile.X + ItemWidth; ++X)
		{
			if (!IsRegionCellAvailable(FinalRegionId, FinalPocketIndex, FIntPoint(X, Y)))
			{
				return false;
			}
			const int32 Index = TileToIndexInRegion(FIntPoint(X, Y), Columns);
			if (!RegionSlots.IsValidIndex(Index) || RegionSlots[Index].bOccupied)
			{
				return false;
			}
		}
	}
	return true;
}

bool UGridInventoryManagerComponent::CanPlaceGridItemInst(UYcInventoryItemInstance* ItemInst, const FIntPoint Tile, const bool bRotated, const FGameplayTag RegionId, const int32 PocketIndex)
{
	if (!IsValid(ItemInst))
	{
		return false;
	}
	const FGameplayTag FinalRegionId = ResolvePlacementRegionId(RegionId);
	const int32 FinalPocketIndex = ResolvePlacementPocketIndex(FinalRegionId, PocketIndex);
	if (!PassesRegionTagConstraintForItemInst(ItemInst, FinalRegionId))
	{
		return false;
	}

	const FItemFragment_GridItem GridFragment = GetItemFragmentGrid(ItemInst->GetItemRegistryId());
	const int32 ItemWidth = bRotated ? GridFragment.Dimensions.Y : GridFragment.Dimensions.X;
	const int32 ItemHeight = bRotated ? GridFragment.Dimensions.X : GridFragment.Dimensions.Y;
	const int32 Columns = GetRegionColumns(FinalRegionId, FinalPocketIndex);
	const int32 Rows = GetRegionRows(FinalRegionId, FinalPocketIndex);
	if (Tile.X < 0 || Tile.Y < 0 || Tile.X + ItemWidth > Columns || Tile.Y + ItemHeight > Rows)
	{
		return false;
	}

	const TArray<FGridInventorySlot> RegionSlots = GetRegionSlots(FinalRegionId, FinalPocketIndex);
	for (int32 Y = Tile.Y; Y < Tile.Y + ItemHeight; ++Y)
	{
		for (int32 X = Tile.X; X < Tile.X + ItemWidth; ++X)
		{
			if (!IsRegionCellAvailable(FinalRegionId, FinalPocketIndex, FIntPoint(X, Y)))
			{
				return false;
			}
			const int32 Index = TileToIndexInRegion(FIntPoint(X, Y), Columns);
			if (!RegionSlots.IsValidIndex(Index))
			{
				return false;
			}
			if (RegionSlots[Index].bOccupied && RegionSlots[Index].ItemInstance != ItemInst)
			{
				return false;
			}
		}
	}
	return true;
}

FItemFragment_GridItem UGridInventoryManagerComponent::GetItemFragmentGrid(const FDataRegistryId ItemDefId) const
{
	TInstancedStruct<FYcInventoryItemFragment> Result = UYcInventoryLibrary::FindItemFragmentById(ItemDefId, FItemFragment_GridItem::StaticStruct());
	if (const FItemFragment_GridItem* GridFragment = Result.GetPtr<FItemFragment_GridItem>())
	{
		return *GridFragment;
	}
	return FItemFragment_GridItem();
}

FIntPoint UGridInventoryManagerComponent::IndexToTile(const int32 Index) const
{
	return FIntPoint(Index % InventoryColumns, Index / FMath::Max(1, InventoryColumns));
}

int32 UGridInventoryManagerComponent::TileToIndex(const FIntPoint Tile) const
{
	return TileToIndexInRegion(Tile, InventoryColumns);
}

int32 UGridInventoryManagerComponent::TileToIndexInRegion(const FIntPoint Tile, const int32 RegionColumns) const
{
	return Tile.Y * RegionColumns + Tile.X;
}

FGameplayTag UGridInventoryManagerComponent::GetPrimaryRegionId() const
{
	for (const FGridInventoryRegionRuntimeState& State : RegionStates)
	{
		if (State.bEnabled)
		{
			return State.RegionId;
		}
	}
	return FGameplayTag();
}

int32 UGridInventoryManagerComponent::GetPrimaryPocketIndex(const FGameplayTag RegionId) const
{
	for (const FGridInventoryRegionRuntimeState& State : RegionStates)
	{
		if (State.bEnabled && State.RegionId == RegionId)
		{
			return State.PocketIndex;
		}
	}
	return 0;
}

TArray<FGameplayTag> UGridInventoryManagerComponent::GetEnabledRegionIdsSorted() const
{
	TArray<FGameplayTag> RegionIds;
	for (const FGridInventoryRegionRuntimeState& State : RegionStates)
	{
		if (State.bEnabled)
		{
			RegionIds.AddUnique(State.RegionId);
		}
	}
	return RegionIds;
}

TArray<FGridInventoryRegionRuntimeState> UGridInventoryManagerComponent::GetEnabledPocketStates() const
{
	TArray<FGridInventoryRegionRuntimeState> States;
	for (const FGridInventoryRegionRuntimeState& State : RegionStates)
	{
		if (State.bEnabled)
		{
			States.Add(State);
		}
	}
	return States;
}

bool UGridInventoryManagerComponent::GetItemPlacementRegion(UYcInventoryItemInstance* ItemInst, FGameplayTag& OutRegionId, int32& OutPocketIndex) const
{
	OutRegionId = FGameplayTag();
	OutPocketIndex = 0;
	const FItemGridInfo* GridInfo = ItemInstanceToTileMap.Find(ItemInst);
	if (!IsValid(ItemInst) || !GridInfo)
	{
		return false;
	}
	OutRegionId = GridInfo->RegionId;
	OutPocketIndex = GridInfo->PocketIndex;
	return true;
}

bool UGridInventoryManagerComponent::GetItemPlacementRegionLegacy(UYcInventoryItemInstance* ItemInst, FGameplayTag& OutRegionId) const
{
	int32 OutPocketIndex = -1;
	return GetItemPlacementRegion(ItemInst, OutRegionId, OutPocketIndex);
}

FGameplayTag UGridInventoryManagerComponent::ResolvePlacementRegionId(const FGameplayTag RegionId) const
{
	if (RegionId.IsValid())
	{
		for (const FGridInventoryRegionRuntimeState& State : RegionStates)
		{
			if (State.RegionId == RegionId && State.bEnabled)
			{
				return RegionId;
			}
		}
	}
	return GetPrimaryRegionId();
}

int32 UGridInventoryManagerComponent::ResolvePlacementPocketIndex(const FGameplayTag RegionId, const int32 PocketIndex) const
{
	if (PocketIndex >= 0)
	{
		for (const FGridInventoryRegionRuntimeState& State : RegionStates)
		{
			if (State.RegionId == RegionId && State.PocketIndex == PocketIndex && State.bEnabled)
			{
				return PocketIndex;
			}
		}
	}
	return GetPrimaryPocketIndex(RegionId);
}

int32 UGridInventoryManagerComponent::GetRegionColumns(const FGameplayTag RegionId, const int32 PocketIndex) const
{
	const int32 FinalPocketIndex = ResolvePlacementPocketIndex(RegionId, PocketIndex);
	for (const FGridInventoryRegionRuntimeState& State : RegionStates)
	{
		if (State.RegionId == RegionId && State.PocketIndex == FinalPocketIndex)
		{
			return State.Columns;
		}
	}
	return InventoryColumns;
}

int32 UGridInventoryManagerComponent::GetRegionRows(const FGameplayTag RegionId, const int32 PocketIndex) const
{
	const int32 FinalPocketIndex = ResolvePlacementPocketIndex(RegionId, PocketIndex);
	for (const FGridInventoryRegionRuntimeState& State : RegionStates)
	{
		if (State.RegionId == RegionId && State.PocketIndex == FinalPocketIndex)
		{
			return State.Rows;
		}
	}
	return InventoryRows;
}

int32 UGridInventoryManagerComponent::GetRegionPriority(const FGameplayTag RegionId, const int32 PocketIndex) const
{
	const int32 FinalPocketIndex = ResolvePlacementPocketIndex(RegionId, PocketIndex);
	for (const FGridInventoryRegionRuntimeState& State : RegionStates)
	{
		if (State.RegionId == RegionId && State.PocketIndex == FinalPocketIndex)
		{
			return State.Priority;
		}
	}
	return 9999;
}

TArray<FGridInventorySlot> UGridInventoryManagerComponent::GetRegionSlots(const FGameplayTag RegionId, const int32 PocketIndex) const
{
	const int32 FinalPocketIndex = ResolvePlacementPocketIndex(RegionId, PocketIndex);
	const int32 Index = FindRegionSlotsStorageIndex(RegionId, FinalPocketIndex);
	if (Index == INDEX_NONE)
	{
		TArray<FGridInventorySlot> Slots;
		Slots.SetNum(FMath::Max(1, GetRegionColumns(RegionId, FinalPocketIndex) * GetRegionRows(RegionId, FinalPocketIndex)));
		for (FGridInventorySlot& Slot : Slots)
		{
			Slot.Reset();
		}
		return Slots;
	}
	return RegionSlotsStorage[Index].Slots;
}

void UGridInventoryManagerComponent::SetRegionSlots(const FGameplayTag RegionId, const int32 PocketIndex, const TArray<FGridInventorySlot>& InSlots)
{
	const int32 FinalPocketIndex = ResolvePlacementPocketIndex(RegionId, PocketIndex);
	const int32 Index = FindRegionSlotsStorageIndex(RegionId, FinalPocketIndex);
	if (Index == INDEX_NONE)
	{
		FGridInventoryRegionSlotsStorage NewStorage;
		NewStorage.RegionId = RegionId;
		NewStorage.PocketIndex = FinalPocketIndex;
		NewStorage.Slots = InSlots;
		RegionSlotsStorage.Add(NewStorage);
		return;
	}

	RegionSlotsStorage[Index].Slots = InSlots;
}

int32 UGridInventoryManagerComponent::FindRegionSlotsStorageIndex(const FGameplayTag RegionId, const int32 PocketIndex) const
{
	for (int32 i = 0; i < RegionSlotsStorage.Num(); ++i)
	{
		if (RegionSlotsStorage[i].RegionId == RegionId && RegionSlotsStorage[i].PocketIndex == PocketIndex)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void UGridInventoryManagerComponent::SetRegionShapeCells(const FGameplayTag RegionId, const int32 PocketIndex, const TArray<FIntPoint>& InCells)
{
	const int32 FinalPocketIndex = ResolvePlacementPocketIndex(RegionId, PocketIndex);
	const int32 Index = FindRegionShapeStorageIndex(RegionId, FinalPocketIndex);
	if (Index == INDEX_NONE)
	{
		FGridInventoryRegionShapeStorage NewStorage;
		NewStorage.RegionId = RegionId;
		NewStorage.PocketIndex = FinalPocketIndex;
		NewStorage.ShapeCells = InCells;
		RegionShapeStorage.Add(NewStorage);
		return;
	}
	RegionShapeStorage[Index].ShapeCells = InCells;
}

int32 UGridInventoryManagerComponent::FindRegionShapeStorageIndex(const FGameplayTag RegionId, const int32 PocketIndex) const
{
	for (int32 i = 0; i < RegionShapeStorage.Num(); ++i)
	{
		if (RegionShapeStorage[i].RegionId == RegionId && RegionShapeStorage[i].PocketIndex == PocketIndex)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void UGridInventoryManagerComponent::SetRegionTagConstraint(const FGameplayTag RegionId, const FGridRegionTagConstraint& Constraint)
{
	const int32 Index = FindRegionTagConstraintStorageIndex(RegionId);
	if (Index == INDEX_NONE)
	{
		FGridInventoryRegionTagConstraintStorage NewStorage;
		NewStorage.RegionId = RegionId;
		NewStorage.Constraint = Constraint;
		RegionTagConstraintStorage.Add(NewStorage);
		return;
	}
	RegionTagConstraintStorage[Index].Constraint = Constraint;
}

int32 UGridInventoryManagerComponent::FindRegionTagConstraintStorageIndex(const FGameplayTag RegionId) const
{
	for (int32 i = 0; i < RegionTagConstraintStorage.Num(); ++i)
	{
		if (RegionTagConstraintStorage[i].RegionId == RegionId)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

FGridRegionTagConstraint UGridInventoryManagerComponent::GetRegionTagConstraint(const FGameplayTag RegionId) const
{
	const int32 Index = FindRegionTagConstraintStorageIndex(RegionId);
	return Index == INDEX_NONE ? FGridRegionTagConstraint() : RegionTagConstraintStorage[Index].Constraint;
}

bool UGridInventoryManagerComponent::IsRegionCellAvailable(const FGameplayTag RegionId, const int32 PocketIndex, const FIntPoint Tile) const
{
	const int32 FinalPocketIndex = ResolvePlacementPocketIndex(RegionId, PocketIndex);
	const int32 Index = FindRegionShapeStorageIndex(RegionId, FinalPocketIndex);
	if (Index == INDEX_NONE)
	{
		return true;
	}
	return RegionShapeStorage[Index].ShapeCells.Contains(Tile);
}

void UGridInventoryManagerComponent::SyncPrimaryRegionToLegacySlots()
{
	const FGameplayTag PrimaryRegionId = GetPrimaryRegionId();
	const int32 PrimaryPocketIndex = GetPrimaryPocketIndex(PrimaryRegionId);
	const int32 PrimaryIndex = FindRegionSlotsStorageIndex(PrimaryRegionId, PrimaryPocketIndex);
	if (!PrimaryRegionId.IsValid() || PrimaryIndex == INDEX_NONE)
	{
		return;
	}

	InventoryColumns = GetRegionColumns(PrimaryRegionId, PrimaryPocketIndex);
	InventoryRows = GetRegionRows(PrimaryRegionId, PrimaryPocketIndex);
	InventorySlots = RegionSlotsStorage[PrimaryIndex].Slots;
}

void UGridInventoryManagerComponent::BuildRegionsFromCurrentLoadout()
{
	// 重新构建区域状态。
	RegionStates.Reset();
	RegionShapeStorage.Reset();
	RegionTagConstraintStorage.Reset();

	FGridInventoryRegionRuntimeState MainRegion;
	MainRegion.RegionId = TAG_Inventory_Region_Main;
	MainRegion.DisplayName = FText::FromString(TEXT("Main"));
	MainRegion.Priority = 0;
	MainRegion.PocketIndex = 0;
	MainRegion.Columns = InventoryColumns;
	MainRegion.Rows = InventoryRows;
	MainRegion.LayoutOffset = FIntPoint::ZeroValue;
	MainRegion.bEnabled = true;
	RegionStates.Add(MainRegion);
	SetRegionTagConstraint(MainRegion.RegionId, FGridRegionTagConstraint());

	auto AppendRegionDef = [this](const FGridInventoryRegionDefinition& Def)
	{
		if (!Def.RegionId.IsValid())
		{
			return;
		}
		if (Def.Pockets.Num() > 0)
		{
			// 按口袋定义展开运行时区域。
			for (int32 PocketIndex = 0; PocketIndex < Def.Pockets.Num(); ++PocketIndex)
			{
				const FGridInventoryPocketDefinition& PocketDef = Def.Pockets[PocketIndex];
				int32 ResolvedColumns = FMath::Max(1, PocketDef.Dimensions.X);
				int32 ResolvedRows = FMath::Max(1, PocketDef.Dimensions.Y);
				for (const FGridRegionShapeCell& Cell : PocketDef.ShapeCells)
				{
					ResolvedColumns = FMath::Max(ResolvedColumns, Cell.X + 1);
					ResolvedRows = FMath::Max(ResolvedRows, Cell.Y + 1);
				}

				FGridInventoryRegionRuntimeState RuntimeRegion;
				RuntimeRegion.RegionId = Def.RegionId;
				RuntimeRegion.PocketIndex = PocketIndex;
				RuntimeRegion.DisplayName = Def.DisplayName;
				RuntimeRegion.Priority = Def.Priority + PocketDef.Priority;
				RuntimeRegion.Columns = ResolvedColumns;
				RuntimeRegion.Rows = ResolvedRows;
				RuntimeRegion.LayoutOffset = Def.LayoutOffset + PocketDef.LayoutOffset;
				RuntimeRegion.bEnabled = true;
				RegionStates.Add(RuntimeRegion);
				SetRegionTagConstraint(Def.RegionId, Def.ItemTagConstraint);

				if (PocketDef.ShapeCells.Num() > 0)
				{
					TArray<FIntPoint> CellSet;
					for (const FGridRegionShapeCell& Cell : PocketDef.ShapeCells)
					{
						CellSet.Add(Cell.ToPoint());
					}
					SetRegionShapeCells(Def.RegionId, PocketIndex, CellSet);
				}
			}
		}
		else
		{
			int32 ResolvedColumns = FMath::Max(1, Def.Dimensions.X);
			int32 ResolvedRows = FMath::Max(1, Def.Dimensions.Y);
			for (const FGridRegionShapeCell& Cell : Def.ShapeCells)
			{
				ResolvedColumns = FMath::Max(ResolvedColumns, Cell.X + 1);
				ResolvedRows = FMath::Max(ResolvedRows, Cell.Y + 1);
			}

			FGridInventoryRegionRuntimeState RuntimeRegion;
			RuntimeRegion.RegionId = Def.RegionId;
			RuntimeRegion.PocketIndex = 0;
			RuntimeRegion.DisplayName = Def.DisplayName;
			RuntimeRegion.Priority = Def.Priority;
			RuntimeRegion.Columns = ResolvedColumns;
			RuntimeRegion.Rows = ResolvedRows;
			RuntimeRegion.LayoutOffset = Def.LayoutOffset;
			RuntimeRegion.bEnabled = true;
			RegionStates.Add(RuntimeRegion);
			SetRegionTagConstraint(Def.RegionId, Def.ItemTagConstraint);

			if (Def.ShapeCells.Num() > 0)
			{
				TArray<FIntPoint> CellSet;
				for (const FGridRegionShapeCell& Cell : Def.ShapeCells)
				{
					CellSet.Add(Cell.ToPoint());
				}
				SetRegionShapeCells(Def.RegionId, 0, CellSet);
			}
		}
	};

	for (const FGridInventoryRegionDefinition& Def : BaseRegionDefinitions)
	{
		AppendRegionDef(Def);
	}

	if (UYcEquipmentSlotComponent* EquipmentSlotComp = UYcEquipmentSlotComponent::FindEquipmentSlotComponent(GetOwner()))
	{
		// 读取当前装备槽位用于扩展区域。
		const TArray<FYcEquipmentSlot>& EquippedSlots = EquipmentSlotComp->GetSlots();
		for (const FYcEquipmentSlot& EquippedSlot : EquippedSlots)
		{
			if (!IsValid(EquippedSlot.ItemInstance))
			{
				continue;
			}

			TInstancedStruct<FYcInventoryItemFragment> RegionFragmentResult = EquippedSlot.ItemInstance->FindItemFragment(FItemFragment_GridRegions::StaticStruct());
			const FItemFragment_GridRegions* RegionFragment = RegionFragmentResult.GetPtr<FItemFragment_GridRegions>();
			if (!RegionFragment)
			{
				continue;
			}

			for (const FGridInventoryRegionDefinition& Def : RegionFragment->Regions)
			{
				if (!Def.RegionId.IsValid())
				{
					continue;
				}
				AppendRegionDef(Def);
			}
		}
	}

	RegionStates.Sort([](const FGridInventoryRegionRuntimeState& A, const FGridInventoryRegionRuntimeState& B)
	{
		return A.Priority < B.Priority;
	});

	// 区域变化后重建落位映射。
	RebuildRegionSlotsFromPlacementMap();
}

bool UGridInventoryManagerComponent::CanAcceptItemForReturn_Implementation(UYcInventoryItemInstance* ItemInstance, FString& OutReason)
{
	OutReason = TEXT("");
	if (!IsValid(ItemInstance))
	{
		OutReason = TEXT("Return item is null.");
		return false;
	}

	FGameplayTag RegionId;
	int32 PocketIndex = -1;
	FIntPoint Tile = FIntPoint::ZeroValue;
	bool bRotated = false;
	if (!FindFirstFitPlacementForItemInst(ItemInstance, RegionId, PocketIndex, Tile, bRotated))
	{
		OutReason = TEXT("Target inventory has no free grid placement.");
		return false;
	}
	return true;
}

bool UGridInventoryManagerComponent::CanApplyInventoryRelocation_Implementation(const FYcInventoryRelocationRequest& Request, FString& OutReason)
{
	OutReason = TEXT("");
	const FYcInventoryRelocationPayload_ItemScope* Payload = Request.Payload.GetPtr<FYcInventoryRelocationPayload_ItemScope>();
	if (!Payload || !IsValid(Payload->AnchorItem))
	{
		OutReason = TEXT("Inventory relocation payload is invalid or unsupported.");
		return false;
	}

	UYcInventoryItemInstance* EquippedItem = Payload->AnchorItem;
	if (!IsValid(EquippedItem))
	{
		OutReason = TEXT("Inventory relocation anchor item is invalid.");
		return false;
	}

	TArray<FUnequipRelocateMove> RelocateMoves;
	FUnequipRelocateMove EquipMove;
	const bool bCan = TryBuildUnequipRelocationPlan(EquippedItem, RelocateMoves, EquipMove, OutReason);
	if (!bCan)
	{
		InvalidateRelocationPlanCache();
		UE_LOG(LogYcGridInventory, Warning, TEXT("CanApplyInventoryRelocation failed. Item=%s Reason=%s"), *GetNameSafe(Payload->AnchorItem), *OutReason);
	}
	else
	{
		CacheRelocationPlan(EquippedItem, RelocateMoves, EquipMove);
	}
	return bCan;
}

bool UGridInventoryManagerComponent::ApplyInventoryRelocation_Implementation(const FYcInventoryRelocationRequest& Request, FString& OutReason)
{
	OutReason = TEXT("");
	const FYcInventoryRelocationPayload_ItemScope* Payload = Request.Payload.GetPtr<FYcInventoryRelocationPayload_ItemScope>();
	if (!Payload || !IsValid(Payload->AnchorItem))
	{
		OutReason = TEXT("Inventory relocation payload is invalid or unsupported.");
		return false;
	}

	UYcInventoryItemInstance* EquippedItem = Payload->AnchorItem;
	if (!IsValid(EquippedItem))
	{
		OutReason = TEXT("Inventory relocation anchor item is invalid.");
		return false;
	}

	TArray<FUnequipRelocateMove> RelocateMoves;
	FUnequipRelocateMove EquipMove;
	if (!TryGetCachedRelocationPlan(EquippedItem, RelocateMoves, EquipMove) &&
		!TryBuildUnequipRelocationPlan(EquippedItem, RelocateMoves, EquipMove, OutReason))
	{
		InvalidateRelocationPlanCache();
		UE_LOG(LogYcGridInventory, Warning, TEXT("ApplyInventoryRelocation validate failed. Item=%s Reason=%s"), *GetNameSafe(Payload->AnchorItem), *OutReason);
		return false;
	}
	CacheRelocationPlan(EquippedItem, RelocateMoves, EquipMove);

	const TMap<TObjectPtr<UYcInventoryItemInstance>, FItemGridInfo> ItemMapSnapshot = ItemInstanceToTileMap;
	const TArray<FGridInventoryRegionSlotsStorage> RegionSlotsSnapshot = RegionSlotsStorage;
	const int32 RevisionSnapshot = InventoryGridRevision;
	// 先快照，任一步失败都回滚，确保重排应用原子性。
	for (const FUnequipRelocateMove& Move : RelocateMoves)
	{
		if (!IsValid(Move.ItemInstance))
		{
			continue;
		}

		const int32 StackCount = GetStackCountByItemInstance(Move.ItemInstance);
		if (StackCount <= 0 || !OnGridItemInstanceAdded(Move.ItemInstance, StackCount, Move.Tile, Move.bRotated, Move.RegionId, Move.PocketIndex))
		{
			ItemInstanceToTileMap = ItemMapSnapshot;
			RegionSlotsStorage = RegionSlotsSnapshot;
			InventoryGridRevision = RevisionSnapshot;
			SyncPrimaryRegionToLegacySlots();
			InvalidateRelocationPlanCache();
			OnInventoryGridChanged.Broadcast();
			OutReason = TEXT("Unequip relocate apply failed, rolled back.");
			return false;
		}
	}

	CachedTile = EquipMove.Tile;
	CachedRegionId = EquipMove.RegionId;
	CachedPocketIndex = EquipMove.PocketIndex;
	InvalidateRelocationPlanCache();
	return true;
}

bool UGridInventoryManagerComponent::TryGetCachedRelocationPlan(UYcInventoryItemInstance* AnchorItem, TArray<FUnequipRelocateMove>& OutRelocateMoves, FUnequipRelocateMove& OutEquipMove) const
{
	if (!CachedRelocationPlan.bValid)
	{
		return false;
	}
	if (!IsValid(AnchorItem) || CachedRelocationPlan.AnchorItem.Get() != AnchorItem)
	{
		return false;
	}
	if (CachedRelocationPlan.GridRevision != InventoryGridRevision)
	{
		return false;
	}

	OutRelocateMoves = CachedRelocationPlan.RelocateMoves;
	OutEquipMove = CachedRelocationPlan.EquipMove;
	return true;
}

void UGridInventoryManagerComponent::CacheRelocationPlan(UYcInventoryItemInstance* AnchorItem, const TArray<FUnequipRelocateMove>& RelocateMoves, const FUnequipRelocateMove& EquipMove)
{
	CachedRelocationPlan.AnchorItem = AnchorItem;
	CachedRelocationPlan.GridRevision = InventoryGridRevision;
	CachedRelocationPlan.RelocateMoves = RelocateMoves;
	CachedRelocationPlan.EquipMove = EquipMove;
	CachedRelocationPlan.bValid = true;
}

void UGridInventoryManagerComponent::InvalidateRelocationPlanCache()
{
	CachedRelocationPlan = FRelocationPlanCache();
}

bool UGridInventoryManagerComponent::GetProvidedRegionIdsFromItem(UYcInventoryItemInstance* ItemInst, TArray<FGameplayTag>& OutRegionIds) const
{
	OutRegionIds.Reset();
	if (!IsValid(ItemInst))
	{
		return false;
	}

	const TInstancedStruct<FYcInventoryItemFragment> RegionFragmentResult = ItemInst->FindItemFragment(FItemFragment_GridRegions::StaticStruct());
	const FItemFragment_GridRegions* RegionFragment = RegionFragmentResult.GetPtr<FItemFragment_GridRegions>();
	if (!RegionFragment)
	{
		return false;
	}

	for (const FGridInventoryRegionDefinition& Def : RegionFragment->Regions)
	{
		if (Def.RegionId.IsValid())
		{
			OutRegionIds.AddUnique(Def.RegionId);
		}
	}

	return OutRegionIds.Num() > 0;
}

bool UGridInventoryManagerComponent::FindFirstFitPositionInRegionForItemInst(UYcInventoryItemInstance* ItemInst, const FGameplayTag RegionId, const int32 PocketIndex, FIntPoint& Tile, bool& OutRotated)
{
	if (!IsValid(ItemInst))
	{
		return false;
	}
	if (!PassesRegionTagConstraintForItemInst(ItemInst, RegionId))
	{
		return false;
	}

	const FItemFragment_GridItem GridFragment = GetItemFragmentGrid(ItemInst->GetItemRegistryId());
	const int32 Columns = GetRegionColumns(RegionId, PocketIndex);
	const int32 Rows = GetRegionRows(RegionId, PocketIndex);
	for (int32 Y = 0; Y < Rows; ++Y)
	{
		for (int32 X = 0; X < Columns; ++X)
		{
			if (CanPlaceGridItemInst(ItemInst, FIntPoint(X, Y), false, RegionId, PocketIndex))
			{
				Tile = FIntPoint(X, Y);
				OutRotated = false;
				return true;
			}
		}
	}
	if (!GridFragment.bCanRotate)
	{
		return false;
	}
	for (int32 Y = 0; Y < Rows; ++Y)
	{
		for (int32 X = 0; X < Columns; ++X)
		{
			if (CanPlaceGridItemInst(ItemInst, FIntPoint(X, Y), true, RegionId, PocketIndex))
			{
				Tile = FIntPoint(X, Y);
				OutRotated = true;
				return true;
			}
		}
	}
	return false;
}

int32 UGridInventoryManagerComponent::GetGridItemArea(UYcInventoryItemInstance* ItemInst) const
{
	if (!IsValid(ItemInst))
	{
		return 0;
	}
	const FItemFragment_GridItem GridFragment = GetItemFragmentGrid(ItemInst->GetItemRegistryId());
	return FMath::Max(1, GridFragment.Dimensions.X) * FMath::Max(1, GridFragment.Dimensions.Y);
}

bool UGridInventoryManagerComponent::TryFindFitInSimRegion(UYcInventoryItemInstance* ItemInst, const FGameplayTag TargetRegionId, TArray<FUnequipRegionPocketSimState>& SimPockets, int32& OutPocketIndex, FIntPoint& OutTile, bool& OutRotated)
{
	OutPocketIndex = -1;
	OutTile = FIntPoint::ZeroValue;
	OutRotated = false;
	if (!IsValid(ItemInst))
	{
		return false;
	}

	const FItemFragment_GridItem GridFragment = GetItemFragmentGrid(ItemInst->GetItemRegistryId());
	for (int32 RotationPass = 0; RotationPass < 2; ++RotationPass)
	{
		const bool bTryRotated = RotationPass == 1;
		if (bTryRotated && !GridFragment.bCanRotate)
		{
			continue;
		}
		const int32 ItemWidth = bTryRotated ? GridFragment.Dimensions.Y : GridFragment.Dimensions.X;
		const int32 ItemHeight = bTryRotated ? GridFragment.Dimensions.X : GridFragment.Dimensions.Y;
		for (int32 PocketArrayIndex = 0; PocketArrayIndex < SimPockets.Num(); ++PocketArrayIndex)
		{
			FUnequipRegionPocketSimState Pocket = SimPockets[PocketArrayIndex];
			if (!PassesRegionTagConstraintForItemInst(ItemInst, TargetRegionId))
			{
				continue;
			}

			for (int32 Y = 0; Y < Pocket.Rows; ++Y)
			{
				for (int32 X = 0; X < Pocket.Columns; ++X)
				{
					if (X + ItemWidth > Pocket.Columns || Y + ItemHeight > Pocket.Rows)
					{
						continue;
					}

					bool bCanPlace = true;
					for (int32 TestY = Y; TestY < Y + ItemHeight && bCanPlace; ++TestY)
					{
						for (int32 TestX = X; TestX < X + ItemWidth; ++TestX)
						{
							if (!IsRegionCellAvailable(TargetRegionId, Pocket.PocketIndex, FIntPoint(TestX, TestY)))
							{
								bCanPlace = false;
								break;
							}
							const int32 SlotIndex = TileToIndexInRegion(FIntPoint(TestX, TestY), Pocket.Columns);
							if (!Pocket.Slots.IsValidIndex(SlotIndex) || Pocket.Slots[SlotIndex].bOccupied)
							{
								bCanPlace = false;
								break;
							}
						}
					}

					if (!bCanPlace)
					{
						continue;
					}

					for (int32 FillY = Y; FillY < Y + ItemHeight; ++FillY)
					{
						for (int32 FillX = X; FillX < X + ItemWidth; ++FillX)
						{
							const int32 SlotIndex = TileToIndexInRegion(FIntPoint(FillX, FillY), Pocket.Columns);
							Pocket.Slots[SlotIndex].bOccupied = true;
						}
					}

					SimPockets[PocketArrayIndex] = Pocket;
					OutPocketIndex = Pocket.PocketIndex;
					OutTile = FIntPoint(X, Y);
					OutRotated = bTryRotated;
					return true;
				}
			}
		}
	}
	return false;
}

bool UGridInventoryManagerComponent::TryBuildUnequipRelocationPlan(UYcInventoryItemInstance* EquippedItem, TArray<FUnequipRelocateMove>& OutRelocateMoves, FUnequipRelocateMove& OutEquipMove, FString& OutReason)
{
	OutRelocateMoves.Reset();
	OutEquipMove = FUnequipRelocateMove();
	OutReason = TEXT("");

	if (!IsValid(EquippedItem))
	{
		OutReason = TEXT("Unequip failed: equipped item is invalid.");
		return false;
	}

	TArray<FGameplayTag> RegionIdsToDisable;
	if (!GetProvidedRegionIdsFromItem(EquippedItem, RegionIdsToDisable))
	{
		// 该装备不提供额外区域时无需重排。
		return true;
	}

	TArray<UYcInventoryItemInstance*> ItemsToRelocate;
	for (const TPair<TObjectPtr<UYcInventoryItemInstance>, FItemGridInfo>& Entry : ItemInstanceToTileMap)
	{
		if (Entry.Key && RegionIdsToDisable.Contains(Entry.Value.RegionId))
		{
			ItemsToRelocate.Add(Entry.Key);
		}
	}
	ItemsToRelocate.Sort([this](const UYcInventoryItemInstance& A, const UYcInventoryItemInstance& B)
	{
		// 大占地物品优先重排。
		return GetGridItemArea(const_cast<UYcInventoryItemInstance*>(&A)) > GetGridItemArea(const_cast<UYcInventoryItemInstance*>(&B));
	});

	TArray<FGameplayTag> CandidateRegionIds;
	for (const FGridInventoryRegionRuntimeState& State : RegionStates)
	{
		if (!State.bEnabled || RegionIdsToDisable.Contains(State.RegionId))
		{
			continue;
		}
		CandidateRegionIds.AddUnique(State.RegionId);
	}

	for (const FGameplayTag& CandidateRegionId : CandidateRegionIds)
	{
		// 为候选区域构建模拟口袋状态。
		TArray<FUnequipRegionPocketSimState> SimPockets;
		for (const FGridInventoryRegionRuntimeState& State : RegionStates)
		{
			if (!State.bEnabled || State.RegionId != CandidateRegionId || RegionIdsToDisable.Contains(State.RegionId))
			{
				continue;
			}

			FUnequipRegionPocketSimState SimState;
			SimState.PocketIndex = State.PocketIndex;
			SimState.Priority = State.Priority;
			SimState.Columns = State.Columns;
			SimState.Rows = State.Rows;
			SimState.Slots = GetRegionSlots(State.RegionId, State.PocketIndex);
			SimPockets.Add(SimState);
		}
		if (SimPockets.Num() <= 0)
		{
			continue;
		}
		SimPockets.Sort([](const FUnequipRegionPocketSimState& A, const FUnequipRegionPocketSimState& B)
		{
			return A.Priority < B.Priority;
		});

		int32 EquipPocketIndex = -1;
		FIntPoint EquipTile = FIntPoint::ZeroValue;
		bool bEquipRotated = false;
		if (!TryFindFitInSimRegion(EquippedItem, CandidateRegionId, SimPockets, EquipPocketIndex, EquipTile, bEquipRotated))
		{
			continue;
		}

		TArray<FUnequipRelocateMove> CandidateMoves;
		bool bPlanValid = true;
		for (UYcInventoryItemInstance* ItemToRelocate : ItemsToRelocate)
		{
			int32 MovePocketIndex = -1;
			FIntPoint MoveTile = FIntPoint::ZeroValue;
			bool bMoveRotated = false;
			if (!TryFindFitInSimRegion(ItemToRelocate, CandidateRegionId, SimPockets, MovePocketIndex, MoveTile, bMoveRotated))
			{
				bPlanValid = false;
				break;
			}

			FUnequipRelocateMove Move;
			Move.ItemInstance = ItemToRelocate;
			Move.RegionId = CandidateRegionId;
			Move.PocketIndex = MovePocketIndex;
			Move.Tile = MoveTile;
			Move.bRotated = bMoveRotated;
			CandidateMoves.Add(Move);
		}

		if (!bPlanValid)
		{
			continue;
		}

		OutRelocateMoves = CandidateMoves;
		OutEquipMove.ItemInstance = EquippedItem;
		OutEquipMove.RegionId = CandidateRegionId;
		OutEquipMove.PocketIndex = EquipPocketIndex;
		OutEquipMove.Tile = EquipTile;
		OutEquipMove.bRotated = bEquipRotated;
		return true;
	}

	OutReason = TEXT("Unequip blocked: target region has no enough free space for equipment item and provided-region items.");
	return false;
}

void UGridInventoryManagerComponent::DebugPrintSlots()
{
	FString DebugStr = TEXT("\nGrid State:\n");
	UE_LOG(LogYcGridInventory, Log, TEXT("Grid inventory size: %d"), InventorySlots.Num());
	for (int32 i = 0; i < InventorySlots.Num(); ++i)
	{
		if (i % FMath::Max(1, InventoryColumns) == 0)
		{
			DebugStr.Append(TEXT("\n"));
		}
		DebugStr.Append(InventorySlots[i].bOccupied ? TEXT("1 / ") : TEXT("0 / "));
	}
	UE_LOG(LogYcGridInventory, Log, TEXT("%s"), *DebugStr);
}
