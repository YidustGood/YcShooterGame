// 单个网格槽位数据：记录占用状态、所属物品与相对偏移
USTRUCT()
struct FGridInventorySlot
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bOccupied = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FYcItemInstanceId OccupyingItemID;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ItemRelativeX;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ItemRelativeY;
	UPROPERTY()
	UYcInventoryItemInstance ItemInstance;

	void Reset()
	{
		bOccupied = false;
		OccupyingItemID = FYcItemInstanceId();
		ItemRelativeX = 0;
		ItemRelativeY = 0;
		ItemInstance = nullptr;
	}
};

// 物品在网格中的左上角坐标与尺寸缓存
USTRUCT()
struct FItemGridInfo
{
	FItemGridInfo(FIntPoint InTilePos, FIntPoint InItemSize)
	{
		TilePos = InTilePos;
		ItemSize = InItemSize;
		RegionId = FGameplayTag();
		PocketIndex = 0;
	}
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint TilePos;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint ItemSize;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag RegionId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PocketIndex = 0;
};

USTRUCT()
struct FGridInventoryRegionRuntimeState
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag RegionId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PocketIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Priority = 100;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Columns = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Rows = 1;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint LayoutOffset = FIntPoint(0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bEnabled = true;
};

// 区域一个口袋的槽位数据
USTRUCT()
struct FGridInventoryRegionSlotsStorage
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag RegionId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PocketIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGridInventorySlot> Slots;
};

// 区域一个口袋的形状数据
USTRUCT()
struct FGridInventoryRegionShapeStorage
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag RegionId;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PocketIndex = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FIntPoint> ShapeCells;
};

// 区域标签约束
USTRUCT()
struct FGridInventoryRegionTagConstraintStorage
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag RegionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGridRegionTagConstraint Constraint;
};

// 与装备槽绑定的区域ID列表
USTRUCT()
struct FEquipmentSlotRegionBinding
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag SlotTag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGameplayTag> RegionIds;
};

USTRUCT()
struct FUnequipRelocateMove
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UYcInventoryItemInstance ItemInstance;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag RegionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PocketIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint Tile = FIntPoint(0, 0);

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bRotated = false;
};

USTRUCT()
struct FUnequipRegionPocketSimState
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 PocketIndex = -1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Priority = 9999;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Columns = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Rows = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGridInventorySlot> Slots;
};
event void FInventoryGridChanged();

// @TODO 后续可以考虑迁移到C++层实现以获得更好的性能
// 网格库存核心组件：负责格子占用、物品放置、容器搜索会话与服务端交换校验
// 网格库存架构：
// 按照区域划分网格区域，例如胸挂区域、背包区域、安全箱区域用Tag标识区分，可动态增删区域。每个区域包含一个或多个口袋，每个口袋包含一个或多个物品槽位，物品根据物品大小占用对应数量的物品槽位。
class UGridInventoryManagerComponent : UYcInventoryManagerComponent
{
	// 基础区域定义
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory|Regions")
	TArray<FGridInventoryRegionDefinition> BaseRegionDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 InventoryColumns = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 InventoryRows = 10;
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Search")
	bool bEnableContainerSearch = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	bool bAllowDirectContainerInteraction = false;
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FGridInventorySlot> InventorySlots;
	UPROPERTY(Replicated)
	int32 InventoryGridRevision = 0;
	TMap<UYcInventoryItemInstance, FItemGridInfo> ItemInstanceToTileMap;

	// 运行时区域状态
	UPROPERTY(Replicated, VisibleAnywhere, BlueprintReadOnly, Category = "Inventory|Regions")
	TArray<FGridInventoryRegionRuntimeState> RegionStates;
	UPROPERTY(VisibleAnywhere, Category = "Inventory|Regions")
	private TArray<FGridInventoryRegionSlotsStorage> RegionSlotsStorage;
	UPROPERTY(VisibleAnywhere, Category = "Inventory|Regions")
	private TArray<FGridInventoryRegionShapeStorage> RegionShapeStorage;
	UPROPERTY(VisibleAnywhere, Category = "Inventory|Regions")
	private TArray<FGridInventoryRegionTagConstraintStorage> RegionTagConstraintStorage;
	UPROPERTY(VisibleAnywhere, Category = "Inventory|Regions")
	private TArray<FEquipmentSlotRegionBinding> EquipmentSlotRegionBindings;
	UPROPERTY(VisibleAnywhere, Category = "Inventory|Regions")
	private TOptional<FGameplayTag> CachedRegionId;
	UPROPERTY(VisibleAnywhere, Category = "Inventory|Regions")
	private TOptional<int32> CachedPocketIndex;

	UPROPERTY()
	FInventoryGridChanged OnInventoryGridChanged;

	FGameplayMessageListenerHandle InventoryChangedHandle;
	FGameplayMessageListenerHandle EquipmentSlotChangedHandle;

	private FIntPoint CurrentHandleTile;
	private FGameplayTag CurrentHandleRegionId;
	private FGameplayTag CurrentExpectedSourceRegionId;
	private int32 CurrentHandlePocketIndex = -1;
	private int32 CurrentExpectedSourcePocketIndex = -1;

	TOptional<FIntPoint> CachedTile;
	UPROPERTY(Replicated)
	bool bSearchSessionActive = false;
	UPROPERTY(Replicated)
	UGridInventoryManagerComponent SearchContainerInventory;
	UPROPERTY(Replicated)
	TArray<UYcInventoryItemInstance> RevealedSearchItems;
	UPROPERTY(Replicated)
	TArray<UYcInventoryItemInstance> KnownSearchItems;
	UPROPERTY(Replicated)
	UYcInventoryItemInstance CurrentSearchingItem;
	UPROPERTY(Replicated)
	float CurrentSearchProgress01 = 0.0f;
	UPROPERTY(Replicated)
	int32 SearchTotalItemCount = 0;

	UPROPERTY(Replicated)
	int32 SearchRevealedItemCount = 0;
	UPROPERTY(Replicated)
	int32 SearchSessionRevision = 0;
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Search")
	float SearchSpeedMultiplier = 1.0f;

