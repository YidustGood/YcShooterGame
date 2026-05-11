// 网格背包UI：负责绘制网格线、拖拽投放预览，并在库存变化时刷新物品Widget
class UGridInventoryWidget : UUserWidget
{
	UPROPERTY(BindWidget)
	UBorder GridBorder;

	UPROPERTY(BindWidget)
	UCanvasPanel GridCanvasPanel;

	// 可选滚动条：用于给玩家明确的滚动提示
	UPROPERTY(meta = (BindWidgetOptional))
	UCanvasPanel ScrollbarCanvas;

	UPROPERTY(meta = (BindWidgetOptional))
	USizeBox ScrollbarThumbSizeBox;

	UPROPERTY(meta = (BindWidgetOptional))
	UBorder ScrollbarBorder;

	UPROPERTY()
	TSubclassOf<UGridItemWidget> ItemWidgetClass;

	UPROPERTY()
	UGridInventoryManagerComponent GridInventoryManager;

	// 网格线段缓存
	private TArray<FGridInventoryLine> Lines;

	// 每个格子的像素大小（用于显示与布局）
	UPROPERTY()
	float TileSize;

	// 视口最多显示多少行；<=0 时展示完整网格
	UPROPERTY()
	int32 MaxVisibleRows = 0;

	// 鼠标滚轮每次滚动的格子行数
	UPROPERTY()
	float ScrollRowsPerWheelStep = 1.0f;

	private int32 Columns;
	private int32 Rows;
	private FGameplayTag CurrentRegionId;
	private int32 CurrentPocketIndex = -1;
	private float ScrollOffsetY = 0.0f;
	private bool bDragHovering = false;
	private bool bScrollbarDragging = false;
	private float ScrollbarDragGrabOffsetY = 0.0f;
	private FVector2D LastDragLocalMousePos = FVector2D(0.0f, 0.0f);
	private TMap<UYcInventoryItemInstance, FIntPoint> ItemWidgetTileMap;

	UPROPERTY()
	FIntPoint DraggedItemTopLeftTile;

	UPROPERTY()
	bool bDrawDropLocation;

	private int32 CachedSearchRevision = -1;
	private int32 CachedGridRevision = -1;
	private bool bForceItemVisualRefresh = false;
	private TMap<UYcInventoryItemInstance, UGridItemWidget> ItemWidgetMap;
	private TArray<FYcInventoryOperation> PendingPredictedOps;
	private float DragAutoScrollSpeed = 600.0f;
	private float DragAutoScrollEdgePadding = 24.0f;

	FGameplayMessageListenerHandle InventoryChangedHandle;
	FGameplayMessageListenerHandle OperationStateHandle;

	UFUNCTION()
	void Initialize(UGridInventoryManagerComponent InGridInventoryManager, float InTileSize, FGameplayTag InRegionId = FGameplayTag(), int32 InPocketIndex = -1)
	{
		if (InGridInventoryManager == nullptr)
		{
			Error("UGridInventoryWidget::Initialize - InGridInventoryManager is nullptr");
			return;
		}

		GridInventoryManager = InGridInventoryManager;
		CurrentRegionId = InRegionId;
		if (!CurrentRegionId.IsValid())
		{
			CurrentRegionId = GridInventoryManager.GetPrimaryRegionId();
		}
		CurrentPocketIndex = InPocketIndex;
		if (CurrentPocketIndex < 0)
		{
			CurrentPocketIndex = GridInventoryManager.GetPrimaryPocketIndex(CurrentRegionId);
		}
		TileSize = InTileSize;
		Columns = GridInventoryManager.GetRegionColumns(CurrentRegionId, CurrentPocketIndex);
		Rows = GridInventoryManager.GetRegionRows(CurrentRegionId, CurrentPocketIndex);
		CachedGridRevision = -1;
		CachedSearchRevision = -1;
		bForceItemVisualRefresh = true;
		ItemWidgetMap.Empty();
		ItemWidgetTileMap.Empty();
		PendingPredictedOps.Empty();
		GridCanvasPanel.ClearChildren();

		CreateLineSegments();
		ApplyViewportLayout();
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
		ItemWidgetTileMap.Empty();
		PendingPredictedOps.Empty();
	}

	UFUNCTION(BlueprintOverride)
	void Tick(FGeometry MyGeometry, float InDeltaTime)
	{
		UpdateInventoryPresentation();
		UpdateSearchPresentation();
		UpdateDragAutoScroll(InDeltaTime);
	}

