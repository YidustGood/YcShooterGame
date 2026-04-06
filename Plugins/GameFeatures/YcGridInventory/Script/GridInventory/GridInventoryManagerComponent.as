// 单个网格槽位数据：记录占用状态、所属物品与相对偏移
USTRUCT()
struct FGridInventorySlot
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool bOccupied = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName OccupyingItemID;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ItemRelativeX;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 ItemRelativeY;
	UPROPERTY()
	UYcInventoryItemInstance ItemInstance;

	void Reset()
	{
		bOccupied = false;
		OccupyingItemID = NAME_None;
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
	}
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint TilePos;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint ItemSize;
};
event void FInventoryGridChanged();

// @TODO 后续可以考虑迁移到C++层实现以获得更好的性能
// 网格库存核心组件：负责格子占用、物品放置、容器搜索会话与服务端交换校验
class UGridInventoryManagerComponent : UYcInventoryManagerComponent
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 InventoryColumns = 10;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 InventoryRows = 10;
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Search")
	bool bEnableContainerSearch = false;
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FGridInventorySlot> InventorySlots;
	UPROPERTY(Replicated)
	int32 InventoryGridRevision = 0;
	TMap<UYcInventoryItemInstance, FItemGridInfo> ItemInstanceToTileMap;

	UPROPERTY()
	FInventoryGridChanged OnInventoryGridChanged;

	FGameplayMessageListenerHandle InventoryChangedHandle;

	private FIntPoint CurrentHandleTile;

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
	}

	UFUNCTION(BlueprintOverride)
	void EndPlay(EEndPlayReason EndPlayReason)
	{
		InventoryChangedHandle.Unregister();
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
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	void InitializeInventory()
	{
		InventorySlots.SetNum(InventoryColumns * InventoryRows);
		for (int32 i = 0; i < InventorySlots.Num(); i++)
		{
			InventorySlots[i].Reset();
		}
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
					OnGridItemInstanceAdded(Data.ItemInstance, Data.NewCount, CachedTile.GetValue(), false);
					CachedTile.Reset();
					return;
				}
				bool bRotated;
				FIntPoint Tile;
				if (FindFirstFitPosition(Data.ItemInstance.ItemRegistryId, Tile, bRotated))
				{
					OnGridItemInstanceAdded(Data.ItemInstance, Data.NewCount, Tile, bRotated);
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
	bool TryAddGridItemByDefinition(FDataRegistryId ItemDefId, int32 StackCount, FIntPoint Tile, bool bRotated = false)
	{
		if (!CanPlaceGridItem(ItemDefId, Tile, bRotated))
		{
			return false;
		}

		auto ItemInstance = AddItem(ItemDefId, StackCount);
		if (ItemInstance == nullptr)
		{
			Error("ItemInstance is invalid!");
			return false;
		}
		return true;
	}
	UFUNCTION(BlueprintCallable, Category = "Inventory", BlueprintAuthorityOnly)
	bool TryAddGridItemInstance(UYcInventoryItemInstance ItemInst, int32 StackCount, FIntPoint Tile, bool bRotated = false)
	{
		check(ItemInst != nullptr, "ItemInst is nullptr");

		if (!CanPlaceGridItem(ItemInst.ItemRegistryId, Tile, bRotated))
		{
			return false;
		}

		AddItemInstance(ItemInst, StackCount);
		return true;
	}
	UFUNCTION(BlueprintCallable, Category = "Inventory", BlueprintAuthorityOnly)
	bool RemoveGridItem(UYcInventoryItemInstance ItemInst)
	{
		bool bValidRemove = false;
		for (int32 i = 0; i < InventorySlots.Num(); i++)
		{
			if (InventorySlots[i].ItemInstance == ItemInst)
			{
				InventorySlots[i].Reset();
				bValidRemove = true;
			}
		}
		RemoveItemInstance(ItemInst);
		return bValidRemove;
	}
	UFUNCTION(BlueprintCallable, Category = "Inventory", BlueprintAuthorityOnly)
	bool OnGridItemInstanceAdded(UYcInventoryItemInstance ItemInst, int32 StackCount, FIntPoint Tile, bool bRotated = false)
	{
		check(ItemInst != nullptr, "ItemInst is nullptr");

		if (!CanPlaceGridItemInst(ItemInst, Tile, bRotated))
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
		ItemInstanceToTileMap.Add(ItemInst, FItemGridInfo(Tile, FIntPoint(ItemWidth, ItemHeight)));
		for (int32 Y = Tile.Y; Y < Tile.Y + ItemHeight; Y++)
		{
			for (int32 X = Tile.X; X < Tile.X + ItemWidth; X++)
			{
				int32 Index = TileToIndex(FIntPoint(X, Y));
				FYcInventoryItemDefinition ItemDef;
				ItemInst.GetItemDef(ItemDef);
				InventorySlots[Index].bOccupied = true;
				InventorySlots[Index].OccupyingItemID = ItemDef.ItemId;
				InventorySlots[Index].ItemInstance = ItemInst;
				InventorySlots[Index].ItemRelativeX = X - Tile.X;
				InventorySlots[Index].ItemRelativeY = Y - Tile.Y;
			}
		}
		InventoryGridRevision++;
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
		for (int X = ItemGridInfo.TilePos.X; X < ItemGridInfo.TilePos.X + ItemGridInfo.ItemSize.X; X++)
		{
			for (int Y = ItemGridInfo.TilePos.Y; Y < ItemGridInfo.TilePos.Y + ItemGridInfo.ItemSize.Y; Y++)
			{
				int32 Index = TileToIndex(FIntPoint(X, Y));
				InventorySlots[Index].Reset();
			}
		}
		ItemInstanceToTileMap.Remove(ItemInst);
		InventoryGridRevision++;
		OnInventoryGridChanged.Broadcast();
	}

	private bool TryGetItemPlacementInfo(UYcInventoryItemInstance ItemInst, FIntPoint&out OutTile, bool&out bOutRotated)
	{
		if (ItemInst == nullptr || !ItemInstanceToTileMap.Contains(ItemInst))
		{
			return false;
		}

		auto ItemGridInfo = ItemInstanceToTileMap[ItemInst];
		OutTile = ItemGridInfo.TilePos;
		auto IF_Grid = GetItemFragmentGrid(ItemInst.ItemRegistryId);
		bOutRotated = (ItemGridInfo.ItemSize.X == IF_Grid.Dimensions.Y && ItemGridInfo.ItemSize.Y == IF_Grid.Dimensions.X);
		return true;
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

		auto ItemOuterInventory = Cast<UGridInventoryManagerComponent>(ItemInst.GetActorOuter().GetComponentByClass(UGridInventoryManagerComponent));
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

		auto ItemOuterInventory = Cast<UGridInventoryManagerComponent>(ItemInst.GetActorOuter().GetComponentByClass(UGridInventoryManagerComponent));
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
		bool bSuccess = ExecuteSwapGridOperation(
			InOperation.TargetInventory,
			InOperation.ItemInstance,
			InOperation.StackCount,
			InOperation.GridTile,
			InOperation.bRotated,
			InOperation.SourceInventory,
			OutReason,
			OutSummary);
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

		auto SourceInventory = Cast<UGridInventoryManagerComponent>(ItemInst.GetActorOuter().GetComponentByClass(UGridInventoryManagerComponent));
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
		if (SourceInventory != this && !SourceInventory.bEnableContainerSearch)
		{
			OutReason = "SwapGrid source not interactable.";
			return false;
		}
		if (TargetInventory != this && TargetInventory != SourceInventory && !TargetInventory.bEnableContainerSearch)
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
		if (!TargetInventory.CanPlaceGridItemInst(ItemInst, Tile, bRotated))
		{
			OutReason = "SwapGrid target blocked.";
			return false;
		}

		if (SourceInventory == TargetInventory)
		{
			SourceInventory.InnerSwapItemPosition(ItemInst, RealStackCount, Tile, bRotated);
			OutSummary = "SwapGrid same inventory.";
			return true;
		}

		FIntPoint SourceTile = FIntPoint(0, 0);
		bool bSourceRotated = false;
		bool bHasSourcePlacement = SourceInventory.TryGetItemPlacementInfo(ItemInst, SourceTile, bSourceRotated);

		if (!SourceInventory.RemoveItemInstance(ItemInst))
		{
			OutReason = "SwapGrid remove source failed.";
			return false;
		}

		TargetInventory.CachedTile.Set(Tile);
		if (!TargetInventory.AddItemInstance(ItemInst, RealStackCount))
		{
			if (bHasSourcePlacement && SourceInventory.CanPlaceGridItemInst(ItemInst, SourceTile, bSourceRotated))
			{
				SourceInventory.CachedTile.Set(SourceTile);
				SourceInventory.AddItemInstance(ItemInst, RealStackCount);
			}
			else if (bHasSourcePlacement && SourceInventory.CanPlaceGridItemInst(ItemInst, SourceTile, false))
			{
				SourceInventory.CachedTile.Set(SourceTile);
				SourceInventory.AddItemInstance(ItemInst, RealStackCount);
			}
			else
			{
				bool bFitRotated = false;
				FIntPoint FallbackTile;
				if (SourceInventory.FindFirstFitPosition(ItemInst.ItemRegistryId, FallbackTile, bFitRotated))
				{
					SourceInventory.CachedTile.Set(FallbackTile);
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
	void InnerSwapItemPosition(UYcInventoryItemInstance ItemInst, int32 StackCount, FIntPoint Tile, bool bRotated = false)
	{
		OnGridItemInstanceAdded(ItemInst, StackCount, Tile, bRotated);
	}
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool GetItemLeftTopPosition(FName ItemID, FIntPoint&out OutPoint)
	{
		FYcInventoryItemEntry ItemEntry;
		if (!FindItemById(ItemID, ItemEntry))
			return false;

		for (int32 i = 0; i < InventorySlots.Num(); i++)
		{
			if (InventorySlots[i].OccupyingItemID == ItemID &&
				InventorySlots[i].ItemRelativeX == 0 &&
				InventorySlots[i].ItemRelativeY == 0)
			{
				OutPoint = IndexToTile(i);
				return true;
			}
		}
		return false;
	}

	UFUNCTION()
	TMap<UYcInventoryItemInstance, FIntPoint> GetGridItemsTileMap()
	{
		TMap<UYcInventoryItemInstance, FIntPoint> Items;
		for (int32 i = 0; i < InventorySlots.Num(); i++)
		{
			if (Items.Contains(InventorySlots[i].ItemInstance))
				continue;
			if (!InventorySlots[i].bOccupied)
				continue;
			FIntPoint Tile = IndexToTile(i);
			Items.Add(InventorySlots[i].ItemInstance, Tile);
		}
		return Items;
	}
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool FindFirstFitPosition(FDataRegistryId ItemDefId, FIntPoint&out Tile, bool&out OutRotated)
	{
		auto IF_Grid = GetItemFragmentGrid(ItemDefId);
		for (int32 Y = 0; Y < InventoryRows; Y++)
		{
			for (int32 X = 0; X < InventoryColumns; X++)
			{
				if (CanPlaceGridItem(ItemDefId, FIntPoint(X, Y), false))
				{
					Tile = FIntPoint(X, Y);
					OutRotated = false;
					return true;
				}
			}
		}

		if (!IF_Grid.bCanRotate)
			return false;
		for (int32 Y = 0; Y < InventoryRows; Y++)
		{
			for (int32 X = 0; X < InventoryColumns; X++)
			{
				if (CanPlaceGridItem(ItemDefId, FIntPoint(X, Y), true))
				{
					Tile = FIntPoint(X, Y);
					OutRotated = true;
					return true;
				}
			}
		}
		return false;
	};
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool CanPlaceGridItem(FDataRegistryId ItemDefId, FIntPoint Tile, bool bRotated = false)
	{
		auto IF_Grid = GetItemFragmentGrid(ItemDefId);
		int32 ItemWidth = bRotated ? IF_Grid.Dimensions.Y : IF_Grid.Dimensions.X;
		int32 ItemHeight = bRotated ? IF_Grid.Dimensions.X : IF_Grid.Dimensions.Y;
		if (Tile.X < 0 || Tile.Y < 0 || Tile.X + ItemWidth > InventoryColumns || Tile.Y + ItemHeight > InventoryRows)
		{
			return false;
		}
		for (int32 Y = Tile.Y; Y < Tile.Y + ItemHeight; Y++)
		{
			for (int32 X = Tile.X; X < Tile.X + ItemWidth; X++)
			{
				int32 Index = TileToIndex(FIntPoint(X, Y));
				if (InventorySlots[Index].bOccupied)
					return false;
			}
		}
		return true;
	}

	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool CanPlaceGridItemInst(UYcInventoryItemInstance ItemInst, FIntPoint Tile, bool bRotated = false)
	{
		if (ItemInst == nullptr)
		{
			Warning("ItemInst is nullptr");
			return false;
		}
		auto IF_Grid = GetItemFragmentGrid(ItemInst.ItemRegistryId);
		int32 ItemWidth = bRotated ? IF_Grid.Dimensions.Y : IF_Grid.Dimensions.X;
		int32 ItemHeight = bRotated ? IF_Grid.Dimensions.X : IF_Grid.Dimensions.Y;
		if (Tile.X < 0 || Tile.Y < 0 || Tile.X + ItemWidth > InventoryColumns || Tile.Y + ItemHeight > InventoryRows)
		{
			return false;
		}
		for (int32 Y = Tile.Y; Y < Tile.Y + ItemHeight; Y++)
		{
			for (int32 X = Tile.X; X < Tile.X + ItemWidth; X++)
			{
				int32 Index = TileToIndex(FIntPoint(X, Y));
				if (InventorySlots[Index].bOccupied && InventorySlots[Index].ItemInstance != ItemInst)
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
		int32 Index;
		Index = Tile.Y * InventoryColumns + Tile.X;
		return Index;
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