	// 待搜索队列（按容器格子顺序组织）
	private TArray<UYcInventoryItemInstance> PendingSearchQueue;
	private float CurrentSearchTargetDuration = 0.0f;
	private float SearchTickInterval = 0.1f;

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		InitializeInventory();
		if (GetOwner().HasAuthority())
		{
			auto Router = UYcInventoryOperationRouterComponent::FindOrCreateRouter(GetOwner());
			if (Router != nullptr)
			{
				// 向 Router 注册通用脚本处理器，Router 不感知具体业务类型。
				Router.RegisterScriptOperationHandler(n"Inventory.SwapGrid", this, n"ValidateSwapLikeOperation", n"ExecuteSwapLikeOperation", n"BuildSwapLikeOperationDelta", false, 0);
				Router.RegisterScriptOperationHandler(n"Container.", this, n"ValidateSwapLikeOperation", n"ExecuteSwapLikeOperation", n"BuildSwapLikeOperationDelta", true, -1);
				Router.RegisterScriptOperationHandler(n"Search.", this, n"ValidateSwapLikeOperation", n"ExecuteSwapLikeOperation", n"BuildSwapLikeOperationDelta", true, -1);
			}
		}
		if (GetOwner().HasAuthority())
		{
			InventoryChangedHandle = UGameplayMessageSubsystem::Get().RegisterListener(
				GameplayTags::Yc_Inventory_Message_StackChanged,
				this,
				n"OnInventoryChanged",
				FYcInventoryItemChangeMessage(),
				EGameplayMessageMatch::ExactMatch);
		}
		EquipmentSlotChangedHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			GameplayTags::Yc_EquipmentSlot_Message_SlotChanged,
			this,
			n"OnEquipmentSlotChanged",
			FYcEquipmentSlotChangedMessage(),
			EGameplayMessageMatch::ExactMatch);
	}

	UFUNCTION(BlueprintOverride)
	void EndPlay(EEndPlayReason EndPlayReason)
	{
		InventoryChangedHandle.Unregister();
		EquipmentSlotChangedHandle.Unregister();
		if (GetOwner().HasAuthority())
		{
			auto Router = UYcInventoryOperationRouterComponent::FindRouter(GetOwner());
			if (Router != nullptr)
			{
				Router.UnregisterScriptOperationHandler(n"Inventory.SwapGrid", this);
				Router.UnregisterScriptOperationHandler(n"Container.", this);
				Router.UnregisterScriptOperationHandler(n"Search.", this);
			}
			ResetSearchSession_Internal();
		}
	}

	UFUNCTION()
	void OnEquipmentSlotChanged(FGameplayTag ActualTag, FYcEquipmentSlotChangedMessage Data)
	{
		if (!IsOwnerActorMatched(Data.Owner, GetOwner()))
		{
			return;
		}

		BuildRegionsFromCurrentLoadout();
		SyncPrimaryRegionToLegacySlots();
		InventoryGridRevision++;
		OnInventoryGridChanged.Broadcast();
	}

	private bool IsOwnerActorMatched(AActor MessageOwner, AActor LocalOwner) const
	{
		if (MessageOwner == nullptr || LocalOwner == nullptr)
		{
			return false;
		}
		if (MessageOwner == LocalOwner)
		{
			return true;
		}

		APawn MessagePawn = Cast<APawn>(MessageOwner);
		AController MessageController = Cast<AController>(MessageOwner);
		if (MessageController != nullptr)
		{
			MessagePawn = MessageController.ControlledPawn;
		}
		else if (MessagePawn != nullptr)
		{
			MessageController = Cast<AController>(MessagePawn.GetController());
		}

		APawn LocalPawn = Cast<APawn>(LocalOwner);
		AController LocalController = Cast<AController>(LocalOwner);
		if (LocalController != nullptr)
		{
			LocalPawn = LocalController.ControlledPawn;
		}
		else if (LocalPawn != nullptr)
		{
			LocalController = Cast<AController>(LocalPawn.GetController());
		}

		if (MessagePawn != nullptr && LocalPawn != nullptr && MessagePawn == LocalPawn)
		{
			return true;
		}
		if (MessageController != nullptr && LocalController != nullptr && MessageController == LocalController)
		{
			return true;
		}
		return false;
	}
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeInventory()
	{
		BuildRegionsFromCurrentLoadout();
		InventorySlots.SetNum(InventoryColumns * InventoryRows);
		for (int32 i = 0; i < InventorySlots.Num(); i++)
		{
			InventorySlots[i].Reset();
		}
		SyncPrimaryRegionToLegacySlots();
	}
	UFUNCTION()
	void OnInventoryChanged(FGameplayTag ActualTag, FYcInventoryItemChangeMessage Data)
	{
		if (Data.InventoryOwner == this)
		{
			if (Data.Delta > 0)
			{
				if (CachedTile.IsSet())
				{
					OnGridItemInstanceAdded(
						Data.ItemInstance,
						Data.NewCount,
						CachedTile.GetValue(),
						false,
						CachedRegionId.IsSet() ? CachedRegionId.GetValue() : FGameplayTag(),
						CachedPocketIndex.IsSet() ? CachedPocketIndex.GetValue() : 0);
					CachedTile.Reset();
					CachedRegionId.Reset();
					CachedPocketIndex.Reset();
					return;
				}
				bool bRotated;
				FIntPoint Tile;
				FGameplayTag RegionId;
				int32 PocketIndex = 0;
				if (FindFirstFitPlacementForItemInst(Data.ItemInstance, RegionId, PocketIndex, Tile, bRotated))
				{
					OnGridItemInstanceAdded(Data.ItemInstance, Data.NewCount, Tile, bRotated, RegionId, PocketIndex);
				}
				else
				{
					Warning("添加物品失败：没有可用位置。");
				}
			}
			else if (Data.NewCount == 0)
			{
				OnRemoveGridItem(Data.ItemInstance);
			}
			return;
		}

		// 当前玩家正在搜索的容器发生变化（多人并发取放），需要实时重建待搜索队列
		if (bSearchSessionActive && SearchContainerInventory != nullptr && Data.InventoryOwner == SearchContainerInventory)
		{
			RebuildSearchQueueFromContainerPreserveCurrent();
		}
	}
	UFUNCTION(BlueprintCallable, Category = "Inventory", BlueprintAuthorityOnly)
	bool TryAddGridItemByDefinition(FDataRegistryId ItemDefId, int32 StackCount, FIntPoint Tile, bool bRotated = false, FGameplayTag RegionId = FGameplayTag(), int32 PocketIndex = -1)
	{
		if (!CanPlaceGridItem(ItemDefId, Tile, bRotated, RegionId, PocketIndex))
		{
			return false;
		}

		CachedTile.Set(Tile);
		FGameplayTag FinalRegionId = ResolvePlacementRegionId(RegionId);
		CachedRegionId.Set(FinalRegionId);
		CachedPocketIndex.Set(ResolvePlacementPocketIndex(FinalRegionId, PocketIndex));
		auto ItemInstance = AddItem(ItemDefId, StackCount);
		if (ItemInstance == nullptr)
		{
			CachedTile.Reset();
			CachedRegionId.Reset();
			CachedPocketIndex.Reset();
			Error("ItemInstance is invalid!");
			return false;
		}
		return true;
	}
	UFUNCTION(BlueprintCallable, Category = "Inventory", BlueprintAuthorityOnly)
	bool TryAddGridItemInstance(UYcInventoryItemInstance ItemInst, int32 StackCount, FIntPoint Tile, bool bRotated = false, FGameplayTag RegionId = FGameplayTag(), int32 PocketIndex = -1)
	{
		check(ItemInst != nullptr, "ItemInst is nullptr");

		if (!CanPlaceGridItem(ItemInst.ItemRegistryId, Tile, bRotated, RegionId, PocketIndex))
		{
			return false;
		}

		CachedTile.Set(Tile);
		FGameplayTag FinalRegionId = ResolvePlacementRegionId(RegionId);
		CachedRegionId.Set(FinalRegionId);
		CachedPocketIndex.Set(ResolvePlacementPocketIndex(FinalRegionId, PocketIndex));
		AddItemInstance(ItemInst, StackCount);
		return true;
	}
	UFUNCTION(BlueprintCallable, Category = "Inventory", BlueprintAuthorityOnly)
	bool RemoveGridItem(UYcInventoryItemInstance ItemInst)
	{
		bool bValidRemove = ItemInstanceToTileMap.Contains(ItemInst);
		OnRemoveGridItem(ItemInst);
		RemoveItemInstance(ItemInst);
		return bValidRemove;
	}
	UFUNCTION(BlueprintCallable, Category = "Inventory", BlueprintAuthorityOnly)
	bool OnGridItemInstanceAdded(UYcInventoryItemInstance ItemInst, int32 StackCount, FIntPoint Tile, bool bRotated = false, FGameplayTag RegionId = FGameplayTag(), int32 PocketIndex = -1)
	{
		check(ItemInst != nullptr, "ItemInst is nullptr");

		FGameplayTag FinalRegionId = ResolvePlacementRegionId(RegionId);
		int32 FinalPocketIndex = ResolvePlacementPocketIndex(FinalRegionId, PocketIndex);
		if (!CanPlaceGridItemInst(ItemInst, Tile, bRotated, FinalRegionId, FinalPocketIndex))
		{
			return false;
		}
		auto IF_Grid = GetItemFragmentGrid(ItemInst.ItemRegistryId);
		int32 ItemWidth = bRotated ? IF_Grid.Dimensions.Y : IF_Grid.Dimensions.X;
		int32 ItemHeight = bRotated ? IF_Grid.Dimensions.X : IF_Grid.Dimensions.Y;

		if (ItemInstanceToTileMap.Contains(ItemInst))
		{
			OnRemoveGridItem(ItemInst);
		}
		FItemGridInfo ItemInfo = FItemGridInfo(Tile, FIntPoint(ItemWidth, ItemHeight));
		ItemInfo.RegionId = FinalRegionId;
		ItemInfo.PocketIndex = FinalPocketIndex;
		ItemInstanceToTileMap.Add(ItemInst, ItemInfo);
		auto RegionSlots = GetRegionSlots(FinalRegionId, FinalPocketIndex);
		int32 Columns = GetRegionColumns(FinalRegionId, FinalPocketIndex);
		for (int32 Y = Tile.Y; Y < Tile.Y + ItemHeight; Y++)
		{
			for (int32 X = Tile.X; X < Tile.X + ItemWidth; X++)
			{
				int32 Index = TileToIndexInRegion(FIntPoint(X, Y), Columns);
				RegionSlots[Index].bOccupied = true;
				RegionSlots[Index].OccupyingItemID = ItemInst.GetItemInstId();
				RegionSlots[Index].ItemInstance = ItemInst;
				RegionSlots[Index].ItemRelativeX = X - Tile.X;
				RegionSlots[Index].ItemRelativeY = Y - Tile.Y;
			}
		}
		SetRegionSlots(FinalRegionId, FinalPocketIndex, RegionSlots);
		InventoryGridRevision++;
		SyncPrimaryRegionToLegacySlots();
		OnInventoryGridChanged.Broadcast();
		return true;
	}
	UFUNCTION(BlueprintCallable, Category = "Inventory", BlueprintAuthorityOnly)
	void OnRemoveGridItem(UYcInventoryItemInstance ItemInst)
	{
		if (!ItemInstanceToTileMap.Contains(ItemInst))
		{
			return;
		}
		auto ItemGridInfo = ItemInstanceToTileMap[ItemInst];
		auto RegionSlots = GetRegionSlots(ItemGridInfo.RegionId, ItemGridInfo.PocketIndex);
		int32 Columns = GetRegionColumns(ItemGridInfo.RegionId, ItemGridInfo.PocketIndex);
		for (int X = ItemGridInfo.TilePos.X; X < ItemGridInfo.TilePos.X + ItemGridInfo.ItemSize.X; X++)
		{
			for (int Y = ItemGridInfo.TilePos.Y; Y < ItemGridInfo.TilePos.Y + ItemGridInfo.ItemSize.Y; Y++)
			{
				int32 Index = TileToIndexInRegion(FIntPoint(X, Y), Columns);
				if (RegionSlots.IsValidIndex(Index))
				{
					RegionSlots[Index].Reset();
				}
			}
		}
		SetRegionSlots(ItemGridInfo.RegionId, ItemGridInfo.PocketIndex, RegionSlots);
		ItemInstanceToTileMap.Remove(ItemInst);
		InventoryGridRevision++;
		SyncPrimaryRegionToLegacySlots();
		OnInventoryGridChanged.Broadcast();
	}

	private bool TryGetItemPlacementInfo(UYcInventoryItemInstance ItemInst, FIntPoint&out OutTile, bool&out bOutRotated)
	{
		FGameplayTag UnusedRegionId;
		int32 UnusedPocketIndex = -1;
		return TryGetItemPlacementInfoWithRegion(ItemInst, UnusedRegionId, UnusedPocketIndex, OutTile, bOutRotated);
	}

	private bool TryGetItemPlacementInfoWithRegion(UYcInventoryItemInstance ItemInst, FGameplayTag&out OutRegionId, int32&out OutPocketIndex, FIntPoint&out OutTile, bool&out bOutRotated)
	{
		OutRegionId = FGameplayTag();
		OutPocketIndex = -1;
		if (ItemInst == nullptr || !ItemInstanceToTileMap.Contains(ItemInst))
		{
			return false;
		}

		auto ItemGridInfo = ItemInstanceToTileMap[ItemInst];
		OutRegionId = ItemGridInfo.RegionId;
		OutPocketIndex = ItemGridInfo.PocketIndex;
		OutTile = ItemGridInfo.TilePos;
		auto IF_Grid = GetItemFragmentGrid(ItemInst.ItemRegistryId);
		bOutRotated = (ItemGridInfo.ItemSize.X == IF_Grid.Dimensions.Y && ItemGridInfo.ItemSize.Y == IF_Grid.Dimensions.X);
		return true;
	}

	private bool IsRegionStateEnabled(FGameplayTag RegionId, int32 PocketIndex) const
	{
		for (int32 i = 0; i < RegionStates.Num(); i++)
		{
			if (RegionStates[i].bEnabled && RegionStates[i].RegionId == RegionId && RegionStates[i].PocketIndex == PocketIndex)
			{
				return true;
			}
		}
		return false;
	}

	private bool CommitPlacementWithoutBroadcast(UYcInventoryItemInstance ItemInst, FIntPoint Tile, bool bRotated, FGameplayTag RegionId, int32 PocketIndex)
	{
		// 仅落地网格占用与映射，不触发版本号和广播；用于批量重建阶段。
		if (ItemInst == nullptr)
		{
			return false;
		}
		if (!CanPlaceGridItemInst(ItemInst, Tile, bRotated, RegionId, PocketIndex))
		{
			return false;
		}

		auto IF_Grid = GetItemFragmentGrid(ItemInst.ItemRegistryId);
		int32 ItemWidth = bRotated ? IF_Grid.Dimensions.Y : IF_Grid.Dimensions.X;
		int32 ItemHeight = bRotated ? IF_Grid.Dimensions.X : IF_Grid.Dimensions.Y;
		FItemGridInfo ItemInfo = FItemGridInfo(Tile, FIntPoint(ItemWidth, ItemHeight));
		ItemInfo.RegionId = RegionId;
		ItemInfo.PocketIndex = PocketIndex;
		ItemInstanceToTileMap.Add(ItemInst, ItemInfo);

		auto RegionSlots = GetRegionSlots(RegionId, PocketIndex);
		int32 Columns = GetRegionColumns(RegionId, PocketIndex);
		for (int32 Y = Tile.Y; Y < Tile.Y + ItemHeight; Y++)
		{
			for (int32 X = Tile.X; X < Tile.X + ItemWidth; X++)
			{
				int32 Index = TileToIndexInRegion(FIntPoint(X, Y), Columns);
				RegionSlots[Index].bOccupied = true;
				RegionSlots[Index].OccupyingItemID = ItemInst.GetItemInstId();
				RegionSlots[Index].ItemInstance = ItemInst;
				RegionSlots[Index].ItemRelativeX = X - Tile.X;
				RegionSlots[Index].ItemRelativeY = Y - Tile.Y;
			}
		}
		SetRegionSlots(RegionId, PocketIndex, RegionSlots);
		return true;
	}

	private void RebuildRegionSlotsFromPlacementMap()
	{
		// 根据当前 ItemInstanceToTileMap 重新生成所有区域槽位缓存。
		// 若原位置不可用，则尝试自动找新位置；仍失败则从映射中移除避免脏数据。
		RegionSlotsStorage.Empty();

		TArray<UYcInventoryItemInstance> Items;
		for (auto It : ItemInstanceToTileMap)
		{
			Items.Add(It.Key);
		}

		TArray<UYcInventoryItemInstance> ItemsToRemove;
		for (int32 i = 0; i < Items.Num(); i++)
		{
			auto ItemInst = Items[i];
			if (ItemInst == nullptr || !ItemInstanceToTileMap.Contains(ItemInst))
			{
				continue;
			}

			auto OldInfo = ItemInstanceToTileMap[ItemInst];
			auto IF_Grid = GetItemFragmentGrid(ItemInst.ItemRegistryId);
			bool bOldRotated = (OldInfo.ItemSize.X == IF_Grid.Dimensions.Y && OldInfo.ItemSize.Y == IF_Grid.Dimensions.X);
			bool bPlaced = false;

			if (IsRegionStateEnabled(OldInfo.RegionId, OldInfo.PocketIndex))
			{
				// 优先尝试“原区域+原口袋+原朝向”，尽量保持玩家已有布局。
				bPlaced = CommitPlacementWithoutBroadcast(ItemInst, OldInfo.TilePos, bOldRotated, OldInfo.RegionId, OldInfo.PocketIndex);
			}

			if (!bPlaced)
			{
				FGameplayTag NewRegionId;
				int32 NewPocketIndex = -1;
				FIntPoint NewTile;
				bool bNewRotated = false;
				if (FindFirstFitPlacementForItemInst(ItemInst, NewRegionId, NewPocketIndex, NewTile, bNewRotated))
				{
					bPlaced = CommitPlacementWithoutBroadcast(ItemInst, NewTile, bNewRotated, NewRegionId, NewPocketIndex);
				}
			}

			if (!bPlaced)
			{
				ItemsToRemove.Add(ItemInst);
				Warning("RebuildRegionSlotsFromPlacementMap: item has no valid placement and was removed from grid map.");
			}
		}

		for (int32 i = 0; i < ItemsToRemove.Num(); i++)
		{
			ItemInstanceToTileMap.Remove(ItemsToRemove[i]);
		}
	}
	UFUNCTION(BlueprintCallable, Category = "Search")
	float GetSearchSpeedMultiplier() const
	{
		return Math::Max(0.01f, SearchSpeedMultiplier);
	}

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Search")
	void SetSearchSpeedMultiplier(float NewValue)
	{
		SearchSpeedMultiplier = Math::Max(0.01f, NewValue);
	}

	UFUNCTION(BlueprintCallable, Category = "Search")
	bool IsItemRevealedForCurrentSession(UYcInventoryItemInstance ItemInst) const
	{
		if (ItemInst == nullptr)
		{
			return false;
		}

		UGridInventoryManagerComponent ItemOuterInventory = ItemInst.GetActorOuter().GetComponentByClass(UGridInventoryManagerComponent);
		if (ItemOuterInventory == nullptr)
		{
			return true;
		}

		return IsItemRevealedForContainerSession(ItemOuterInventory, ItemInst);
	}

	UFUNCTION(BlueprintCallable, Category = "Search")
	bool IsItemOperableForCurrentSession(UYcInventoryItemInstance ItemInst) const
	{
		return IsItemRevealedForCurrentSession(ItemInst);
	}

	UFUNCTION(BlueprintCallable, Category = "Search")
	bool GetCurrentSearchProgress(float&out OutProgress01, float&out OutRemainingSeconds, int32&out OutRevealedCount, int32&out OutTotalCount) const
	{
		OutProgress01 = CurrentSearchProgress01;
		OutRemainingSeconds = 0.0f;
		OutRevealedCount = SearchRevealedItemCount;
		OutTotalCount = SearchTotalItemCount;

		if (!bSearchSessionActive || SearchContainerInventory == nullptr)
		{
			return false;
		}
		if (SearchTotalItemCount <= 0)
		{
			return false;
		}

		if (CurrentSearchingItem != nullptr)
		{
			OutRemainingSeconds = (1.0f - Math::Clamp(CurrentSearchProgress01, 0.0f, 1.0f)) * CurrentSearchTargetDuration;
		}
		return true;
	}

	UFUNCTION(BlueprintCallable, Category = "Search")
	bool GetSearchSessionRevisionForContainer(UGridInventoryManagerComponent ContainerInventory, int32&out OutRevision) const
	{
		OutRevision = -1;
		if (!bSearchSessionActive || SearchContainerInventory == nullptr || SearchContainerInventory != ContainerInventory)
		{
			return false;
		}
		OutRevision = SearchSessionRevision;
		return true;
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetInventoryGridRevision() const
	{
		return InventoryGridRevision;
	}

	UFUNCTION(BlueprintCallable, Category = "ContextMenu")
	bool GetContextMenuActionsForItem(UYcInventoryItemInstance ItemInst, TArray<FGridItemContextMenuAction>&out OutActions)
	{
		OutActions.Empty();
		if (ItemInst == nullptr)
		{
			return false;
		}

		FInstancedStruct Result = ItemInst.FindItemFragment(FItemFragment_ContextMenu);
		if (!Result.IsValid())
		{
			return false;
		}

		auto MenuFragment = Result.Get(FItemFragment_ContextMenu);
		if (MenuFragment.Actions.Num() <= 0)
		{
			return false;
		}

		TArray<FGameplayTag> SeenActionTags;
		for (int32 i = 0; i < MenuFragment.Actions.Num(); i++)
		{
			FGridItemContextMenuAction ActionDef = MenuFragment.Actions[i];
			if (!ActionDef.ActionTag.IsValid())
			{
				continue;
			}
			if (SeenActionTags.Contains(ActionDef.ActionTag))
			{
				Warning("ContextMenu action tag duplicated on same item, later one ignored.");
				continue;
			}
			SeenActionTags.Add(ActionDef.ActionTag);

			FText DisabledReason;
			bool bCanExecute = EvaluateContextActionExecutability(ItemInst, ActionDef, DisabledReason);
			ActionDef.bRuntimeCanExecute = bCanExecute;
			ActionDef.RuntimeDisabledReason = DisabledReason;
			if (!bCanExecute && !ActionDef.bShowWhenDisabled)
			{
				continue;
			}

			int32 InsertIndex = OutActions.Num();
			for (int32 ExistingIndex = 0; ExistingIndex < OutActions.Num(); ExistingIndex++)
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

	UFUNCTION(BlueprintCallable, Category = "ContextMenu")
	bool CanExecuteContextAction(UYcInventoryItemInstance ItemInst, FGameplayTag ActionTag, FGridItemContextMenuAction&out OutActionDef)
	{
		if (!TryFindContextMenuActionDef(ItemInst, ActionTag, OutActionDef))
		{
			return false;
		}
		FText DisabledReason;
		bool bCanExecute = EvaluateContextActionExecutability(ItemInst, OutActionDef, DisabledReason);
		OutActionDef.bRuntimeCanExecute = bCanExecute;
		OutActionDef.RuntimeDisabledReason = DisabledReason;
		return bCanExecute;
	}

	UFUNCTION(Server)
	void ServerRequestExecuteItemContextAction(UYcInventoryItemInstance ItemInst, FGameplayTag ActionTag)
	{
		if (ItemInst == nullptr || !ActionTag.IsValid())
		{
			return;
		}
		if (!CanExecuteContextActionInAuthoritativeState(ItemInst))
		{
			Warning("Reject context action: authoritative item state check failed.");
			return;
		}

		FGridItemContextMenuAction ActionDef;
		if (!CanExecuteContextAction(ItemInst, ActionTag, ActionDef))
		{
			Warning("Reject context action: item/action is not executable.");
			return;
		}

		FGridItemContextActionRequest Request;
		Request.Player = GetOwner();
		Request.ItemInst = ItemInst;
		Request.ActionTag = ActionDef.ActionTag;
		Request.ExecutorTag = ActionDef.ExecutorTag;
		Request.EventTag = ActionDef.EventTag;

		FGameplayTag RequestMessageTag = FGameplayTag::RequestGameplayTag(n"Yc.Inventory.Message.Grid.ContextAction.Request");
		UGameplayMessageSubsystem::Get().BroadcastMessage(RequestMessageTag, Request);

		FGameplayTag MessageExecutorTag = FGameplayTag::RequestGameplayTag(n"Yc.Inventory.ContextAction.Executor.Message");
		FGameplayTag GasExecutorTag = FGameplayTag::RequestGameplayTag(n"Yc.Inventory.ContextAction.Executor.GAS");

		// 1) Message executor: broadcast to action event tag for gameplay layer subscribers.
		if (ActionDef.ExecutorTag == MessageExecutorTag)
		{
			if (ActionDef.EventTag.IsValid())
			{
				UGameplayMessageSubsystem::Get().BroadcastMessage(ActionDef.EventTag, Request);
			}
			return;
		}

		// 2) GAS executor: dispatch gameplay event to the owner actor.
		if (ActionDef.ExecutorTag == GasExecutorTag)
		{
			if (!ActionDef.EventTag.IsValid())
			{
				Warning("Reject context action: GAS executor requires valid EventTag.");
				return;
			}

			FGameplayEventData Payload;
			Payload.EventTag = ActionDef.EventTag;
			Payload.Instigator = GetOwner();
			Payload.Target = ItemInst.GetActorOuter();
			Payload.OptionalObject = ItemInst;
			AbilitySystem::SendGameplayEventToActor(GetOwner(), ActionDef.EventTag, Payload);
			return;
		}

		// 3) Unknown executor: keep only the raw request broadcast for custom gameplay-side handling.
		Warning("Unknown context action executor tag, request broadcast only.");
	}

	// 服务端最终校验：防止客户端因网络延迟对“已被他人取走/转移”的旧物品发起右键操作。
	private bool CanExecuteContextActionInAuthoritativeState(UYcInventoryItemInstance ItemInst) const
	{
		if (ItemInst == nullptr)
		{
			return false;
		}

		UGridInventoryManagerComponent ItemOuterInventory = ItemInst.GetActorOuter().GetComponentByClass(UGridInventoryManagerComponent);
		if (ItemOuterInventory == nullptr)
		{
			// 物品已不在任何网格库存上下文中（可能已销毁/已转移到其他系统），拒绝执行。
			return false;
		}

		// 1) 物品在自己的背包里：允许（仍会走动作策略校验）。
		if (ItemOuterInventory == this)
		{
			return GetStackCountByItemInstance(ItemInst) > 0;
		}

		// 2) 物品在搜索容器里：必须仍属于当前会话容器，且该物品当前仍在容器且已揭示。
		if (bSearchSessionActive && SearchContainerInventory != nullptr && ItemOuterInventory == SearchContainerInventory)
		{
			if (!IsItemStillInContainer(ItemInst, SearchContainerInventory))
			{
				return false;
			}
			return IsItemRevealedForContainerSession(ItemOuterInventory, ItemInst);
		}

		// 3) 其他情况（例如在其他玩家背包）一律拒绝。
		return false;
	}

	UFUNCTION(Server)
	void ServerStartContainerSearchSession(UGridInventoryManagerComponent ContainerInventory)
	{
		// 每次打开容器都会重建本次会话；本局已知物品不清理
		ResetSearchSession_Internal();

		if (ContainerInventory == nullptr || !ContainerInventory.bEnableContainerSearch)
		{
			return;
		}

		bSearchSessionActive = true;
		SearchContainerInventory = ContainerInventory;
		BuildSearchQueue(ContainerInventory);
		SearchSessionRevision++;
		StartNextSearchItem();
	}

	UFUNCTION(Server)
	void ServerResetSearchSession()
	{
		ResetSearchSession_Internal();
	}

	UFUNCTION()
	void TickSearchSession()
	{
		if (!GetOwner().HasAuthority())
		{
			return;
		}

		if (!bSearchSessionActive || SearchContainerInventory == nullptr || CurrentSearchingItem == nullptr)
		{
			return;
		}
		if (!IsItemStillInContainer(CurrentSearchingItem, SearchContainerInventory))
		{
			// 多人并发下，当前搜索目标可能被其他玩家先拿走，立刻跳到下一项
			StartNextSearchItem();
			return;
		}

		auto IF_Grid = CurrentSearchingItem.FindItemFragment(FItemFragment_GridItem).Get(FItemFragment_GridItem);
		float BaseDuration = Math::Max(0.0f, IF_Grid.SearchDuration);
		if (BaseDuration <= 0.0f)
		{
			RevealItem(CurrentSearchingItem);
			StartNextSearchItem();
			return;
		}

		CurrentSearchProgress01 += (SearchTickInterval * GetSearchSpeedMultiplier()) / BaseDuration;
		CurrentSearchProgress01 = Math::Clamp(CurrentSearchProgress01, 0.0f, 1.0f);
		if (CurrentSearchProgress01 >= 1.0f)
		{
			// 当前物品检索完成后立即切换到下一项，维持连续检索体验。
			RevealItem(CurrentSearchingItem);
			StartNextSearchItem();
			return;
		}

		System::SetTimer(this, n"TickSearchSession", SearchTickInterval, false);
	}

	private void ResetSearchSession_Internal()
	{
		System::ClearTimer(this, "TickSearchSession");
		bSearchSessionActive = false;
		SearchContainerInventory = nullptr;
		RevealedSearchItems.Empty();
		PendingSearchQueue.Empty();
		CurrentSearchingItem = nullptr;
		CurrentSearchTargetDuration = 0.0f;
		CurrentSearchProgress01 = 0.0f;
		SearchTotalItemCount = 0;
		SearchRevealedItemCount = 0;
		SearchSessionRevision++;
	}

	private bool IsItemKnownInMatch(UYcInventoryItemInstance ItemInst) const
	{
		return ItemInst != nullptr && KnownSearchItems.Contains(ItemInst);
	}

	private void MarkItemKnownInMatch(UYcInventoryItemInstance ItemInst)
	{
		if (ItemInst == nullptr)
		{
			return;
		}
		if (!KnownSearchItems.Contains(ItemInst))
		{
			KnownSearchItems.Add(ItemInst);
		}
	}

	private bool IsItemStillInContainer(UYcInventoryItemInstance ItemInst, UGridInventoryManagerComponent ContainerInventory) const
	{
		if (ItemInst == nullptr || ContainerInventory == nullptr)
		{
			return false;
		}

		for (int32 i = 0; i < ContainerInventory.InventorySlots.Num(); i++)
		{
			auto Slot = ContainerInventory.InventorySlots[i];
			if (!Slot.bOccupied || Slot.ItemInstance == nullptr)
			{
				continue;
			}
			if (Slot.ItemInstance == ItemInst)
			{
				return true;
			}
		}

		return false;
	}

	private void RebuildSearchQueueFromContainerPreserveCurrent()
	{
		if (!bSearchSessionActive || SearchContainerInventory == nullptr)
		{
			return;
		}

		System::ClearTimer(this, "TickSearchSession");

		bool bCurrentStillValid = CurrentSearchingItem != nullptr &&
								  IsItemStillInContainer(CurrentSearchingItem, SearchContainerInventory) &&
								  !IsItemKnownInMatch(CurrentSearchingItem);

		// 一次性重建“待搜索”和“已揭示”两组数据，避免多人并发改动导致状态不一致。
		TArray<UYcInventoryItemInstance> NewPendingQueue;
		TArray<UYcInventoryItemInstance> NewRevealedItems;

		for (int32 i = 0; i < SearchContainerInventory.InventorySlots.Num(); i++)
		{
			auto Slot = SearchContainerInventory.InventorySlots[i];
			if (!Slot.bOccupied || Slot.ItemInstance == nullptr)
			{
				continue;
			}
			if (Slot.ItemRelativeX != 0 || Slot.ItemRelativeY != 0)
			{
				// 只看物品左上角主格，防止同一物品按占用格被重复统计。
				continue;
			}

			auto ItemInst = Slot.ItemInstance;
			if (IsItemKnownInMatch(ItemInst))
			{
				if (!NewRevealedItems.Contains(ItemInst))
				{
					NewRevealedItems.Add(ItemInst);
				}
				continue;
			}

			if (bCurrentStillValid && ItemInst == CurrentSearchingItem)
			{
				continue;
			}

			if (!NewPendingQueue.Contains(ItemInst))
			{
				NewPendingQueue.Add(ItemInst);
			}
		}

		RevealedSearchItems = NewRevealedItems;
		SearchRevealedItemCount = RevealedSearchItems.Num();
		PendingSearchQueue = NewPendingQueue;
		SearchTotalItemCount = PendingSearchQueue.Num() + (bCurrentStillValid ? 1 : 0);

		// 当前目标失效（被取走/已知），立刻切到下一项
		if (!bCurrentStillValid)
		{
			CurrentSearchingItem = nullptr;
			CurrentSearchTargetDuration = 0.0f;
			CurrentSearchProgress01 = 0.0f;
			SearchSessionRevision++;
			StartNextSearchItem();
			return;
		}

		SearchSessionRevision++;
		System::SetTimer(this, n"TickSearchSession", SearchTickInterval, false);
	}

	private void BuildSearchQueue(UGridInventoryManagerComponent ContainerInventory)
	{
		// 只把物品左上角主格加入队列，避免同一物品按占用格重复加入
		PendingSearchQueue.Empty();
		SearchTotalItemCount = 0;
		SearchRevealedItemCount = 0;
		RevealedSearchItems.Empty();

		for (int32 i = 0; i < ContainerInventory.InventorySlots.Num(); i++)
		{
			auto Slot = ContainerInventory.InventorySlots[i];
			if (!Slot.bOccupied || Slot.ItemInstance == nullptr)
			{
				continue;
			}
			if (Slot.ItemRelativeX != 0 || Slot.ItemRelativeY != 0)
			{
				continue;
			}
			if (IsItemKnownInMatch(Slot.ItemInstance))
			{
				RevealedSearchItems.Add(Slot.ItemInstance);
				SearchRevealedItemCount = RevealedSearchItems.Num();
				continue;
			}

			PendingSearchQueue.Add(Slot.ItemInstance);
		}

		SearchTotalItemCount = PendingSearchQueue.Num();
	}

	private void StartNextSearchItem()
	{
		System::ClearTimer(this, "TickSearchSession");
		CurrentSearchingItem = nullptr;
		CurrentSearchTargetDuration = 0.0f;
		CurrentSearchProgress01 = 0.0f;

		// 持续弹出直到找到一个可搜索目标，或队列耗尽
		while (PendingSearchQueue.Num() > 0)
		{
			// 队列可能含有“已被拿走/已知”的旧项，这里边弹边清洗。
			UYcInventoryItemInstance NextItem = PendingSearchQueue[0];
			PendingSearchQueue.RemoveAt(0);
			if (NextItem == nullptr)
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

			auto IF_Grid = NextItem.FindItemFragment(FItemFragment_GridItem).Get(FItemFragment_GridItem);
			float BaseDuration = Math::Max(0.0f, IF_Grid.SearchDuration);
			if (BaseDuration <= 0.0f)
			{
				RevealItem(NextItem);
				continue;
			}

			CurrentSearchingItem = NextItem;
			CurrentSearchTargetDuration = BaseDuration / GetSearchSpeedMultiplier();
			CurrentSearchProgress01 = 0.0f;
			SearchSessionRevision++;
			System::SetTimer(this, n"TickSearchSession", SearchTickInterval, false);
			return;
		}
		CurrentSearchProgress01 = SearchTotalItemCount > 0 ? 1.0f : 0.0f;
		SearchSessionRevision++;
	}

	private void RevealItem(UYcInventoryItemInstance ItemInst)
	{
		if (ItemInst == nullptr)
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
		SearchSessionRevision++;
	}

	private bool IsItemRevealedForContainerSession(UGridInventoryManagerComponent ItemOuterInventory, UYcInventoryItemInstance ItemInst) const
	{
		if (ItemOuterInventory == nullptr || !ItemOuterInventory.bEnableContainerSearch)
		{
			return true;
		}
		if (IsItemKnownInMatch(ItemInst))
		{
			return true;
		}
		if (!bSearchSessionActive || SearchContainerInventory == nullptr || SearchContainerInventory != ItemOuterInventory)
		{
			return false;
		}

		return RevealedSearchItems.Contains(ItemInst);
	}

	private bool EvaluateContextActionExecutability(UYcInventoryItemInstance ItemInst, FGridItemContextMenuAction ActionDef, FText&out OutDisabledReason)
	{
		if (ItemInst == nullptr)
		{
			OutDisabledReason = FText::FromString("Invalid item.");
			return false;
		}
		if (!ActionDef.ActionTag.IsValid())
		{
			OutDisabledReason = FText::FromString("Invalid action tag.");
			return false;
		}
		if (!IsValidContextActionExecutorTag(ActionDef.ExecutorTag))
		{
			OutDisabledReason = FText::FromString("Invalid executor tag namespace.");
			return false;
		}
		if (!IsItemOperableForCurrentSession(ItemInst))
		{
			OutDisabledReason = FText::FromString("Item is not revealed yet.");
			return false;
		}

		FYcContextActionEvalResult EvalResult;
		bool bPassed = YcGridInventoryContextAction::EvaluateContextActionPolicyForItem(
			GetOwner(),
			this,
			ItemInst,
			ActionDef,
			true,
			EvalResult);
		if (!bPassed)
		{
			if (!EvalResult.FailReason.IsEmpty())
			{
				OutDisabledReason = EvalResult.FailReason;
			}
			else
			{
				OutDisabledReason = FText::FromString("Context action policy blocked.");
			}
			return false;
		}
		return true;
	}

	private bool IsValidContextActionExecutorTag(FGameplayTag ExecutorTag) const
	{
		return YcGridInventoryContextAction::IsExecutorTagAllowed(ExecutorTag);
	}

	private bool TryFindContextMenuActionDef(UYcInventoryItemInstance ItemInst, FGameplayTag ActionTag, FGridItemContextMenuAction&out OutActionDef) const
	{
		OutActionDef = FGridItemContextMenuAction();
		if (ItemInst == nullptr || !ActionTag.IsValid())
		{
			return false;
		}

		FInstancedStruct Result = ItemInst.FindItemFragment(FItemFragment_ContextMenu);
		if (!Result.IsValid())
		{
			return false;
		}

		auto MenuFragment = Result.Get(FItemFragment_ContextMenu);
		TArray<FGameplayTag> SeenActionTags;
		for (int32 i = 0; i < MenuFragment.Actions.Num(); i++)
		{
			FGridItemContextMenuAction ActionDef = MenuFragment.Actions[i];
			if (!ActionDef.ActionTag.IsValid())
			{
				continue;
			}
			if (SeenActionTags.Contains(ActionDef.ActionTag))
			{
				Warning("ContextMenu action tag duplicated on same item, later one ignored.");
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

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool ValidateSwapLikeOperation(FYcInventoryOperation InOperation, FString&out OutReason)
	{
		OutReason = "";
		if (InOperation.SourceInventory == nullptr || InOperation.TargetInventory == nullptr || InOperation.ItemInstance == nullptr)
		{
			OutReason = "Swap-like op missing source/target/item.";
			return false;
		}
		return true;
	}

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool ExecuteSwapLikeOperation(FYcInventoryOperation InOperation, FString&out OutReason)
	{
		OutReason = "";
		FString OutSummary;
		// 将本次操作上下文暂存到组件字段，供 ExecuteSwapGridOperation 读取区域与口袋预期。
		CurrentHandleRegionId = InOperation.GridRegionId;
		CurrentExpectedSourceRegionId = InOperation.SourceGridRegionId;
		CurrentHandlePocketIndex = InOperation.GridPocketIndex;
		CurrentExpectedSourcePocketIndex = InOperation.SourceGridPocketIndex;
		bool bSuccess = ExecuteSwapGridOperation(
			InOperation.TargetInventory,
			InOperation.ItemInstance,
			InOperation.StackCount,
			InOperation.GridTile,
			InOperation.bRotated,
			InOperation.SourceInventory,
			OutReason,
			OutSummary);
		CurrentHandleRegionId = FGameplayTag();
		CurrentExpectedSourceRegionId = FGameplayTag();
		CurrentHandlePocketIndex = -1;
		CurrentExpectedSourcePocketIndex = -1;
		// 及时清空上下文，避免污染后续请求。
		if (bSuccess && !OutSummary.IsEmpty())
		{
			OutReason = OutSummary;
		}
		return bSuccess;
	}

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	void BuildSwapLikeOperationDelta(FYcInventoryOperation InOperation, bool bSuccess, FYcInventoryOperationDelta&out OutDelta)
	{
		if (!bSuccess)
		{
			OutDelta.Summary = "Swap-like operation failed.";
		}
		if (InOperation.ItemInstance != nullptr)
		{
			OutDelta.AffectedItemIds.Add(InOperation.ItemInstance.GetItemInstId());
		}
	}

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	bool ExecuteSwapGridOperation(UYcInventoryManagerComponent OnDropInventory, UYcInventoryItemInstance ItemInst, int32 StackCount, FIntPoint Tile, bool bRotated, UYcInventoryManagerComponent ExpectedSourceInventory, FString&out OutReason, FString&out OutSummary)
	{
		OutReason = "";
		OutSummary = "";

		if (ItemInst == nullptr || OnDropInventory == nullptr || ExpectedSourceInventory == nullptr)
		{
			OutReason = "SwapGrid invalid input.";
			return false;
		}

		auto TargetInventory = Cast<UGridInventoryManagerComponent>(OnDropInventory);
		auto ExpectedSourceGrid = Cast<UGridInventoryManagerComponent>(ExpectedSourceInventory);
		if (TargetInventory == nullptr || ExpectedSourceGrid == nullptr)
		{
			OutReason = "SwapGrid source/target inventory type invalid.";
			return false;
		}

		UGridInventoryManagerComponent SourceInventory = ItemInst.GetActorOuter().GetComponentByClass(UGridInventoryManagerComponent);
		if (SourceInventory == nullptr)
		{
			OutReason = "SwapGrid source inventory missing.";
			return false;
		}

		// CAS: 只有“期望源库存 == 当前真实源库存”才能继续，避免并发后请求覆盖先请求
		if (SourceInventory != ExpectedSourceGrid)
		{
			OutReason = "SwapGrid source changed.";
			return false;
		}

		// 仅允许操作自己的库存，或当前会话中的可搜索容器库存
		if (SourceInventory != this && !SourceInventory.bEnableContainerSearch && !SourceInventory.bAllowDirectContainerInteraction)
		{
			OutReason = "SwapGrid source not interactable.";
			return false;
		}
		if (TargetInventory != this && TargetInventory != SourceInventory && !TargetInventory.bEnableContainerSearch && !TargetInventory.bAllowDirectContainerInteraction)
		{
			OutReason = "SwapGrid target not interactable.";
			return false;
		}

		if (!IsItemRevealedForContainerSession(SourceInventory, ItemInst))
		{
			OutReason = "SwapGrid source item not revealed.";
			return false;
		}
		int32 RealStackCount = SourceInventory.GetStackCountByItemInstance(ItemInst);
		if (RealStackCount <= 0)
		{
			OutReason = "SwapGrid invalid stack.";
			return false;
		}

		// 先服务端校验目标格子，避免“先删后加失败”导致物品丢失
		FGameplayTag TargetRegionId = TargetInventory.ResolvePlacementRegionId(CurrentHandleRegionId);
		int32 TargetPocketIndex = TargetInventory.ResolvePlacementPocketIndex(TargetRegionId, CurrentHandlePocketIndex);
		if (!TargetInventory.CanPlaceGridItemInst(ItemInst, Tile, bRotated, TargetRegionId, TargetPocketIndex))
		{
			OutReason = "SwapGrid target blocked.";
			return false;
		}

		if (SourceInventory == TargetInventory)
		{
			SourceInventory.InnerSwapItemPosition(ItemInst, RealStackCount, Tile, bRotated, TargetRegionId, TargetPocketIndex);
			OutSummary = "SwapGrid same inventory.";
			return true;
		}

		FGameplayTag SourceRegionId;
		int32 SourcePocketIndex = -1;
		FIntPoint SourceTile = FIntPoint(0, 0);
		bool bSourceRotated = false;
		bool bHasSourcePlacement = SourceInventory.TryGetItemPlacementInfoWithRegion(ItemInst, SourceRegionId, SourcePocketIndex, SourceTile, bSourceRotated);
		if (CurrentExpectedSourceRegionId.IsValid() && SourceRegionId != CurrentExpectedSourceRegionId)
		{
			OutReason = "SwapGrid source region changed.";
			return false;
		}
		if (CurrentExpectedSourcePocketIndex >= 0 && SourcePocketIndex != CurrentExpectedSourcePocketIndex)
		{
			OutReason = "SwapGrid source pocket changed.";
			return false;
		}

		if (!SourceInventory.RemoveItemInstance(ItemInst))
		{
			OutReason = "SwapGrid remove source failed.";
			return false;
		}

		TargetInventory.CachedTile.Set(Tile);
		TargetInventory.CachedRegionId.Set(TargetRegionId);
		TargetInventory.CachedPocketIndex.Set(TargetPocketIndex);
		if (!TargetInventory.AddItemInstance(ItemInst, RealStackCount))
		{
			// 目标添加失败时进行补偿回滚：优先放回原位，其次尝试原朝向/自动寻位。
			if (bHasSourcePlacement && SourceInventory.CanPlaceGridItemInst(ItemInst, SourceTile, bSourceRotated, SourceRegionId, SourcePocketIndex))
			{
				SourceInventory.CachedTile.Set(SourceTile);
				SourceInventory.CachedRegionId.Set(SourceRegionId);
				SourceInventory.CachedPocketIndex.Set(SourcePocketIndex);
				SourceInventory.AddItemInstance(ItemInst, RealStackCount);
			}
			else if (bHasSourcePlacement && SourceInventory.CanPlaceGridItemInst(ItemInst, SourceTile, false, SourceRegionId, SourcePocketIndex))
			{
				SourceInventory.CachedTile.Set(SourceTile);
				SourceInventory.CachedRegionId.Set(SourceRegionId);
				SourceInventory.CachedPocketIndex.Set(SourcePocketIndex);
				SourceInventory.AddItemInstance(ItemInst, RealStackCount);
			}
			else
			{
				FGameplayTag FallbackRegionId;
				int32 FallbackPocketIndex = -1;
				bool bFitRotated = false;
				FIntPoint FallbackTile;
				if (SourceInventory.FindFirstFitPlacementForItemInst(ItemInst, FallbackRegionId, FallbackPocketIndex, FallbackTile, bFitRotated))
				{
					SourceInventory.CachedTile.Set(FallbackTile);
					SourceInventory.CachedRegionId.Set(FallbackRegionId);
					SourceInventory.CachedPocketIndex.Set(FallbackPocketIndex);
					SourceInventory.AddItemInstance(ItemInst, RealStackCount);
				}
				else
				{
					Error("交换回滚失败：无法将物品放回源库存，请检查库存容量与并发逻辑。");
				}
			}
			OutReason = "SwapGrid add target failed, rolled back.";
			return false;
		}

		OutSummary = "SwapGrid completed.";
		return true;
	}
	void InnerSwapItemPosition(UYcInventoryItemInstance ItemInst, int32 StackCount, FIntPoint Tile, bool bRotated = false, FGameplayTag RegionId = FGameplayTag(), int32 PocketIndex = -1)
	{
		OnGridItemInstanceAdded(ItemInst, StackCount, Tile, bRotated, RegionId, PocketIndex);
	}
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetItemLeftTopPosition(FYcItemInstanceId ItemID, FIntPoint&out OutPoint)
	{
		FYcInventoryItemEntry ItemEntry;
		if (!FindItemById(ItemID, ItemEntry))
			return false;

		if (ItemEntry.Instance == nullptr || !ItemInstanceToTileMap.Contains(ItemEntry.Instance))
		{
			return false;
		}
		OutPoint = ItemInstanceToTileMap[ItemEntry.Instance].TilePos;
		return true;
	}

	UFUNCTION()
	TMap<UYcInventoryItemInstance, FIntPoint> GetGridItemsTileMap()
	{
		TMap<UYcInventoryItemInstance, FIntPoint> Items;
		FGameplayTag PrimaryRegionId = GetPrimaryRegionId();
		int32 PrimaryPocketIndex = GetPrimaryPocketIndex(PrimaryRegionId);
		for (auto It : ItemInstanceToTileMap)
		{
			if (PrimaryRegionId.IsValid() && (It.Value.RegionId != PrimaryRegionId || It.Value.PocketIndex != PrimaryPocketIndex))
			{
				continue;
			}
			Items.Add(It.Key, It.Value.TilePos);
		}
		return Items;
	}

	UFUNCTION()
	TMap<UYcInventoryItemInstance, FIntPoint> GetGridItemsTileMapByRegion(FGameplayTag RegionId, int32 PocketIndex = -1)
	{
		TMap<UYcInventoryItemInstance, FIntPoint> Items;
		for (auto It : ItemInstanceToTileMap)
		{
			bool bRegionMatch = !RegionId.IsValid() || It.Value.RegionId == RegionId;
			bool bPocketMatch = PocketIndex < 0 || It.Value.PocketIndex == PocketIndex;
			if (bRegionMatch && bPocketMatch)
			{
				Items.Add(It.Key, It.Value.TilePos);
			}
		}
		return Items;
	}

	UFUNCTION()
	TMap<UYcInventoryItemInstance, bool> GetGridItemRotationMap()
	{
		TMap<UYcInventoryItemInstance, bool> Rotations;
		for (auto It : ItemInstanceToTileMap)
		{
			auto IF_Grid = GetItemFragmentGrid(It.Key.ItemRegistryId);
			int32 PlacedWidth = It.Value.ItemSize.X;
			int32 PlacedHeight = It.Value.ItemSize.Y;

			bool bRotated = false;
			if (PlacedWidth == IF_Grid.Dimensions.Y && PlacedHeight == IF_Grid.Dimensions.X &&
				(PlacedWidth != IF_Grid.Dimensions.X || PlacedHeight != IF_Grid.Dimensions.Y))
			{
				bRotated = true;
			}

			Rotations.Add(It.Key, bRotated);
		}

		return Rotations;
	}
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool FindFirstFitPosition(FDataRegistryId ItemDefId, FIntPoint&out Tile, bool&out OutRotated)
	{
		FGameplayTag RegionId;
		int32 PocketIndex = -1;
		return FindFirstFitPlacement(ItemDefId, RegionId, PocketIndex, Tile, OutRotated);
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	bool FindFirstFitPlacement(FDataRegistryId ItemDefId, FGameplayTag&out OutRegionId, int32&out OutPocketIndex, FIntPoint&out Tile, bool&out OutRotated)
	{
		for (int32 i = 0; i < RegionStates.Num(); i++)
		{
			auto Region = RegionStates[i];
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

	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	bool FindFirstFitPlacementForItemInst(UYcInventoryItemInstance ItemInst, FGameplayTag&out OutRegionId, int32&out OutPocketIndex, FIntPoint&out Tile, bool&out OutRotated)
	{
		if (ItemInst == nullptr)
		{
			OutRegionId = FGameplayTag();
			OutPocketIndex = -1;
			return false;
		}

		for (int32 i = 0; i < RegionStates.Num(); i++)
		{
			auto Region = RegionStates[i];
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

	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	bool FindFirstFitPlacementLegacy(FDataRegistryId ItemDefId, FGameplayTag&out OutRegionId, FIntPoint&out Tile, bool&out OutRotated)
	{
		int32 OutPocketIndex = -1;
		return FindFirstFitPlacement(ItemDefId, OutRegionId, OutPocketIndex, Tile, OutRotated);
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	bool FindFirstFitPositionInRegion(FDataRegistryId ItemDefId, FGameplayTag RegionId, int32 PocketIndex, FIntPoint&out Tile, bool&out OutRotated)
	{
		if (!PassesRegionTagConstraintForItemDef(ItemDefId, RegionId))
		{
			return false;
		}

		auto IF_Grid = GetItemFragmentGrid(ItemDefId);
		int32 Columns = GetRegionColumns(RegionId, PocketIndex);
		int32 Rows = GetRegionRows(RegionId, PocketIndex);
		for (int32 Y = 0; Y < Rows; Y++)
		{
			for (int32 X = 0; X < Columns; X++)
			{
				if (CanPlaceGridItem(ItemDefId, FIntPoint(X, Y), false, RegionId, PocketIndex))
				{
					Tile = FIntPoint(X, Y);
					OutRotated = false;
					return true;
				}
			}
		}
		if (!IF_Grid.bCanRotate)
		{
			return false;
		}
		for (int32 Y = 0; Y < Rows; Y++)
		{
			for (int32 X = 0; X < Columns; X++)
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

	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	bool FindFirstFitPositionInRegionLegacy(FDataRegistryId ItemDefId, FGameplayTag RegionId, FIntPoint&out Tile, bool&out OutRotated)
	{
		int32 PocketIndex = -1;
		return FindFirstFitPositionInRegion(ItemDefId, RegionId, PocketIndex, Tile, OutRotated);
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory|Rules")
	bool PassesRegionTagConstraintForItemDef(FDataRegistryId ItemDefId, FGameplayTag RegionId)
	{
		FGameplayTag FinalRegionId = ResolvePlacementRegionId(RegionId);
		FGridRegionTagConstraint Constraint = GetRegionTagConstraint(FinalRegionId);
		return YcGridInventoryPlacementRule::PassesTagConstraintForItemDef(ItemDefId, Constraint);
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory|Rules")
	bool PassesRegionTagConstraintForItemInst(UYcInventoryItemInstance ItemInst, FGameplayTag RegionId)
	{
		FGameplayTag FinalRegionId = ResolvePlacementRegionId(RegionId);
		FGridRegionTagConstraint Constraint = GetRegionTagConstraint(FinalRegionId);
		return YcGridInventoryPlacementRule::PassesTagConstraintForItemInstance(ItemInst, Constraint);
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool CanPlaceGridItem(FDataRegistryId ItemDefId, FIntPoint Tile, bool bRotated = false, FGameplayTag RegionId = FGameplayTag(), int32 PocketIndex = -1)
	{
		FGameplayTag FinalRegionId = ResolvePlacementRegionId(RegionId);
		int32 FinalPocketIndex = ResolvePlacementPocketIndex(FinalRegionId, PocketIndex);
		if (!PassesRegionTagConstraintForItemDef(ItemDefId, FinalRegionId))
		{
			return false;
		}

		auto IF_Grid = GetItemFragmentGrid(ItemDefId);
		int32 ItemWidth = bRotated ? IF_Grid.Dimensions.Y : IF_Grid.Dimensions.X;
		int32 ItemHeight = bRotated ? IF_Grid.Dimensions.X : IF_Grid.Dimensions.Y;
		int32 Columns = GetRegionColumns(FinalRegionId, FinalPocketIndex);
		int32 Rows = GetRegionRows(FinalRegionId, FinalPocketIndex);
		if (Tile.X < 0 || Tile.Y < 0 || Tile.X + ItemWidth > Columns || Tile.Y + ItemHeight > Rows)
		{
			return false;
		}
		auto RegionSlots = GetRegionSlots(FinalRegionId, FinalPocketIndex);
		for (int32 Y = Tile.Y; Y < Tile.Y + ItemHeight; Y++)
		{
			for (int32 X = Tile.X; X < Tile.X + ItemWidth; X++)
			{
				if (!IsRegionCellAvailable(FinalRegionId, FinalPocketIndex, FIntPoint(X, Y)))
				{
					return false;
				}
				int32 Index = TileToIndexInRegion(FIntPoint(X, Y), Columns);
				if (RegionSlots[Index].bOccupied)
					return false;
			}
		}
		return true;
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool CanPlaceGridItemInst(UYcInventoryItemInstance ItemInst, FIntPoint Tile, bool bRotated = false, FGameplayTag RegionId = FGameplayTag(), int32 PocketIndex = -1)
	{
		if (ItemInst == nullptr)
		{
			Warning("ItemInst is nullptr");
			return false;
		}
		FGameplayTag FinalRegionId = ResolvePlacementRegionId(RegionId);
		int32 FinalPocketIndex = ResolvePlacementPocketIndex(FinalRegionId, PocketIndex);
		if (!PassesRegionTagConstraintForItemInst(ItemInst, FinalRegionId))
		{
			return false;
		}

		auto IF_Grid = GetItemFragmentGrid(ItemInst.ItemRegistryId);
		int32 ItemWidth = bRotated ? IF_Grid.Dimensions.Y : IF_Grid.Dimensions.X;
		int32 ItemHeight = bRotated ? IF_Grid.Dimensions.X : IF_Grid.Dimensions.Y;
		int32 Columns = GetRegionColumns(FinalRegionId, FinalPocketIndex);
		int32 Rows = GetRegionRows(FinalRegionId, FinalPocketIndex);
		if (Tile.X < 0 || Tile.Y < 0 || Tile.X + ItemWidth > Columns || Tile.Y + ItemHeight > Rows)
		{
			return false;
		}
		auto RegionSlots = GetRegionSlots(FinalRegionId, FinalPocketIndex);
		for (int32 Y = Tile.Y; Y < Tile.Y + ItemHeight; Y++)
		{
			for (int32 X = Tile.X; X < Tile.X + ItemWidth; X++)
			{
				if (!IsRegionCellAvailable(FinalRegionId, FinalPocketIndex, FIntPoint(X, Y)))
				{
					return false;
				}
				int32 Index = TileToIndexInRegion(FIntPoint(X, Y), Columns);
				if (RegionSlots[Index].bOccupied && RegionSlots[Index].ItemInstance != ItemInst)
					return false;
			}
		}
		return true;
	}

	UFUNCTION(BlueprintPure)
	FItemFragment_GridItem GetItemFragmentGrid(FDataRegistryId ItemDefId)
	{
		FYcInventoryItemDefinition ItemDef;
		YcInventory::GetItemDefinition(ItemDefId, ItemDef);
		FInstancedStruct Result = YcInventory::FindItemFragment(ItemDef, FItemFragment_GridItem);
		if (!Result.IsValid())
		{
			return FItemFragment_GridItem();
		}
		FItemFragment_GridItem GridItemFragment = Result.Get(FItemFragment_GridItem);
		return GridItemFragment;
	}
	UFUNCTION()
	FIntPoint IndexToTile(int32 Index)
	{
		FIntPoint Tile;
		Tile.X = Index % InventoryColumns;
		Tile.Y = Math::IntegerDivisionTrunc(Index, InventoryColumns);
		return Tile;
	}
	UFUNCTION(BlueprintPure)
	int32 TileToIndex(FIntPoint Tile)
	{
		return TileToIndexInRegion(Tile, InventoryColumns);
	}

	private int32 TileToIndexInRegion(FIntPoint Tile, int32 RegionColumns)
	{
		return Tile.Y * RegionColumns + Tile.X;
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	FGameplayTag GetPrimaryRegionId() const
	{
		for (int32 i = 0; i < RegionStates.Num(); i++)
		{
			if (RegionStates[i].bEnabled)
			{
				return RegionStates[i].RegionId;
			}
		}
		return FGameplayTag();
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	int32 GetPrimaryPocketIndex(FGameplayTag RegionId) const
	{
		for (int32 i = 0; i < RegionStates.Num(); i++)
		{
			if (RegionStates[i].bEnabled && RegionStates[i].RegionId == RegionId)
			{
				return RegionStates[i].PocketIndex;
			}
		}
		return 0;
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	TArray<FGameplayTag> GetEnabledRegionIdsSorted() const
	{
		TArray<FGameplayTag> RegionIds;
		for (int32 i = 0; i < RegionStates.Num(); i++)
		{
			if (RegionStates[i].bEnabled)
			{
				if (!RegionIds.Contains(RegionStates[i].RegionId))
				{
					RegionIds.Add(RegionStates[i].RegionId);
				}
			}
		}
		return RegionIds;
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	TArray<FGridInventoryRegionRuntimeState> GetEnabledPocketStates() const
	{
		TArray<FGridInventoryRegionRuntimeState> States;
		for (int32 i = 0; i < RegionStates.Num(); i++)
		{
			if (RegionStates[i].bEnabled)
			{
				States.Add(RegionStates[i]);
			}
		}
		return States;
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	bool GetItemPlacementRegion(UYcInventoryItemInstance ItemInst, FGameplayTag&out OutRegionId, int32&out OutPocketIndex) const
	{
		OutRegionId = FGameplayTag();
		OutPocketIndex = 0;
		if (ItemInst == nullptr || !ItemInstanceToTileMap.Contains(ItemInst))
		{
			return false;
		}
		OutRegionId = ItemInstanceToTileMap[ItemInst].RegionId;
		OutPocketIndex = ItemInstanceToTileMap[ItemInst].PocketIndex;
		return true;
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	bool GetItemPlacementRegionLegacy(UYcInventoryItemInstance ItemInst, FGameplayTag&out OutRegionId) const
	{
		int32 OutPocketIndex = -1;
		return GetItemPlacementRegion(ItemInst, OutRegionId, OutPocketIndex);
	}

	private FGameplayTag ResolvePlacementRegionId(FGameplayTag RegionId) const
	{
		// 调用方未指定或指定区域不可用时，自动回退到主区域。
		if (RegionId.IsValid())
		{
			for (int32 i = 0; i < RegionStates.Num(); i++)
			{
				if (RegionStates[i].RegionId == RegionId && RegionStates[i].bEnabled)
				{
					return RegionId;
				}
			}
		}
		return GetPrimaryRegionId();
	}

	private int32 ResolvePlacementPocketIndex(FGameplayTag RegionId, int32 PocketIndex) const
	{
		// 调用方未指定或指定口袋不可用时，回退到该区域的主口袋。
		if (PocketIndex >= 0)
		{
			for (int32 i = 0; i < RegionStates.Num(); i++)
			{
				if (RegionStates[i].RegionId == RegionId && RegionStates[i].PocketIndex == PocketIndex && RegionStates[i].bEnabled)
				{
					return PocketIndex;
				}
			}
		}
		return GetPrimaryPocketIndex(RegionId);
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	int32 GetRegionColumns(FGameplayTag RegionId, int32 PocketIndex = -1) const
	{
		int32 FinalPocketIndex = ResolvePlacementPocketIndex(RegionId, PocketIndex);
		for (int32 i = 0; i < RegionStates.Num(); i++)
		{
			if (RegionStates[i].RegionId == RegionId && RegionStates[i].PocketIndex == FinalPocketIndex)
			{
				return RegionStates[i].Columns;
			}
		}
		return InventoryColumns;
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	int32 GetRegionRows(FGameplayTag RegionId, int32 PocketIndex = -1) const
	{
		int32 FinalPocketIndex = ResolvePlacementPocketIndex(RegionId, PocketIndex);
		for (int32 i = 0; i < RegionStates.Num(); i++)
		{
			if (RegionStates[i].RegionId == RegionId && RegionStates[i].PocketIndex == FinalPocketIndex)
			{
				return RegionStates[i].Rows;
			}
		}
		return InventoryRows;
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	int32 GetRegionPriority(FGameplayTag RegionId, int32 PocketIndex = -1) const
	{
		int32 FinalPocketIndex = ResolvePlacementPocketIndex(RegionId, PocketIndex);
		for (int32 i = 0; i < RegionStates.Num(); i++)
		{
			if (RegionStates[i].RegionId == RegionId && RegionStates[i].PocketIndex == FinalPocketIndex)
			{
				return RegionStates[i].Priority;
			}
		}
		return 9999;
	}

	private TArray<FGridInventorySlot> GetRegionSlots(FGameplayTag RegionId, int32 PocketIndex = -1)
	{
		// 惰性初始化：首次访问该区域口袋时按尺寸创建并清空槽位。
		int32 FinalPocketIndex = ResolvePlacementPocketIndex(RegionId, PocketIndex);
		int32 Index = FindRegionSlotsStorageIndex(RegionId, FinalPocketIndex);
		if (Index == -1)
		{
			FGridInventoryRegionSlotsStorage NewStorage;
			NewStorage.RegionId = RegionId;
			NewStorage.PocketIndex = FinalPocketIndex;
			NewStorage.Slots.SetNum(GetRegionColumns(RegionId, FinalPocketIndex) * GetRegionRows(RegionId, FinalPocketIndex));
			for (int32 i = 0; i < NewStorage.Slots.Num(); i++)
			{
				NewStorage.Slots[i].Reset();
			}
			RegionSlotsStorage.Add(NewStorage);
			Index = RegionSlotsStorage.Num() - 1;
		}
		return RegionSlotsStorage[Index].Slots;
	}

	private void SetRegionSlots(FGameplayTag RegionId, int32 PocketIndex, TArray<FGridInventorySlot> InSlots)
	{
		int32 FinalPocketIndex = ResolvePlacementPocketIndex(RegionId, PocketIndex);
		int32 Index = FindRegionSlotsStorageIndex(RegionId, FinalPocketIndex);
		if (Index == -1)
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

	private int32 FindRegionSlotsStorageIndex(FGameplayTag RegionId, int32 PocketIndex) const
	{
		for (int32 i = 0; i < RegionSlotsStorage.Num(); i++)
		{
			if (RegionSlotsStorage[i].RegionId == RegionId && RegionSlotsStorage[i].PocketIndex == PocketIndex)
			{
				return i;
			}
		}
		return -1;
	}

	private void SetRegionShapeCells(FGameplayTag RegionId, int32 PocketIndex, TArray<FIntPoint> InCells)
	{
		int32 FinalPocketIndex = ResolvePlacementPocketIndex(RegionId, PocketIndex);
		int32 Index = FindRegionShapeStorageIndex(RegionId, FinalPocketIndex);
		if (Index == -1)
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

	private int32 FindRegionShapeStorageIndex(FGameplayTag RegionId, int32 PocketIndex) const
	{
		for (int32 i = 0; i < RegionShapeStorage.Num(); i++)
		{
			if (RegionShapeStorage[i].RegionId == RegionId && RegionShapeStorage[i].PocketIndex == PocketIndex)
			{
				return i;
			}
		}
		return -1;
	}

	private void SetRegionTagConstraint(FGameplayTag RegionId, FGridRegionTagConstraint Constraint)
	{
		int32 Index = FindRegionTagConstraintStorageIndex(RegionId);
		if (Index == -1)
		{
			FGridInventoryRegionTagConstraintStorage NewStorage;
			NewStorage.RegionId = RegionId;
			NewStorage.Constraint = Constraint;
			RegionTagConstraintStorage.Add(NewStorage);
			return;
		}
		RegionTagConstraintStorage[Index].Constraint = Constraint;
	}

	private int32 FindRegionTagConstraintStorageIndex(FGameplayTag RegionId) const
	{
		for (int32 i = 0; i < RegionTagConstraintStorage.Num(); i++)
		{
			if (RegionTagConstraintStorage[i].RegionId == RegionId)
			{
				return i;
			}
		}
		return -1;
	}

	private FGridRegionTagConstraint GetRegionTagConstraint(FGameplayTag RegionId) const
	{
		int32 Index = FindRegionTagConstraintStorageIndex(RegionId);
		if (Index == -1)
		{
			return FGridRegionTagConstraint();
		}
		return RegionTagConstraintStorage[Index].Constraint;
	}

	private int32 FindEquipmentSlotBindingIndex(FGameplayTag SlotTag) const
	{
		for (int32 i = 0; i < EquipmentSlotRegionBindings.Num(); i++)
		{
			if (EquipmentSlotRegionBindings[i].SlotTag == SlotTag)
			{
				return i;
			}
		}
		return -1;
	}

	private void SetEquipmentSlotRegionBinding(FGameplayTag SlotTag, TArray<FGameplayTag> RegionIds)
	{
		int32 Index = FindEquipmentSlotBindingIndex(SlotTag);
		if (Index == -1)
		{
			FEquipmentSlotRegionBinding Binding;
			Binding.SlotTag = SlotTag;
			Binding.RegionIds = RegionIds;
			EquipmentSlotRegionBindings.Add(Binding);
			return;
		}
		EquipmentSlotRegionBindings[Index].RegionIds = RegionIds;
	}

	private bool IsRegionCellAvailable(FGameplayTag RegionId, int32 PocketIndex, FIntPoint Tile) const
	{
		// 未配置 ShapeCells 视为完整矩形可用；配置后仅白名单格子可放置。
		int32 FinalPocketIndex = ResolvePlacementPocketIndex(RegionId, PocketIndex);
		int32 Index = FindRegionShapeStorageIndex(RegionId, FinalPocketIndex);
		if (Index == -1)
		{
			return true;
		}
		auto Cells = RegionShapeStorage[Index].ShapeCells;
		for (int32 i = 0; i < Cells.Num(); i++)
		{
			if (Cells[i] == Tile)
			{
				return true;
			}
		}
		return false;
	}

	private void SyncPrimaryRegionToLegacySlots()
	{
		FGameplayTag PrimaryRegionId = GetPrimaryRegionId();
		int32 PrimaryPocketIndex = GetPrimaryPocketIndex(PrimaryRegionId);
		int32 PrimaryIndex = FindRegionSlotsStorageIndex(PrimaryRegionId, PrimaryPocketIndex);
		if (!PrimaryRegionId.IsValid() || PrimaryIndex == -1)
		{
			return;
		}
		auto PrimarySlots = RegionSlotsStorage[PrimaryIndex].Slots;
		InventoryColumns = GetRegionColumns(PrimaryRegionId, PrimaryPocketIndex);
		InventoryRows = GetRegionRows(PrimaryRegionId, PrimaryPocketIndex);
		InventorySlots = PrimarySlots;
	}

	private void BuildRegionsFromCurrentLoadout()
	{
		RegionStates.Empty();
		RegionShapeStorage.Empty();
		RegionTagConstraintStorage.Empty();
		EquipmentSlotRegionBindings.Empty();

		// 1. 创建默认的主区域，其实这个也是可选的
		FGridInventoryRegionRuntimeState MainRegion;
		MainRegion.RegionId = GameplayTags::Inventory_Region_Main;
		MainRegion.DisplayName = FText::FromString("Main");
		MainRegion.Priority = 0;
		MainRegion.PocketIndex = 0;
		MainRegion.Columns = InventoryColumns;
		MainRegion.Rows = InventoryRows;
		MainRegion.LayoutOffset = FIntPoint(0, 0);
		MainRegion.bEnabled = true;
		RegionStates.Add(MainRegion);
		SetRegionTagConstraint(MainRegion.RegionId, FGridRegionTagConstraint());

		// 2. 根据基础区域定义创建区域
		for (int32 i = 0; i < BaseRegionDefinitions.Num(); i++)
		{
			auto Def = BaseRegionDefinitions[i];
			if (!Def.RegionId.IsValid())
			{
				continue;
			}

			// 如果有口袋，就根据口袋配置创建多个区域，否则通过定义的Dimensions创建一个区域
			if (Def.Pockets.Num() > 0)
			{
				for (int32 PocketIndex = 0; PocketIndex < Def.Pockets.Num(); PocketIndex++)
				{
					auto PocketDef = Def.Pockets[PocketIndex];
					int32 ResolvedColumns = Math::Max(1, PocketDef.Dimensions.X);
					int32 ResolvedRows = Math::Max(1, PocketDef.Dimensions.Y);
					if (PocketDef.ShapeCells.Num() > 0)
					{
						for (int32 CellIndex = 0; CellIndex < PocketDef.ShapeCells.Num(); CellIndex++)
						{
							ResolvedColumns = Math::Max(ResolvedColumns, PocketDef.ShapeCells[CellIndex].X + 1);
							ResolvedRows = Math::Max(ResolvedRows, PocketDef.ShapeCells[CellIndex].Y + 1);
						}
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
						for (int32 CellIndex = 0; CellIndex < PocketDef.ShapeCells.Num(); CellIndex++)
						{
							CellSet.Add(PocketDef.ShapeCells[CellIndex].ToPoint());
						}
						SetRegionShapeCells(Def.RegionId, PocketIndex, CellSet);
					}
				}
			}
			else
			{
				int32 ResolvedColumns = Math::Max(1, Def.Dimensions.X);
				int32 ResolvedRows = Math::Max(1, Def.Dimensions.Y);
				if (Def.ShapeCells.Num() > 0)
				{
					for (int32 CellIndex = 0; CellIndex < Def.ShapeCells.Num(); CellIndex++)
					{
						ResolvedColumns = Math::Max(ResolvedColumns, Def.ShapeCells[CellIndex].X + 1);
						ResolvedRows = Math::Max(ResolvedRows, Def.ShapeCells[CellIndex].Y + 1);
					}
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
					for (int32 CellIndex = 0; CellIndex < Def.ShapeCells.Num(); CellIndex++)
					{
						CellSet.Add(Def.ShapeCells[CellIndex].ToPoint());
					}
					SetRegionShapeCells(Def.RegionId, 0, CellSet);
				}
			}
		}

		// 3. 根据当前装备槽位上装备的配置创建区域
		auto EquipmentSlotComp = UYcEquipmentSlotComponent::FindEquipmentSlotComponent(GetOwner());
		if (EquipmentSlotComp != nullptr)
		{
			auto EquippedSlots = EquipmentSlotComp.GetSlots();
			for (int32 SlotIndex = 0; SlotIndex < EquippedSlots.Num(); SlotIndex++)
			{
				auto EquippedSlot = EquippedSlots[SlotIndex];
				if (EquippedSlot.ItemInstance == nullptr)
				{
					continue;
				}

				// 获取装备的区域配置片段
				FInstancedStruct RegionFragmentResult = EquippedSlot.ItemInstance.FindItemFragment(FItemFragment_GridRegions);
				if (!RegionFragmentResult.IsValid())
				{
					continue;
				}

				auto RegionFragment = RegionFragmentResult.Get(FItemFragment_GridRegions);
				TArray<FGameplayTag> ProvidedIds;
				for (int32 RegionIndex = 0; RegionIndex < RegionFragment.Regions.Num(); RegionIndex++)
				{
					auto Def = RegionFragment.Regions[RegionIndex];
					if (!Def.RegionId.IsValid())
					{
						continue;
					}
					if (!ProvidedIds.Contains(Def.RegionId))
					{
						ProvidedIds.Add(Def.RegionId);
					}

					if (Def.Pockets.Num() > 0)
					{
						for (int32 PocketIndex = 0; PocketIndex < Def.Pockets.Num(); PocketIndex++)
						{
							auto PocketDef = Def.Pockets[PocketIndex];
							int32 ResolvedColumns = Math::Max(1, PocketDef.Dimensions.X);
							int32 ResolvedRows = Math::Max(1, PocketDef.Dimensions.Y);
							if (PocketDef.ShapeCells.Num() > 0)
							{
								for (int32 CellIndex = 0; CellIndex < PocketDef.ShapeCells.Num(); CellIndex++)
								{
									ResolvedColumns = Math::Max(ResolvedColumns, PocketDef.ShapeCells[CellIndex].X + 1);
									ResolvedRows = Math::Max(ResolvedRows, PocketDef.ShapeCells[CellIndex].Y + 1);
								}
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
								for (int32 CellIndex = 0; CellIndex < PocketDef.ShapeCells.Num(); CellIndex++)
								{
									CellSet.Add(PocketDef.ShapeCells[CellIndex].ToPoint());
								}
								SetRegionShapeCells(Def.RegionId, PocketIndex, CellSet);
							}
						}
					}
					else
					{
						int32 ResolvedColumns = Math::Max(1, Def.Dimensions.X);
						int32 ResolvedRows = Math::Max(1, Def.Dimensions.Y);
						if (Def.ShapeCells.Num() > 0)
						{
							for (int32 CellIndex = 0; CellIndex < Def.ShapeCells.Num(); CellIndex++)
							{
								ResolvedColumns = Math::Max(ResolvedColumns, Def.ShapeCells[CellIndex].X + 1);
								ResolvedRows = Math::Max(ResolvedRows, Def.ShapeCells[CellIndex].Y + 1);
							}
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
							for (int32 CellIndex = 0; CellIndex < Def.ShapeCells.Num(); CellIndex++)
							{
								CellSet.Add(Def.ShapeCells[CellIndex].ToPoint());
							}
							SetRegionShapeCells(Def.RegionId, 0, CellSet);
						}
					}
				}

				if (ProvidedIds.Num() > 0 && EquippedSlot.SlotTag.IsValid())
				{
					SetEquipmentSlotRegionBinding(EquippedSlot.SlotTag, ProvidedIds);
				}
			}
		}

		// 4. 按优先级排序区域，便于自动放置时根据优先级选择区域放置，目前用的冒泡排序算法实现，@TODO: 后续优化为更高效的排序算法
		for (int32 i = 0; i < RegionStates.Num(); i++)
		{
			for (int32 j = i + 1; j < RegionStates.Num(); j++)
			{
				if (RegionStates[j].Priority < RegionStates[i].Priority)
				{
					auto Tmp = RegionStates[i];
					RegionStates[i] = RegionStates[j];
					RegionStates[j] = Tmp;
				}
			}
		}

		RebuildRegionSlotsFromPlacementMap();
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	bool CanUnequipSlotWithInventoryRegionRelocate(FGameplayTag SlotTag, FString&out OutReason)
	{
		OutReason = "";
		TArray<FUnequipRelocateMove> RelocateMoves;
		FUnequipRelocateMove EquipMove;
		bool bCan = TryBuildUnequipRelocationPlan(SlotTag, RelocateMoves, EquipMove, OutReason);
		if (!bCan)
		{
			Warning(f"CanUnequipSlotWithInventoryRegionRelocate failed. Slot={SlotTag.ToString()} Reason={OutReason}");
		}
		return bCan;
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	bool ApplyPreUnequipRegionRelocation(FGameplayTag SlotTag, FString&out OutReason)
	{
		OutReason = "";
		TArray<FUnequipRelocateMove> RelocateMoves;
		FUnequipRelocateMove EquipMove;
		if (!TryBuildUnequipRelocationPlan(SlotTag, RelocateMoves, EquipMove, OutReason))
		{
			Warning(f"ApplyPreUnequipRegionRelocation validate failed. Slot={SlotTag.ToString()} Reason={OutReason}");
			return false;
		}

		TMap<UYcInventoryItemInstance, FItemGridInfo> ItemMapSnapshot = ItemInstanceToTileMap;
		TArray<FGridInventoryRegionSlotsStorage> RegionSlotsSnapshot = RegionSlotsStorage;
		int32 RevisionSnapshot = InventoryGridRevision;
		// 先做快照，任意一步失败都回滚，保证“卸装前重排”原子性。

		for (int32 i = 0; i < RelocateMoves.Num(); i++)
		{
			auto Move = RelocateMoves[i];
			if (Move.ItemInstance == nullptr)
			{
				continue;
			}
			int32 StackCount = GetStackCountByItemInstance(Move.ItemInstance);
			if (StackCount <= 0 || !OnGridItemInstanceAdded(Move.ItemInstance, StackCount, Move.Tile, Move.bRotated, Move.RegionId, Move.PocketIndex))
			{
				ItemInstanceToTileMap = ItemMapSnapshot;
				RegionSlotsStorage = RegionSlotsSnapshot;
				InventoryGridRevision = RevisionSnapshot;
				SyncPrimaryRegionToLegacySlots();
				OnInventoryGridChanged.Broadcast();
				OutReason = "Unequip relocate apply failed, rolled back.";
				return false;
			}
		}

		CachedTile.Set(EquipMove.Tile);
		CachedRegionId.Set(EquipMove.RegionId);
		CachedPocketIndex.Set(EquipMove.PocketIndex);
		return true;
	}

	private bool TryGetEquippedItemInSlot(FGameplayTag SlotTag, UYcInventoryItemInstance&out OutItem) const
	{
		OutItem = nullptr;
		auto EquipmentSlotComp = UYcEquipmentSlotComponent::FindEquipmentSlotComponent(GetOwner());
		if (EquipmentSlotComp == nullptr)
		{
			return false;
		}

		auto Slots = EquipmentSlotComp.GetSlots();
		for (int32 i = 0; i < Slots.Num(); i++)
		{
			if (Slots[i].SlotTag == SlotTag)
			{
				OutItem = Slots[i].ItemInstance;
				return OutItem != nullptr;
			}
		}
		return false;
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory|Regions")
	bool FindFirstFitPositionInRegionForItemInst(UYcInventoryItemInstance ItemInst, FGameplayTag RegionId, int32 PocketIndex, FIntPoint&out Tile, bool&out OutRotated)
	{
		if (ItemInst == nullptr)
		{
			return false;
		}

		if (!PassesRegionTagConstraintForItemInst(ItemInst, RegionId))
		{
			return false;
		}

		auto IF_Grid = GetItemFragmentGrid(ItemInst.ItemRegistryId);
		int32 Columns = GetRegionColumns(RegionId, PocketIndex);
		int32 Rows = GetRegionRows(RegionId, PocketIndex);
		for (int32 Y = 0; Y < Rows; Y++)
		{
			for (int32 X = 0; X < Columns; X++)
			{
				if (CanPlaceGridItemInst(ItemInst, FIntPoint(X, Y), false, RegionId, PocketIndex))
				{
					Tile = FIntPoint(X, Y);
					OutRotated = false;
					return true;
				}
			}
		}
		if (!IF_Grid.bCanRotate)
		{
			return false;
		}
		for (int32 Y = 0; Y < Rows; Y++)
		{
			for (int32 X = 0; X < Columns; X++)
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

	private int32 GetGridItemArea(UYcInventoryItemInstance ItemInst)
	{
		if (ItemInst == nullptr)
		{
			return 0;
		}
		auto IF_Grid = GetItemFragmentGrid(ItemInst.ItemRegistryId);
		return Math::Max(1, IF_Grid.Dimensions.X) * Math::Max(1, IF_Grid.Dimensions.Y);
	}

	private bool TryFindFitInSimRegion(
		UYcInventoryItemInstance ItemInst,
		FGameplayTag TargetRegionId,
		TArray<FUnequipRegionPocketSimState>&out SimPockets,
		int32&out OutPocketIndex,
		FIntPoint&out OutTile,
		bool&out OutRotated)
	{
		OutPocketIndex = -1;
		OutTile = FIntPoint(0, 0);
		OutRotated = false;
		if (ItemInst == nullptr)
		{
			return false;
		}

		auto IF_Grid = GetItemFragmentGrid(ItemInst.ItemRegistryId);
		for (int32 RotationPass = 0; RotationPass < 2; RotationPass++)
		{
			// 先不旋转后旋转，优先保持物品原朝向，减少布局突变。
			bool bTryRotated = (RotationPass == 1);
			if (bTryRotated && !IF_Grid.bCanRotate)
			{
				continue;
			}
			int32 ItemWidth = bTryRotated ? IF_Grid.Dimensions.Y : IF_Grid.Dimensions.X;
			int32 ItemHeight = bTryRotated ? IF_Grid.Dimensions.X : IF_Grid.Dimensions.Y;
			for (int32 PocketArrayIndex = 0; PocketArrayIndex < SimPockets.Num(); PocketArrayIndex++)
			{
				auto Pocket = SimPockets[PocketArrayIndex];
				if (!PassesRegionTagConstraintForItemInst(ItemInst, TargetRegionId))
				{
					continue;
				}

				for (int32 Y = 0; Y < Pocket.Rows; Y++)
				{
					for (int32 X = 0; X < Pocket.Columns; X++)
					{
						if (X + ItemWidth > Pocket.Columns || Y + ItemHeight > Pocket.Rows)
						{
							continue;
						}

						bool bCanPlace = true;
						for (int32 TestY = Y; TestY < Y + ItemHeight && bCanPlace; TestY++)
						{
							for (int32 TestX = X; TestX < X + ItemWidth; TestX++)
							{
								if (!IsRegionCellAvailable(TargetRegionId, Pocket.PocketIndex, FIntPoint(TestX, TestY)))
								{
									bCanPlace = false;
									break;
								}
								int32 SlotIndex = TileToIndexInRegion(FIntPoint(TestX, TestY), Pocket.Columns);
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

						for (int32 FillY = Y; FillY < Y + ItemHeight; FillY++)
						{
							for (int32 FillX = X; FillX < X + ItemWidth; FillX++)
							{
								int32 SlotIndex = TileToIndexInRegion(FIntPoint(FillX, FillY), Pocket.Columns);
								Pocket.Slots[SlotIndex].bOccupied = true;
							}
						}
						// 命中后立即把占用写回模拟口袋，后续物品基于新状态继续求解。
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

	private bool TryBuildUnequipRelocationPlan(FGameplayTag SlotTag, TArray<FUnequipRelocateMove>&out OutRelocateMoves, FUnequipRelocateMove&out OutEquipMove, FString&out OutReason)
	{
		OutRelocateMoves.Empty();
		OutEquipMove = FUnequipRelocateMove();
		OutReason = "";

		int32 BindingIndex = FindEquipmentSlotBindingIndex(SlotTag);
		if (BindingIndex == -1)
		{
			return true;
		}

		UYcInventoryItemInstance EquippedItem = nullptr;
		if (!TryGetEquippedItemInSlot(SlotTag, EquippedItem) || EquippedItem == nullptr)
		{
			OutReason = "Unequip failed: equipped item not found in slot.";
			return false;
		}

		auto RegionIdsToDisable = EquipmentSlotRegionBindings[BindingIndex].RegionIds;

		TArray<UYcInventoryItemInstance> ItemsToRelocate;
		for (auto Entry : ItemInstanceToTileMap)
		{
			if (Entry.Key != nullptr && RegionIdsToDisable.Contains(Entry.Value.RegionId))
			{
				ItemsToRelocate.Add(Entry.Key);
			}
		}

		for (int32 i = 0; i < ItemsToRelocate.Num(); i++)
		{
			for (int32 j = i + 1; j < ItemsToRelocate.Num(); j++)
			{
				// 大物品优先排布，能显著降低后续无解概率（贪心启发式）。
				if (GetGridItemArea(ItemsToRelocate[j]) > GetGridItemArea(ItemsToRelocate[i]))
				{
					auto Tmp = ItemsToRelocate[i];
					ItemsToRelocate[i] = ItemsToRelocate[j];
					ItemsToRelocate[j] = Tmp;
				}
			}
		}

		TArray<FGameplayTag> CandidateRegionIds;
		for (int32 i = 0; i < RegionStates.Num(); i++)
		{
			auto State = RegionStates[i];
			if (!State.bEnabled || RegionIdsToDisable.Contains(State.RegionId))
			{
				continue;
			}
			if (!CandidateRegionIds.Contains(State.RegionId))
			{
				CandidateRegionIds.Add(State.RegionId);
			}
		}

		for (int32 RegionIdx = 0; RegionIdx < CandidateRegionIds.Num(); RegionIdx++)
		{
			FGameplayTag CandidateRegionId = CandidateRegionIds[RegionIdx];
			TArray<FUnequipRegionPocketSimState> SimPockets;
			for (int32 i = 0; i < RegionStates.Num(); i++)
			{
				auto State = RegionStates[i];
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

			for (int32 i = 0; i < SimPockets.Num(); i++)
			{
				for (int32 j = i + 1; j < SimPockets.Num(); j++)
				{
					if (SimPockets[j].Priority < SimPockets[i].Priority)
					{
						auto Tmp = SimPockets[i];
						SimPockets[i] = SimPockets[j];
						SimPockets[j] = Tmp;
					}
				}
			}

			int32 EquipPocketIndex = -1;
			FIntPoint EquipTile;
			bool bEquipRotated = false;
			if (!TryFindFitInSimRegion(EquippedItem, CandidateRegionId, SimPockets, EquipPocketIndex, EquipTile, bEquipRotated))
			{
				// 装备本体都放不下时，当前候选区域直接判失败。
				continue;
			}

			TArray<FUnequipRelocateMove> CandidateMoves;
			bool bPlanValid = true;
			for (int32 i = 0; i < ItemsToRelocate.Num(); i++)
			{
				auto ItemToRelocate = ItemsToRelocate[i];
				int32 MovePocketIndex = -1;
				FIntPoint MoveTile;
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

		OutReason = "Unequip blocked: target region has no enough free space for equipment item and provided-region items.";
		return false;
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void DebugPrintSlots()
	{

		FString DebugStr = "\nGrid State:\n";
		Log(f"Grid inventory size: {InventorySlots.Num()}");
		for (int32 i = 0; i < InventorySlots.Num(); i++)
		{
			if (i % InventoryColumns == 0)
			{
				DebugStr.Append("\n");
			}
			if (InventorySlots[i].bOccupied)
			{
				DebugStr.Append("1 / ");
			}
			else
			{
				DebugStr.Append("0 / ");
			}
		}
		Log(DebugStr);
	}
}