	UFUNCTION(BlueprintOverride)
	void OnPaint(FPaintContext& Context) const
	{
		auto LocalTopLeft = Slate::GetLocalTopLeft(GridBorder.GetCachedGeometry());
		float ViewportTop = ScrollOffsetY;
		float ViewportBottom = ScrollOffsetY + GetViewportPixelHeight();
		for (int32 i = 0; i < Lines.Num(); i++)
		{
			FGridInventoryLine Line = Lines[i];
			FVector2D PosA;
			FVector2D PosB;
			if (Math::Abs(Line.Start.X - Line.End.X) <= 0.01f)
			{
				float ClampedTop = Math::Max(Line.Start.Y, ViewportTop);
				float ClampedBottom = Math::Min(Line.End.Y, ViewportBottom);
				if (ClampedBottom <= ClampedTop)
				{
					continue;
				}

				PosA = LocalTopLeft + FVector2D(Line.Start.X, ClampedTop - ScrollOffsetY);
				PosB = LocalTopLeft + FVector2D(Line.End.X, ClampedBottom - ScrollOffsetY);
			}
			else
			{
				if (Line.Start.Y < ViewportTop || Line.Start.Y > ViewportBottom)
				{
					continue;
				}

				PosA = LocalTopLeft + FVector2D(Line.Start.X, Line.Start.Y - ScrollOffsetY);
				PosB = LocalTopLeft + FVector2D(Line.End.X, Line.End.Y - ScrollOffsetY);
			}
			Context.DrawLine(PosA, PosB, FLinearColor(0.5, 0.5, 0.5, 0.5));
		}

		if (Widget::IsDragDropping() && bDrawDropLocation)
		{
			auto DraggedItem = Cast<UYcInventoryItemInstance>(Widget::GetDragDroppingContent().Payload);
			if (DraggedItem != nullptr)
			{
				FItemFragment_GridItem IF_Drag;
				if (!TryGetGridFragmentForItem(DraggedItem, IF_Drag, "OnPaint"))
				{
					return;
				}
				bool bIsRoomAvailable = IsRoomAvailable(DraggedItem);
				FLinearColor TintColor = FLinearColor(1, 0, 0, 0.25);
				if (bIsRoomAvailable)
				{
					TintColor = FLinearColor(0, 1, 0, 0.25);
				}
				FVector2D Position = FVector2D(DraggedItemTopLeftTile.X * TileSize, DraggedItemTopLeftTile.Y * TileSize - ScrollOffsetY);
				FVector2D Size = FVector2D(IF_Drag.Dimensions.X * TileSize, IF_Drag.Dimensions.Y * TileSize);
				if (Position.Y + Size.Y > 0.0f && Position.Y < GetViewportPixelHeight())
				{
					Context.DrawBox(Position, Size, TintColor);
				}
			}
		}

		// 若当前容器正在被搜索，在网格顶部绘制搜索进度条
		auto PlayerInventory = GetOwningPlayerGridInventory();
		if (PlayerInventory != nullptr)
		{
			int32 Revision;
			if (!PlayerInventory.GetSearchSessionRevisionForContainer(GridInventoryManager, Revision))
			{
				return;
			}

			float32 Progress01;
			float32 RemainingSeconds;
			int32 RevealedCount;
			int32 TotalCount;
			if (PlayerInventory.GetCurrentSearchProgress(Progress01, RemainingSeconds, RevealedCount, TotalCount))
			{
				float FullWidth = Columns * TileSize;
				float BarHeight = 4.0f;
				// 搜索条固定在视口顶部，滚动时也始终可见。
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
	void SetMaxVisibleRows(int32 InMaxVisibleRows)
	{
		MaxVisibleRows = Math::Max(0, InMaxVisibleRows);
		ApplyViewportLayout();
	}

	UFUNCTION(BlueprintPure)
	float GetContentPixelWidth() const
	{
		return Columns * TileSize;
	}

	UFUNCTION(BlueprintPure)
	float GetContentPixelHeight() const
	{
		return Rows * TileSize;
	}

	UFUNCTION(BlueprintPure)
	float GetViewportPixelHeight() const
	{
		int32 VisibleRows = MaxVisibleRows > 0 ? Math::Min(Rows, MaxVisibleRows) : Rows;
		return VisibleRows * TileSize;
	}

	UFUNCTION(BlueprintPure)
	bool IsScrollable() const
	{
		return GetMaxScrollOffsetY() > 0.0f;
	}

	UFUNCTION()
	private float GetMaxScrollOffsetY() const
	{
		return Math::Max(0.0f, GetContentPixelHeight() - GetViewportPixelHeight());
	}

	UFUNCTION()
	private void ApplyViewportLayout()
	{
		float ViewportWidth = GetContentPixelWidth();
		float ViewportHeight = GetViewportPixelHeight();

		auto GridBorderSlot = WidgetLayout::SlotAsCanvasSlot(GridBorder);
		if (GridBorderSlot != nullptr)
		{
			GridBorderSlot.SetSize(FVector2D(ViewportWidth, ViewportHeight));
		}

		auto GridCanvasSlot = WidgetLayout::SlotAsCanvasSlot(GridCanvasPanel);
		if (GridCanvasSlot != nullptr)
		{
			GridCanvasSlot.SetSize(FVector2D(ViewportWidth, ViewportHeight));
		}

		auto ScrollbarCanvasSlot = WidgetLayout::SlotAsCanvasSlot(ScrollbarCanvas);
		if (ScrollbarCanvasSlot != nullptr)
		{
			ScrollbarCanvasSlot.SetSize(FVector2D(ScrollbarCanvasSlot.GetSize().X, ViewportHeight));
		}

		SetScrollOffsetY(ScrollOffsetY);
	}

	UFUNCTION()
	private void SetScrollOffsetY(float InScrollOffsetY)
	{
		ScrollOffsetY = Math::Clamp(InScrollOffsetY, 0.0f, GetMaxScrollOffsetY());
		ApplyItemWidgetViewportState();
		UpdateScrollbarVisual();
	}

	UFUNCTION()
	private void UpdateScrollbarVisual()
	{
		if (ScrollbarCanvas == nullptr)
		{
			return;
		}

		bool bShouldShowScrollbar = IsScrollable();
		ScrollbarCanvas.SetVisibility(bShouldShowScrollbar ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		if (!bShouldShowScrollbar || ScrollbarThumbSizeBox == nullptr)
		{
			return;
		}

		auto ThumbSlot = WidgetLayout::SlotAsCanvasSlot(ScrollbarThumbSizeBox);
		if (ThumbSlot == nullptr)
		{
			return;
		}

		float ThumbHeight;
		float AvailableTravel;
		float ThumbTop;
		GetScrollbarThumbMetrics(ThumbHeight, AvailableTravel, ThumbTop);
		ScrollbarThumbSizeBox.SetHeightOverride(ThumbHeight);

		ThumbSlot.SetAnchors(FAnchors(0.0f, 0.0f, 0.0f, 0.0f));
		ThumbSlot.Alignment = FVector2D(0.0f, 0.0f);
		ThumbSlot.SetSize(FVector2D(ThumbSlot.GetSize().X, ThumbHeight));
		ThumbSlot.SetPosition(FVector2D(0.0f, ThumbTop));
	}

	UFUNCTION()
	private float GetScrollbarTrackHeight() const
	{
		return GetViewportPixelHeight();
	}

	UFUNCTION()
	private bool TryGetScrollbarLocalPosition(FVector2D ScreenSpacePosition, FVector2D&out OutLocalPos) const
	{
		OutLocalPos = FVector2D(0.0f, 0.0f);
		if (ScrollbarCanvas == nullptr || !IsScrollable())
		{
			return false;
		}

		FGeometry ScrollbarGeometry = ScrollbarCanvas.GetCachedGeometry();
		OutLocalPos = ScrollbarGeometry.AbsoluteToLocal(ScreenSpacePosition);
		FVector2D LocalSize = ScrollbarGeometry.GetLocalSize();
		auto ScrollbarCanvasSlot = WidgetLayout::SlotAsCanvasSlot(ScrollbarCanvas);
		if (ScrollbarCanvasSlot != nullptr)
		{
			FVector2D SlotSize = ScrollbarCanvasSlot.GetSize();
			if (SlotSize.X > 0.0f)
			{
				LocalSize.X = SlotSize.X;
			}
			if (SlotSize.Y > 0.0f)
			{
				LocalSize.Y = SlotSize.Y;
			}
		}
		return OutLocalPos.X >= 0.0f && OutLocalPos.Y >= 0.0f && OutLocalPos.X <= LocalSize.X && OutLocalPos.Y <= LocalSize.Y;
	}

	UFUNCTION()
	private bool GetScrollbarLocalPositionUnbounded(FVector2D ScreenSpacePosition, FVector2D&out OutLocalPos) const
	{
		OutLocalPos = FVector2D(0.0f, 0.0f);
		if (ScrollbarCanvas == nullptr || !IsScrollable())
		{
			return false;
		}

		OutLocalPos = ScrollbarCanvas.GetCachedGeometry().AbsoluteToLocal(ScreenSpacePosition);
		return true;
	}

	UFUNCTION()
	private bool IsScreenSpacePositionOverScrollbarThumb(FVector2D ScreenSpacePosition) const
	{
		if (!IsScrollable())
		{
			return false;
		}

		FGeometry ThumbGeometry;
		if (ScrollbarBorder != nullptr)
		{
			ThumbGeometry = ScrollbarBorder.GetCachedGeometry();
		}
		else if (ScrollbarThumbSizeBox != nullptr)
		{
			ThumbGeometry = ScrollbarThumbSizeBox.GetCachedGeometry();
		}
		else
		{
			return false;
		}

		FVector2D ThumbLocalPos = ThumbGeometry.AbsoluteToLocal(ScreenSpacePosition);
		FVector2D ThumbLocalSize = ThumbGeometry.GetLocalSize();
		return ThumbLocalPos.X >= 0.0f && ThumbLocalPos.Y >= 0.0f && ThumbLocalPos.X <= ThumbLocalSize.X && ThumbLocalPos.Y <= ThumbLocalSize.Y;
	}

	UFUNCTION()
	private void GetScrollbarThumbMetrics(float&out OutThumbHeight, float&out OutAvailableTravel, float&out OutThumbTop) const
	{
		float TrackHeight = GetScrollbarTrackHeight();
		float ViewportHeight = GetViewportPixelHeight();
		float ContentHeight = Math::Max(GetContentPixelHeight(), 1.0f);
		OutThumbHeight = Math::Max(24.0f, TrackHeight * (ViewportHeight / ContentHeight));
		OutThumbHeight = Math::Min(OutThumbHeight, TrackHeight);
		OutAvailableTravel = Math::Max(0.0f, TrackHeight - OutThumbHeight);

		float ScrollAlpha = 0.0f;
		float MaxScrollOffset = GetMaxScrollOffsetY();
		if (MaxScrollOffset > 0.0f)
		{
			ScrollAlpha = ScrollOffsetY / MaxScrollOffset;
		}
		OutThumbTop = OutAvailableTravel * ScrollAlpha;
	}

	UFUNCTION()
	private void SetScrollOffsetFromScrollbarTrack(float LocalY, bool bKeepGrabOffset)
	{
		float ThumbHeight;
		float AvailableTravel;
		float ThumbTop;
		GetScrollbarThumbMetrics(ThumbHeight, AvailableTravel, ThumbTop);
		if (AvailableTravel <= 0.0f)
		{
			SetScrollOffsetY(0.0f);
			return;
		}

		float DesiredThumbTop = 0.0f;
		if (bKeepGrabOffset)
		{
			DesiredThumbTop = LocalY - ScrollbarDragGrabOffsetY;
		}
		else
		{
			DesiredThumbTop = LocalY - ThumbHeight * 0.5f;
			ScrollbarDragGrabOffsetY = ThumbHeight * 0.5f;
		}

		DesiredThumbTop = Math::Clamp(DesiredThumbTop, 0.0f, AvailableTravel);
		float ScrollAlpha = DesiredThumbTop / AvailableTravel;
		SetScrollOffsetY(GetMaxScrollOffsetY() * ScrollAlpha);
	}

	UFUNCTION()
	private void UpdateDragAutoScroll(float InDeltaTime)
	{
		if (!bDragHovering || !Widget::IsDragDropping() || !IsScrollable())
		{
			return;
		}

		float ViewportHeight = GetViewportPixelHeight();
		if (LastDragLocalMousePos.Y < DragAutoScrollEdgePadding)
		{
			float Strength = 1.0f - Math::Clamp(LastDragLocalMousePos.Y / DragAutoScrollEdgePadding, 0.0f, 1.0f);
			SetScrollOffsetY(ScrollOffsetY - DragAutoScrollSpeed * Strength * InDeltaTime);
		}
		else if (LastDragLocalMousePos.Y > ViewportHeight - DragAutoScrollEdgePadding)
		{
			float DistanceToBottom = ViewportHeight - LastDragLocalMousePos.Y;
			float Strength = 1.0f - Math::Clamp(DistanceToBottom / DragAutoScrollEdgePadding, 0.0f, 1.0f);
			SetScrollOffsetY(ScrollOffsetY + DragAutoScrollSpeed * Strength * InDeltaTime);
		}
	}

	UFUNCTION()
	private void ScrollTileRectIntoView(FIntPoint Tile, FIntPoint Dimensions)
	{
		if (!IsScrollable())
		{
			return;
		}

		float RectTop = Tile.Y * TileSize;
		float RectBottom = RectTop + Dimensions.Y * TileSize;
		float ViewportHeight = GetViewportPixelHeight();
		float TargetScrollOffsetY = ScrollOffsetY;

		if (RectTop < ScrollOffsetY)
		{
			TargetScrollOffsetY = RectTop;
		}
		else if (RectBottom > ScrollOffsetY + ViewportHeight)
		{
			TargetScrollOffsetY = RectBottom - ViewportHeight;
		}

		SetScrollOffsetY(TargetScrollOffsetY);
	}

	UFUNCTION()
	private void ScrollItemIntoView(UYcInventoryItemInstance ItemInstance, FIntPoint Tile)
	{
		if (ItemInstance == nullptr)
		{
			return;
		}

		FItemFragment_GridItem GridFragment;
		if (!TryGetGridFragmentForItem(ItemInstance, GridFragment, "ScrollItemIntoView"))
		{
			return;
		}

		ScrollTileRectIntoView(Tile, GridFragment.Dimensions);
	}

	UFUNCTION()
	private bool IsTileRectVisible(FIntPoint Tile, FIntPoint Dimensions) const
	{
		float Top = Tile.Y * TileSize;
		float Bottom = Top + Dimensions.Y * TileSize;
		return Bottom > ScrollOffsetY && Top < ScrollOffsetY + GetViewportPixelHeight();
	}

	UFUNCTION()
	private void ApplyItemWidgetViewportState()
	{
		for (auto Entry : ItemWidgetMap)
		{
			auto ItemInstance = Entry.Key;
			auto ItemWidget = Entry.Value;
			if (ItemInstance == nullptr || ItemWidget == nullptr)
			{
				continue;
			}

			FIntPoint Tile;
			if (!ItemWidgetTileMap.Find(ItemInstance, Tile))
			{
				continue;
			}

			UpdateItemWidgetLayout(ItemInstance, ItemWidget, Tile);
		}
	}

	UFUNCTION()
	private void UpdateItemWidgetLayout(UYcInventoryItemInstance ItemInstance, UGridItemWidget ItemWidget, FIntPoint Tile)
	{
		if (ItemInstance == nullptr || ItemWidget == nullptr)
		{
			return;
		}

		FItemFragment_GridItem GridFragment;
		if (!TryGetGridFragmentForItem(ItemInstance, GridFragment, "UpdateItemWidgetLayout"))
		{
			ItemWidget.SetVisibility(ESlateVisibility::Collapsed);
			return;
		}

		auto ItemWidgetSlot = WidgetLayout::SlotAsCanvasSlot(ItemWidget);
		if (ItemWidgetSlot != nullptr)
		{
			ItemWidgetSlot.SetPosition(FVector2D(Tile.X * TileSize, Tile.Y * TileSize - ScrollOffsetY));
		}

		// 仅显示视口内的物品，避免超出视口的控件露出来。
		ItemWidget.SetVisibility(IsTileRectVisible(Tile, GridFragment.Dimensions) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
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
				if (Data.Operation.TargetInventory == GridInventoryManager && (!Data.Operation.GridRegionId.IsValid() || Data.Operation.GridRegionId == CurrentRegionId) && (Data.Operation.GridPocketIndex < 0 || Data.Operation.GridPocketIndex == CurrentPocketIndex))
				{
					ScrollItemIntoView(Data.Operation.ItemInstance, Data.Operation.GridTile);
				}
				ApplyPredictedSwap(Data.Operation);
			}
			else if ((Data.Operation.OpType == n"Equipment.Unequip" || Data.Operation.OpType == n"QuickBar.Remove") && Data.Operation.ItemInstance != nullptr)
			{
				FIntPoint PredictedTile;
				bool bPredictedRotated;
				if (TryFindPredictedReturnTile(Data.Operation, PredictedTile, bPredictedRotated))
				{
					ScrollItemIntoView(Data.Operation.ItemInstance, PredictedTile);
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
			return GridInventoryManager.CanPlaceGridItemInst(Op.ItemInstance, Op.GridTile, Op.bRotated, Op.GridRegionId, Op.GridPocketIndex);
		}

		return true;
	}
	UFUNCTION()
	bool TryFindPredictedReturnTile(FYcInventoryOperation Op, FIntPoint&out OutTile, bool&out OutRotated) const
	{
		OutTile = FIntPoint(0, 0);
		OutRotated = false;
		if (GridInventoryManager == nullptr || Op.ItemInstance == nullptr)
		{
			return false;
		}

		if (Op.TargetInventory == GridInventoryManager && Op.GridRegionId == CurrentRegionId && Op.GridPocketIndex == CurrentPocketIndex && GridInventoryManager.CanPlaceGridItemInst(Op.ItemInstance, Op.GridTile, Op.bRotated, Op.GridRegionId, Op.GridPocketIndex))
		{
			OutTile = Op.GridTile;
			OutRotated = Op.bRotated;
			return true;
		}

		TMap<UYcInventoryItemInstance, FIntPoint> Items = GridInventoryManager.GetGridItemsTileMapByRegion(CurrentRegionId, CurrentPocketIndex);
		if (Items.Find(Op.ItemInstance, OutTile))
		{
			return true;
		}

		return GridInventoryManager.FindFirstFitPositionInRegion(Op.ItemInstance.ItemRegistryId, CurrentRegionId, CurrentPocketIndex, OutTile, OutRotated);
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
				if (TryFindPredictedReturnTile(PendingOp, PredictedTile, bPredictedRotated))
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

		bool bSourceIsThis = (Op.SourceInventory == GridInventoryManager && (!Op.SourceGridRegionId.IsValid() || Op.SourceGridRegionId == CurrentRegionId) && (Op.SourceGridPocketIndex < 0 || Op.SourceGridPocketIndex == CurrentPocketIndex));
		bool bTargetIsThis = (Op.TargetInventory == GridInventoryManager && (!Op.GridRegionId.IsValid() || Op.GridRegionId == CurrentRegionId) && (Op.GridPocketIndex < 0 || Op.GridPocketIndex == CurrentPocketIndex));
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
		ItemWidgetTileMap.Remove(ItemInstance);
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
		ItemWidgetTileMap.Add(ItemInstance, Tile);
		UpdateItemWidgetLayout(ItemInstance, ItemWidget, Tile);
	}

	UFUNCTION()
	UGridInventoryManagerComponent GetOwningPlayerGridInventory() const
	{
		return Cast<UGridInventoryManagerComponent>(YcInventory::GetInventoryManagerComponent(GetOwningPlayer()));
	}

	UFUNCTION()
	bool TryGetGridFragmentForItem(UYcInventoryItemInstance ItemInst, FItemFragment_GridItem&out OutGridFragment, FString Context) const
	{
		if (ItemInst == nullptr)
		{
			Warning(f"UGridInventoryWidget::{Context} failed: ItemInst is nullptr.");
			return false;
		}

		FInstancedStruct GridFragmentResult = ItemInst.FindItemFragment(FItemFragment_GridItem);
		if (!GridFragmentResult.IsValid())
		{
			AActor OuterActor = ItemInst.GetActorOuter();
			FString OuterName = OuterActor != nullptr ? OuterActor.GetName().ToString() : "None";
			Warning(f"UGridInventoryWidget::{Context} failed: missing FItemFragment_GridItem. Item={ItemInst.GetName()} RegistryId={ItemInst.ItemRegistryId.ToString()} Outer={OuterName} Region={CurrentRegionId.ToString()} Pocket={CurrentPocketIndex}");
			return false;
		}

		OutGridFragment = GridFragmentResult.Get(FItemFragment_GridItem);
		return true;
	}

	UFUNCTION()
	// 搜索会话版本变化时刷新可见性（未知/已识别状态）
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
	// 背包网格版本变化时刷新物品显示
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
		if (GridInventoryManager == nullptr)
		{
			Warning("UGridInventoryWidget::Refresh ignored: GridInventoryManager is nullptr.");
			return;
		}

		if (ItemWidgetClass == nullptr)
		{
			Warning("ItemWidgetClass is nullptr!");
			return;
		}

		auto Items = GridInventoryManager.GetGridItemsTileMapByRegion(CurrentRegionId, CurrentPocketIndex);

		// 删除已不在当前网格中的缓存Widget，避免残留
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
				ItemWidgetTileMap.Remove(ItemInst);
			}
		}

		// 为每个物品创建或复用Widget，并更新其网格位置
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

			ItemWidgetTileMap.Add(ItemInstance, Pos);
			UpdateItemWidgetLayout(ItemInstance, ItemWidget, Pos);
		}

		bForceItemVisualRefresh = false;

		ReapplyPendingPredictions();
	}

	UFUNCTION()
	bool IsRoomAvailable(UYcInventoryItemInstance ItemInst) const
	{
		return GridInventoryManager.CanPlaceGridItemInst(ItemInst, DraggedItemTopLeftTile, false, CurrentRegionId, CurrentPocketIndex);
	}

	UFUNCTION()
	void MousePositionInTile(UYcInventoryItemInstance ItemInst, FVector2D MousePosition, bool&out bRight, bool&out bDown) const
	{
		FItemFragment_GridItem IF_Grid;
		if (!TryGetGridFragmentForItem(ItemInst, IF_Grid, "MousePositionInTile"))
		{
			bRight = false;
			bDown = false;
			return;
		}
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
		bDragHovering = false;
		auto ItemInst = Cast<UYcInventoryItemInstance>(Operation.Payload);
		if (ItemInst == nullptr)
			return true;

		auto Inventory = GetOwningPlayerGridInventory();
		if (Inventory == nullptr || !Inventory.IsItemOperableForCurrentSession(ItemInst))
		{
			return true;
		}

		FYcInventoryOperation SlotReturnOp;
		if (TryBuildEquipmentUnequipOperation(ItemInst, Inventory, SlotReturnOp) || TryBuildQuickBarRemoveOperation(ItemInst, Inventory, SlotReturnOp))
		{
			if (!GridInventoryManager.CanPlaceGridItemInst(ItemInst, DraggedItemTopLeftTile, false, CurrentRegionId, CurrentPocketIndex))
			{
				return true;
			}

			SlotReturnOp.GridTile = DraggedItemTopLeftTile;
			SlotReturnOp.bRotated = false;
			SlotReturnOp.GridRegionId = CurrentRegionId;
			SlotReturnOp.GridPocketIndex = CurrentPocketIndex;

			if (SlotReturnOp.OpType == n"QuickBar.Remove")
			{
				FYcQuickBarSlotRemovedMessage RemovedMessage;
				RemovedMessage.Owner = GetOwningPlayer();
				RemovedMessage.SlotIndex = SlotReturnOp.SlotIndex;
				UGameplayMessageSubsystem::Get().BroadcastMessage(GameplayTags::Yc_QuickBar_Message_SlotRemoved, RemovedMessage);
			}

			auto Router = UYcInventoryOperationRouterComponent::FindOrCreateRouter(GetOwningPlayer());
			if (Router != nullptr)
			{
				Router.SubmitInventoryOperation(Inventory, SlotReturnOp, true);
			}
			return true;
		}

		auto ItemStack = GridInventoryManager.GetStackCountByItemInstance(ItemInst);

		if (!GridInventoryManager.CanPlaceGridItemInst(ItemInst, DraggedItemTopLeftTile, false, CurrentRegionId, CurrentPocketIndex))
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
		auto SourceGridInventory = Cast<UGridInventoryManagerComponent>(Op.SourceInventory);
		if (SourceGridInventory != nullptr)
		{
			SourceGridInventory.GetItemPlacementRegion(ItemInst, Op.SourceGridRegionId, Op.SourceGridPocketIndex);
		}
		Op.TargetInventory = GridInventoryManager;
		Op.StackCount = ItemStack;
		Op.GridTile = DraggedItemTopLeftTile;
		Op.bRotated = false;
		Op.GridRegionId = CurrentRegionId;
		Op.GridPocketIndex = CurrentPocketIndex;
		auto Router = UYcInventoryOperationRouterComponent::FindOrCreateRouter(GetOwningPlayer());
		if (Router != nullptr)
		{
			Router.SubmitInventoryOperation(Inventory, Op, true);
		}
		return true;
	}

	UFUNCTION()
	bool TryBuildEquipmentUnequipOperation(UYcInventoryItemInstance ItemInst, UGridInventoryManagerComponent Inventory, FYcInventoryOperation&out OutOp) const
	{
		if (ItemInst == nullptr || Inventory == nullptr)
		{
			return false;
		}

		auto EquipmentSlotComp = UYcEquipmentSlotComponent::FindEquipmentSlotComponent(GetOwningPlayer());
		if (EquipmentSlotComp == nullptr)
		{
			return false;
		}

		auto Slots = EquipmentSlotComp.GetSlots();
		for (auto Slot : Slots)
		{
			if (Slot.ItemInstance != ItemInst)
			{
				continue;
			}

			OutOp.OpType = n"Equipment.Unequip";
			OutOp.ItemInstance = ItemInst;
			OutOp.SlotTag = Slot.SlotTag;
			OutOp.SourceInventory = Inventory;
			OutOp.TargetInventory = Inventory;
			return true;
		}

		return false;
	}

	UFUNCTION()
	bool TryBuildQuickBarRemoveOperation(UYcInventoryItemInstance ItemInst, UGridInventoryManagerComponent Inventory, FYcInventoryOperation&out OutOp) const
	{
		if (ItemInst == nullptr || Inventory == nullptr)
		{
			return false;
		}

		auto QuickBarComp = UYcQuickBarComponent::FindQuickBarComponent(GetOwningPlayer());
		if (QuickBarComp == nullptr)
		{
			return false;
		}

		auto Slots = QuickBarComp.GetSlots();
		for (int32 Index = 0; Index < Slots.Num(); ++Index)
		{
			if (Slots[Index] != ItemInst)
			{
				continue;
			}

			OutOp.OpType = n"QuickBar.Remove";
			OutOp.ItemInstance = ItemInst;
			OutOp.SlotIndex = Index;
			OutOp.SourceInventory = Inventory;
			OutOp.TargetInventory = Inventory;
			return true;
		}

		return false;
	}

	UFUNCTION(BlueprintOverride)
	void OnDragEnter(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation Operation)
	{
		bDrawDropLocation = true;
		bDragHovering = true;
		LastDragLocalMousePos = MyGeometry.AbsoluteToLocal(PointerEvent.GetScreenSpacePosition());
	}

	UFUNCTION(BlueprintOverride)
	void OnDragLeave(FPointerEvent PointerEvent, UDragDropOperation Operation)
	{
		bDrawDropLocation = false;
		bDragHovering = false;
	}

	UFUNCTION(BlueprintOverride)
	bool OnDragOver(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation Operation)
	{
		FVector2D MouseLocalPos = MyGeometry.AbsoluteToLocal(PointerEvent.GetScreenSpacePosition());
		LastDragLocalMousePos = MouseLocalPos;
		FVector2D ContentLocalPos = MouseLocalPos + FVector2D(0.0f, ScrollOffsetY);
		auto ItemInst = Cast<UYcInventoryItemInstance>(Operation.Payload);
		auto Inventory = GetOwningPlayerGridInventory();
		if (ItemInst == nullptr || Inventory == nullptr || !Inventory.IsItemOperableForCurrentSession(ItemInst))
		{
			return false;
		}

		FItemFragment_GridItem IF_Grid;
		if (!TryGetGridFragmentForItem(ItemInst, IF_Grid, "OnDragOver"))
		{
			return false;
		}
		bool bRight, bDown;
		MousePositionInTile(ItemInst, ContentLocalPos, bRight, bDown);
		int32 PosX = IF_Grid.Dimensions.X - (bRight ? 0 : 1);
		PosX = Math::Clamp(PosX, 0, PosX);
		int32 PosY = IF_Grid.Dimensions.Y - (bDown ? 0 : 1);
		PosY = Math::Clamp(PosY, 0, PosY);
		FVector2D PosXY = FVector2D(PosX, PosY) / 2;
		FVector2D MouseTilePos = ContentLocalPos / TileSize;
		auto FinalTilePos = MouseTilePos - PosXY;
		DraggedItemTopLeftTile = FIntPoint(Math::Clamp(FinalTilePos.X, 0, FinalTilePos.X), Math::Clamp(FinalTilePos.Y, 0, FinalTilePos.Y));
		return true;
	}

	UFUNCTION(BlueprintOverride)
	FEventReply OnMouseWheel(FGeometry MyGeometry, FPointerEvent MouseEvent)
	{
		if (!IsScrollable())
		{
			return Widget::Unhandled();
		}

		float WheelDelta = MouseEvent.GetWheelDelta();
		if (Math::Abs(WheelDelta) <= 0.01f)
		{
			return Widget::Unhandled();
		}

		SetScrollOffsetY(ScrollOffsetY - WheelDelta * ScrollRowsPerWheelStep * TileSize);
		return Widget::Handled();
	}

	UFUNCTION(BlueprintOverride)
	FEventReply OnMouseMove(FGeometry MyGeometry, FPointerEvent MouseEvent)
	{
		if (bScrollbarDragging)
		{
			FVector2D ScrollbarLocalPos;
			if (GetScrollbarLocalPositionUnbounded(MouseEvent.GetScreenSpacePosition(), ScrollbarLocalPos))
			{
				SetScrollOffsetFromScrollbarTrack(ScrollbarLocalPos.Y, true);
			}
			return Widget::Handled();
		}

		return Widget::Unhandled();
	}

	UFUNCTION(BlueprintOverride)
	FEventReply OnMouseButtonUp(FGeometry InMyGeometry, FPointerEvent InMouseEvent)
	{
		if (bScrollbarDragging)
		{
			bScrollbarDragging = false;
			auto Reply = FEventReply::Handled();
			Reply.ReleaseMouseCapture();
			return Reply;
		}

		return Widget::Unhandled();
	}

	UFUNCTION(BlueprintOverride)
	FEventReply OnPreviewMouseButtonDown(FGeometry InMyGeometry, FPointerEvent InMouseEvent)
	{
		// 预处理鼠标按下：点击空白处时关闭右键菜单，避免菜单残留
		FGameplayTag CloseMenuTag = FGameplayTag::RequestGameplayTag(n"Yc.Inventory.Message.Grid.ContextMenu.Close");
		FGridItemContextMenuCloseMessage CloseMsg;
		UGameplayMessageSubsystem::Get().BroadcastMessage(CloseMenuTag, CloseMsg);

		FKey EffectingButton = InMouseEvent.GetEffectingButton();
		if (EffectingButton.KeyName == n"LeftMouseButton")
		{
			FVector2D ScrollbarLocalPos;
			if (TryGetScrollbarLocalPosition(InMouseEvent.GetScreenSpacePosition(), ScrollbarLocalPos))
			{
				float ThumbHeight;
				float AvailableTravel;
				float ThumbTop;
				GetScrollbarThumbMetrics(ThumbHeight, AvailableTravel, ThumbTop);
				if (IsScreenSpacePositionOverScrollbarThumb(InMouseEvent.GetScreenSpacePosition()))
				{
					// 点中滑块时记录抓取偏移，拖动时手感会稳定很多。
					ScrollbarDragGrabOffsetY = ScrollbarLocalPos.Y - ThumbTop;
				}
				else
				{
					// 点轨道时先跳到该位置，再进入拖动态。
					SetScrollOffsetFromScrollbarTrack(ScrollbarLocalPos.Y, false);
				}

				bScrollbarDragging = true;
				auto Reply = FEventReply::Handled();
				Reply.CaptureMouse(this);
				return Reply;
			}
		}

		return Widget::Unhandled();
	}
}
