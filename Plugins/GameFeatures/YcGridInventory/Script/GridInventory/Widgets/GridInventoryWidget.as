// 单个网格库存视图：负责网格绘制、物品Widget增量刷新与拖拽落点处理
class UGridInventoryWidget : UUserWidget
{
	UPROPERTY(BindWidget)
	UBorder GridBorder;

	UPROPERTY(BindWidget)
	UCanvasPanel GridCanvasPanel;

	UPROPERTY()
	TSubclassOf<UGridItemWidget> ItemWidgetClass;

	UPROPERTY()
	UGridInventoryManagerComponent GridInventoryManager;

	// 线条位置数据
	private TArray<FGridInventoryLine> Lines;

	// 库存格子大小 显示长宽
	UPROPERTY()
	float TileSize;

	private int32 Columns;
	private int32 Rows;

	UPROPERTY()
	FIntPoint DraggedItemTopLeftTile;

	UPROPERTY()
	bool bDrawDropLocation;

	private int32 CachedSearchRevision = -1;
	private int32 CachedGridRevision = -1;
	private bool bForceItemVisualRefresh = false;
	private TMap<UYcInventoryItemInstance, UGridItemWidget> ItemWidgetMap;
	private TArray<FYcInventoryOperation> PendingPredictedOps;

	FGameplayMessageListenerHandle InventoryChangedHandle;
	FGameplayMessageListenerHandle OperationStateHandle;

	UFUNCTION()
	void Initialize(UGridInventoryManagerComponent InGridInventoryManager, float InTileSize)
	{
		if (InGridInventoryManager == nullptr)
		{
			Error("UGridInventoryWidget::Initialize - InGridInventoryManager is nullptr");
			return;
		}

		GridInventoryManager = InGridInventoryManager;
		TileSize = InTileSize;
		Columns = GridInventoryManager.InventoryColumns;
		Rows = GridInventoryManager.InventoryRows;
		CachedGridRevision = -1;
		CachedSearchRevision = -1;
		bForceItemVisualRefresh = true;
		ItemWidgetMap.Empty();
        PendingPredictedOps.Empty();
		GridCanvasPanel.ClearChildren();

		auto GridBorderSlot = WidgetLayout::SlotAsCanvasSlot(GridBorder);
		GridBorderSlot.SetSize(FVector2D(TileSize * GridInventoryManager.InventoryColumns, TileSize * GridInventoryManager.InventoryRows));

		CreateLineSegments();
		GridInventoryManager.OnInventoryGridChanged.AddUFunction(this, n"OnInventoryGridChanged");
	}

	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		InventoryChangedHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			GameplayTags::Yc_Inventory_Message_StackChanged,
			this,
			n"OnInventoryChanged",
			FYcInventoryItemChangeMessage(),
			EGameplayMessageMatch::ExactMatch);

		OperationStateHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			FGameplayTag::RequestGameplayTag(n"Yc.Inventory.Message.ProjectedStateChanged"),
			this,
			n"OnOperationStateChanged",
			FYcInventoryProjectedStateChangedMessage(),
			EGameplayMessageMatch::ExactMatch);
	}

	UFUNCTION(BlueprintOverride)
	void Destruct()
	{
		InventoryChangedHandle.Unregister();
		OperationStateHandle.Unregister();
		ItemWidgetMap.Empty();
        PendingPredictedOps.Empty();
	}

	UFUNCTION(BlueprintOverride)
	void Tick(FGeometry MyGeometry, float InDeltaTime)
	{
		UpdateInventoryPresentation();
		UpdateSearchPresentation();
	}

	UFUNCTION(BlueprintOverride)
	void OnPaint(FPaintContext& Context) const
	{
		auto LocalTopLeft = Slate::GetLocalTopLeft(GridBorder.GetCachedGeometry());
		for (int32 i = 0; i < Lines.Num(); i++)
		{
			FGridInventoryLine Line = Lines[i];
			FVector2D PosA = LocalTopLeft + Line.Start;
			FVector2D PosB = LocalTopLeft + Line.End;
			Context.DrawLine(PosA, PosB, FLinearColor(0.5, 0.5, 0.5, 0.5));
		}

		if (Widget::IsDragDropping() && bDrawDropLocation)
		{
			auto DraggedItem = Cast<UYcInventoryItemInstance>(Widget::GetDragDroppingContent().Payload);
			if (DraggedItem != nullptr)
			{
				auto IF_Drag = DraggedItem.FindItemFragment(FItemFragment_GridItem).Get(FItemFragment_GridItem);
				bool bIsRoomAvailable = IsRoomAvailable(DraggedItem);
				FLinearColor TintColor = FLinearColor(1, 0, 0, 0.25);
				if (bIsRoomAvailable)
				{
					TintColor = FLinearColor(0, 1, 0, 0.25);
				}
				FVector2D Position = FVector2D(DraggedItemTopLeftTile.X * TileSize, DraggedItemTopLeftTile.Y * TileSize);
				FVector2D Size = FVector2D(IF_Drag.Dimensions.X * TileSize, IF_Drag.Dimensions.Y * TileSize);
				Context.DrawBox(Position, Size, TintColor);
			}
		}

		// 容器搜索进度反馈（顶部细进度条，仅当前会话容器显示）
		auto PlayerInventory = GetOwningPlayerGridInventory();
		if (PlayerInventory != nullptr)
		{
			int32 Revision;
			if (!PlayerInventory.GetSearchSessionRevisionForContainer(GridInventoryManager, Revision))
			{
				return;
			}

			float Progress01;
			float RemainingSeconds;
			int32 RevealedCount;
			int32 TotalCount;
			if (PlayerInventory.GetCurrentSearchProgress(Progress01, RemainingSeconds, RevealedCount, TotalCount))
			{
				float FullWidth = Columns * TileSize;
				float BarHeight = 4.0f;
				Context.DrawBox(LocalTopLeft + FVector2D(0, 0), FVector2D(FullWidth, BarHeight), FLinearColor(0.08f, 0.08f, 0.08f, 0.85f));
				Context.DrawBox(LocalTopLeft + FVector2D(0, 0), FVector2D(FullWidth * Math::Clamp(Progress01, 0.0f, 1.0f), BarHeight), FLinearColor(0.95f, 0.75f, 0.2f, 0.95f));
			}
		}
	}

	UFUNCTION()
	void OnInventoryGridChanged()
	{
		Refresh();
	}

	UFUNCTION()
	bool RemoveGridItem(UYcInventoryItemInstance ItemInst)
	{
		return GridInventoryManager.RemoveGridItem(ItemInst);
	}

	UFUNCTION()
	private void CreateLineSegments()
	{
		Lines.Empty();

		for (int32 i = 0; i < Columns; i++)
		{
			float LocalX = TileSize * i;
			FGridInventoryLine Line;
			Line.Start = FVector2D(LocalX, 0);
			Line.End = FVector2D(LocalX, Rows * TileSize);
			Lines.Add(Line);
		}

		for (int32 i = 0; i < Rows; i++)
		{
			float LocalY = TileSize * i;
			FGridInventoryLine Line;
			Line.Start = FVector2D(0, LocalY);
			Line.End = FVector2D(Columns * TileSize, LocalY);
			Lines.Add(Line);
		}
	}

	UFUNCTION()
	void OnInventoryChanged(FGameplayTag ActualTag, FYcInventoryItemChangeMessage Data)
	{
		DelayUntilNextTickForAs(n"Refresh");
	}

	UFUNCTION()
	void OnOperationStateChanged(FGameplayTag ActualTag, FYcInventoryProjectedStateChangedMessage Data)
	{
		if (GridInventoryManager == nullptr)
		{
			return;
		}
		if (Data.Operation.SourceInventory != GridInventoryManager && Data.Operation.TargetInventory != GridInventoryManager)
		{
			return;
		}

		if (Data.Event == EYcInventoryOperationEvent::Submitted)
		{
			UpsertPendingPredictedOp(Data.Operation);
			if (Data.Operation.OpType == n"Inventory.SwapGrid" && IsSwapPredictionLocallyValid(Data.Operation))
			{
				ApplyPredictedSwap(Data.Operation);
			}
			else if ((Data.Operation.OpType == n"Equipment.Unequip" || Data.Operation.OpType == n"QuickBar.Remove") && Data.Operation.ItemInstance != nullptr)
			{
				FIntPoint PredictedTile;
				bool bPredictedRotated;
				if (TryFindPredictedReturnTile(Data.Operation.ItemInstance, PredictedTile, bPredictedRotated))
				{
					MoveOrAddPredictedItemWidget(Data.Operation.ItemInstance, PredictedTile);
				}
			}
			else if ((Data.Operation.OpType == n"Equipment.Equip" || Data.Operation.OpType == n"QuickBar.Add") && Data.Operation.ItemInstance != nullptr)
			{
				ApplyPredictedEquipOrQuickBarAdd(Data.Operation);
			}
			return;
		}

		if (Data.Event == EYcInventoryOperationEvent::Acked || Data.Event == EYcInventoryOperationEvent::Nacked)
		{
			RemovePendingPredictedOp(Data.Operation.OpId);
			DelayUntilNextTickForAs(n"Refresh");
		}
	}

	UFUNCTION()
	bool IsSwapPredictionLocallyValid(FYcInventoryOperation Op) const
	{
		if (Op.ItemInstance == nullptr)
		{
			return false;
		}

		if (Op.TargetInventory == GridInventoryManager)
		{
			return GridInventoryManager.CanPlaceGridItemInst(Op.ItemInstance, Op.GridTile, Op.bRotated);
		}

		return true;
	}
	UFUNCTION()
	bool TryFindPredictedReturnTile(UYcInventoryItemInstance ItemInstance, FIntPoint&out OutTile, bool&out OutRotated) const
	{
		OutTile = FIntPoint(0, 0);
		OutRotated = false;
		if (GridInventoryManager == nullptr || ItemInstance == nullptr)
		{
			return false;
		}

		TMap<UYcInventoryItemInstance, FIntPoint> Items = GridInventoryManager.GetGridItemsTileMap();
		if (Items.Find(ItemInstance, OutTile))
		{
			return true;
		}

		return GridInventoryManager.FindFirstFitPosition(ItemInstance.ItemRegistryId, OutTile, OutRotated);
	}

	UFUNCTION()
	void UpsertPendingPredictedOp(FYcInventoryOperation Op)
	{
		for (int32 i = 0; i < PendingPredictedOps.Num(); i++)
		{
			if (PendingPredictedOps[i].OpId == Op.OpId)
			{
				PendingPredictedOps[i] = Op;
				return;
			}
		}
		PendingPredictedOps.Add(Op);
	}

	UFUNCTION()
	void RemovePendingPredictedOp(int64 OpId)
	{
		for (int32 i = PendingPredictedOps.Num() - 1; i >= 0; i--)
		{
			if (PendingPredictedOps[i].OpId == OpId)
			{
				PendingPredictedOps.RemoveAt(i);
			}
		}
	}

	UFUNCTION()
	void ReapplyPendingPredictions()
	{
		for (int32 i = 0; i < PendingPredictedOps.Num(); i++)
		{
			auto PendingOp = PendingPredictedOps[i];
			if (PendingOp.OpType == n"Inventory.SwapGrid")
			{
				if (!IsSwapPredictionLocallyValid(PendingOp))
				{
					continue;
				}
				ApplyPredictedSwap(PendingOp);
				continue;
			}

			if ((PendingOp.OpType == n"Equipment.Unequip" || PendingOp.OpType == n"QuickBar.Remove") && PendingOp.ItemInstance != nullptr)
			{
				FIntPoint PredictedTile;
				bool bPredictedRotated;
				if (TryFindPredictedReturnTile(PendingOp.ItemInstance, PredictedTile, bPredictedRotated))
				{
					MoveOrAddPredictedItemWidget(PendingOp.ItemInstance, PredictedTile);
				}
				continue;
			}

			if ((PendingOp.OpType == n"Equipment.Equip" || PendingOp.OpType == n"QuickBar.Add") && PendingOp.ItemInstance != nullptr)
			{
				ApplyPredictedEquipOrQuickBarAdd(PendingOp);
			}
		}
	}

	UFUNCTION()
	void ApplyPredictedSwap(FYcInventoryOperation Op)
	{
		if (GridInventoryManager == nullptr || Op.ItemInstance == nullptr)
		{
			return;
		}

		bool bSourceIsThis = (Op.SourceInventory == GridInventoryManager);
		bool bTargetIsThis = (Op.TargetInventory == GridInventoryManager);
		if (!bSourceIsThis && !bTargetIsThis)
		{
			return;
		}

		if (bSourceIsThis && !bTargetIsThis)
		{
			RemovePredictedItemWidget(Op.ItemInstance);
			return;
		}

		if (bTargetIsThis)
		{
			MoveOrAddPredictedItemWidget(Op.ItemInstance, Op.GridTile);
		}
	}

	UFUNCTION()
	void ApplyPredictedEquipOrQuickBarAdd(FYcInventoryOperation Op)
	{
		if (GridInventoryManager == nullptr || Op.ItemInstance == nullptr)
		{
			return;
		}

		if (Op.SourceInventory == GridInventoryManager)
		{
			RemovePredictedItemWidget(Op.ItemInstance);
		}
	}
	UFUNCTION()
	void RemovePredictedItemWidget(UYcInventoryItemInstance ItemInstance)
	{
		if (ItemInstance == nullptr)
		{
			return;
		}

		UGridItemWidget ItemWidget = nullptr;
		if (!ItemWidgetMap.Find(ItemInstance, ItemWidget))
		{
			return;
		}

		if (ItemWidget != nullptr)
		{
			ItemWidget.RemoveFromParent();
		}
		ItemWidgetMap.Remove(ItemInstance);
	}

	UFUNCTION()
	void MoveOrAddPredictedItemWidget(UYcInventoryItemInstance ItemInstance, FIntPoint Tile)
	{
		if (ItemInstance == nullptr || ItemWidgetClass == nullptr)
		{
			return;
		}

		UGridItemWidget ItemWidget = nullptr;
		if (!ItemWidgetMap.Find(ItemInstance, ItemWidget) || ItemWidget == nullptr)
		{
			ItemWidget = Cast<UGridItemWidget>(WidgetBlueprint::CreateWidget(ItemWidgetClass, OwningPlayer));
			if (ItemWidget == nullptr)
			{
				return;
			}

			ItemWidget.Initialize(ItemInstance, TileSize);
			auto ItemWidgetSlot = Cast<UCanvasPanelSlot>(GridCanvasPanel.AddChild(ItemWidget));
			ItemWidgetSlot.SetAutoSize(true);
			ItemWidgetMap.Add(ItemInstance, ItemWidget);
		}

		auto ItemWidgetCanvasSlot = WidgetLayout::SlotAsCanvasSlot(ItemWidget);
		if (ItemWidgetCanvasSlot != nullptr)
		{
			ItemWidgetCanvasSlot.SetPosition(FVector2D(Tile.X * TileSize, Tile.Y * TileSize));
		}
	}

	UFUNCTION()
	UGridInventoryManagerComponent GetOwningPlayerGridInventory() const
	{
		return Cast<UGridInventoryManagerComponent>(YcInventory::GetInventoryManagerComponent(GetOwningPlayer()));
	}

	UFUNCTION()
	// 搜索会话版本变化时刷新可见性（未知/已识别）
	void UpdateSearchPresentation()
	{
		auto PlayerInventory = GetOwningPlayerGridInventory();
		if (PlayerInventory == nullptr)
		{
			return;
		}

		int32 Revision;
		if (PlayerInventory.GetSearchSessionRevisionForContainer(GridInventoryManager, Revision))
		{
			if (CachedSearchRevision != Revision)
			{
				CachedSearchRevision = Revision;
				bForceItemVisualRefresh = true;
				Refresh();
			}
		}
		else if (CachedSearchRevision != -1)
		{
			CachedSearchRevision = -1;
			bForceItemVisualRefresh = true;
			Refresh();
		}
	}

	UFUNCTION()
	// 网格内容版本变化时刷新物品布局
	void UpdateInventoryPresentation()
	{
		if (GridInventoryManager == nullptr)
		{
			return;
		}

		int32 Revision = GridInventoryManager.GetInventoryGridRevision();
		if (CachedGridRevision != Revision)
		{
			CachedGridRevision = Revision;
			Refresh();
		}
	}

	UFUNCTION()
	void Refresh()
	{
		if (ItemWidgetClass == nullptr)
		{
			Warning("ItemWidgetClass is nullptr!");
			return;
		}

		auto Items = GridInventoryManager.GetGridItemsTileMap();

		// 删除已经不在库存中的Widget，避免整表重建造成闪烁
		TArray<UYcInventoryItemInstance> CachedKeys;
		TArray<UGridItemWidget> CachedWidgets;
		for (auto Entry : ItemWidgetMap)
		{
			CachedKeys.Add(Entry.Key);
			CachedWidgets.Add(Entry.Value);
		}
		for (int32 i = 0; i < CachedKeys.Num(); i++)
		{
			auto ItemInst = CachedKeys[i];
			auto ItemWidget = CachedWidgets[i];
			if (ItemInst == nullptr || !Items.Contains(ItemInst))
			{
				if (ItemWidget != nullptr)
				{
					ItemWidget.RemoveFromParent();
				}
				ItemWidgetMap.Remove(ItemInst);
			}
		}

		// 增量更新：已有Widget仅更新位置和显示，新增物品才创建Widget
		for (auto Item : Items)
		{
			auto ItemInstance = Item.Key;
			auto Pos = Item.Value;
			if (ItemInstance == nullptr)
			{
				continue;
			}

			UGridItemWidget ItemWidget = nullptr;
			if (ItemWidgetMap.Contains(ItemInstance))
			{
				ItemWidget = ItemWidgetMap[ItemInstance];
			}

			if (ItemWidget == nullptr)
			{
				ItemWidget = Cast<UGridItemWidget>(WidgetBlueprint::CreateWidget(ItemWidgetClass, OwningPlayer));
				if (ItemWidget == nullptr)
				{
					Error("Create ItemWidget failed!");
					continue;
				}
				ItemWidget.Initialize(ItemInstance, TileSize);
				auto ItemWidgetSlot = Cast<UCanvasPanelSlot>(GridCanvasPanel.AddChild(ItemWidget));
				ItemWidgetSlot.SetAutoSize(true);
				ItemWidgetMap.Add(ItemInstance, ItemWidget);
			}
			else
			{
				if (bForceItemVisualRefresh)
				{
					ItemWidget.Refresh();
				}
			}

			auto ItemWidgetSlot = WidgetLayout::SlotAsCanvasSlot(ItemWidget);
			ItemWidgetSlot.SetPosition(FVector2D(Pos.X * TileSize, Pos.Y * TileSize));
		}

		bForceItemVisualRefresh = false;

		ReapplyPendingPredictions();
	}

	UFUNCTION()
	bool IsRoomAvailable(UYcInventoryItemInstance ItemInst) const
	{
		return GridInventoryManager.CanPlaceGridItemInst(ItemInst, DraggedItemTopLeftTile);
	}

	UFUNCTION()
	void MousePositionInTile(UYcInventoryItemInstance ItemInst, FVector2D MousePosition, bool&out bRight, bool&out bDown) const
	{
		auto IF_Grid = ItemInst.FindItemFragment(FItemFragment_GridItem).Get(FItemFragment_GridItem);
		int IntTileSize = Math::TruncToInt(TileSize);
		int ItemTileSizeX = IF_Grid.Dimensions.X * IntTileSize;
		int ItemTileSizeY = IF_Grid.Dimensions.Y * IntTileSize;
		int X = Math::TruncToInt(MousePosition.X) % ItemTileSizeX;
		int Y = Math::TruncToInt(MousePosition.Y) % ItemTileSizeY;
		bRight = X > Math::IntegerDivisionTrunc(ItemTileSizeX, 2);
		bDown = Y > Math::IntegerDivisionTrunc(ItemTileSizeY, 2);
	}

	UFUNCTION(BlueprintOverride)
	bool OnDrop(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation Operation)
	{
		bDrawDropLocation = false;
		auto ItemInst = Cast<UYcInventoryItemInstance>(Operation.Payload);
		if (ItemInst == nullptr)
			return true;

		auto Inventory = GetOwningPlayerGridInventory();
		if (Inventory == nullptr || !Inventory.IsItemOperableForCurrentSession(ItemInst))
		{
			return true;
		}

		auto ItemStack = GridInventoryManager.GetStackCountByItemInstance(ItemInst);

        if (!GridInventoryManager.CanPlaceGridItemInst(ItemInst, DraggedItemTopLeftTile, false))
        {
            return true;
        }

		FYcInventoryOperation Op;
		Op.OpType = n"Inventory.SwapGrid";
		Op.ItemInstance = ItemInst;
		Op.SourceInventory = Cast<UYcInventoryManagerComponent>(ItemInst.GetActorOuter().GetComponentByClass(UGridInventoryManagerComponent));

        if (Op.SourceInventory == nullptr)
        {
            return true;
        }
		Op.TargetInventory = GridInventoryManager;
		Op.StackCount = ItemStack;
		Op.GridTile = DraggedItemTopLeftTile;
		Op.bRotated = false;
		auto Router = UYcInventoryOperationRouterComponent::FindOrCreateRouter(GetOwningPlayer());
		if (Router != nullptr)
		{
			Router.SubmitInventoryOperation(Inventory, Op, true);
		}
		return true;
	}

	UFUNCTION(BlueprintOverride)
	void OnDragEnter(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation Operation)
	{
		bDrawDropLocation = true;
	}

	UFUNCTION(BlueprintOverride)
	void OnDragLeave(FPointerEvent PointerEvent, UDragDropOperation Operation)
	{
		bDrawDropLocation = false;
	}

	UFUNCTION(BlueprintOverride)
	bool OnDragOver(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation Operation)
	{
		FVector2D MouseLocalPos = MyGeometry.AbsoluteToLocal(PointerEvent.GetScreenSpacePosition());
		auto ItemInst = Cast<UYcInventoryItemInstance>(Operation.Payload);
		auto Inventory = GetOwningPlayerGridInventory();
		if (ItemInst == nullptr || Inventory == nullptr || !Inventory.IsItemOperableForCurrentSession(ItemInst))
		{
			return false;
		}

		auto IF_Grid = ItemInst.FindItemFragment(FItemFragment_GridItem).Get(FItemFragment_GridItem);
		bool bRight, bDown;
		MousePositionInTile(ItemInst, MouseLocalPos, bRight, bDown);
		int32 PosX = IF_Grid.Dimensions.X - (bRight ? 0 : 1);
		PosX = Math::Clamp(PosX, 0, PosX);
		int32 PosY = IF_Grid.Dimensions.Y - (bDown ? 0 : 1);
		PosY = Math::Clamp(PosY, 0, PosY);
		FVector2D PosXY = FVector2D(PosX, PosY) / 2;
		FVector2D MouseTilePos = MouseLocalPos / TileSize;
		auto FinalTilePos = MouseTilePos - PosXY;
		DraggedItemTopLeftTile = FIntPoint(Math::Clamp(FinalTilePos.X, 0, FinalTilePos.X), Math::Clamp(FinalTilePos.Y, 0, FinalTilePos.Y));
		return true;
	}

	UFUNCTION(BlueprintOverride)
	FEventReply OnPreviewMouseButtonDown(FGeometry InMyGeometry, FPointerEvent InMouseEvent)
	{
		// 预处理阶段拦截空白区域点击（左/右键），稳定关闭右键菜单。
		FGameplayTag CloseMenuTag = FGameplayTag::RequestGameplayTag(n"Yc.Inventory.Message.Grid.ContextMenu.Close");
		FGridItemContextMenuCloseMessage CloseMsg;
		UGameplayMessageSubsystem::Get().BroadcastMessage(CloseMenuTag, CloseMsg);
		return Widget::Unhandled();
	}
}


















