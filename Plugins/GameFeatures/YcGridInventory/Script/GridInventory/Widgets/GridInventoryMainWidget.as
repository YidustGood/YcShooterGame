/** 网格库存UI界面的父UI, 网格库存UI需要放置在这个UI里, 以确保其能正确计算相对位置来处理物品网格放置位置 */
UCLASS(Abstract)
// 背包主界面：管理“玩家背包网格 + 当前交互容器网格”的双面板显示
class UGridInventoryMainWidget : UYcActivatableWidget
{
	UPROPERTY(BindWidget)
	UCanvasPanel Canvas;

	UPROPERTY(BindWidget)
	UGridInventoryWidget GridInventory;

	UPROPERTY()
	TSubclassOf<UGridItemContextMenuWidget> ContextMenuWidgetClass;

	UPROPERTY(BlueprintReadOnly)
	UGridInventoryManagerComponent OwnerInventory;

	UPROPERTY()
	float TileSize;

	default TileSize = 50;
	default InputConfig = EYcWidgetInputMode::GameAndMenu;

	private FGameplayMessageListenerHandle GridItemContainerMsgHandle;
	private FGameplayMessageListenerHandle SwapItemMsgHandle;
	private FGameplayMessageListenerHandle ContextMenuOpenMsgHandle;
	private FGameplayMessageListenerHandle ContextMenuClickMsgHandle;
	private FGameplayMessageListenerHandle ContextMenuCloseMsgHandle;
	private FGameplayMessageListenerHandle EquipmentSlotChangedMsgHandle;

	// 当前的物品容器界面
	private UGridInventoryWidget CurrentContainerWidget;
	private TArray<UGridInventoryWidget> OwnerRegionWidgets;
	private UGridItemContextMenuWidget CurrentContextMenuWidget;
	private TOptional<FGridItemContainerMessage> CachedCurrentContainerMsg;

	UFUNCTION(BlueprintOverride)
	void OnInitialized()
	{
	}

	UFUNCTION(BlueprintOverride)
	void OnActivated()
	{
		OwnerInventory = Cast<UGridInventoryManagerComponent>(YcInventory::GetInventoryManagerComponent(GetOwningPlayer()));
		if (OwnerInventory == nullptr)
		{
			Error("UGridInventoryMainWidget::Initialize - Owner GridInventoryManagerComponent is nullptr");
			return;
		}

		BuildOwnerRegionWidgets();

		GridItemContainerMsgHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			GameplayTags::Yc_Inventory_Message_Grid_Container_Push,
			this,
			n"OnReceivedGridItemContainerMsg",
			FGridItemContainerMessage(),
			EGameplayMessageMatch::ExactMatch);

		SwapItemMsgHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			GameplayTags::Yc_Inventory_Message_Grid_Container_SwapItem,
			this,
			n"OnReceivedSwapItemMessage",
			FSwapGridItemMessage(),
			EGameplayMessageMatch::ExactMatch);

		FGameplayTag ContextOpenTag = FGameplayTag::RequestGameplayTag(n"Yc.Inventory.Message.Grid.ContextMenu.Open");
		ContextMenuOpenMsgHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			ContextOpenTag,
			this,
			n"OnReceivedContextMenuOpenMessage",
			FGridItemContextMenuOpenMessage(),
			EGameplayMessageMatch::ExactMatch);

		FGameplayTag ContextClickTag = FGameplayTag::RequestGameplayTag(n"Yc.Inventory.Message.Grid.ContextMenu.Click");
		ContextMenuClickMsgHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			ContextClickTag,
			this,
			n"OnReceivedContextMenuClickMessage",
			FGridItemContextMenuClickMessage(),
			EGameplayMessageMatch::ExactMatch);

		FGameplayTag ContextCloseTag = FGameplayTag::RequestGameplayTag(n"Yc.Inventory.Message.Grid.ContextMenu.Close");
		ContextMenuCloseMsgHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			ContextCloseTag,
			this,
			n"OnReceivedContextMenuCloseMessage",
			FGridItemContextMenuCloseMessage(),
			EGameplayMessageMatch::ExactMatch);

		EquipmentSlotChangedMsgHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			GameplayTags::Yc_EquipmentSlot_Message_SlotChanged,
			this,
			n"OnOwnerEquipmentSlotChanged",
			FYcEquipmentSlotChangedMessage(),
			EGameplayMessageMatch::ExactMatch);
	}

	UFUNCTION(BlueprintOverride)
	void OnDeactivated()
	{
		// 关闭界面时仅重置当前容器搜索会话进度，不清理本局已识别物品（满足同局记忆）
		if (OwnerInventory != nullptr)
		{
			OwnerInventory.ServerResetSearchSession();
		}

		SwapItemMsgHandle.Unregister();
		GridItemContainerMsgHandle.Unregister();
		ContextMenuOpenMsgHandle.Unregister();
		ContextMenuClickMsgHandle.Unregister();
		ContextMenuCloseMsgHandle.Unregister();
		EquipmentSlotChangedMsgHandle.Unregister();

		if (CurrentContainerWidget != nullptr)
			CurrentContainerWidget.RemoveFromParent();
		for (int32 i = 0; i < OwnerRegionWidgets.Num(); i++)
		{
			if (OwnerRegionWidgets[i] != nullptr && OwnerRegionWidgets[i] != GridInventory)
			{
				OwnerRegionWidgets[i].RemoveFromParent();
			}
		}
		OwnerRegionWidgets.Empty();
		CloseContextMenu();
		CachedCurrentContainerMsg.Reset();
	}

	UFUNCTION(BlueprintOverride)
	bool OnDrop(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation Operation)
	{
		CloseContextMenu();
		Print("UGridInventoryMainWidget::OnDrop.");
		return true;
	}

	UFUNCTION()
	void OnReceivedGridItemContainerMsg(FGameplayTag ActualTag, FGridItemContainerMessage Data)
	{
		CloseContextMenu();

		if (Data.Player != nullptr && Data.Player != GetOwningPlayerPawn())
		{
			return;
		}

		if (Data.bIsOpen)
		{
			CachedCurrentContainerMsg = Data;
			CurrentContainerWidget = Cast<UGridInventoryWidget>(WidgetBlueprint::CreateWidget(Data.ContainerWidgetClass, GetOwningPlayer()));
			Canvas.AddChildToCanvas(CurrentContainerWidget);
			auto GridBorderSlot = WidgetLayout::SlotAsCanvasSlot(CurrentContainerWidget);
			GridBorderSlot.SetAnchors(FAnchors(1, 0.5));
			GridBorderSlot.Alignment = FVector2D(1, 0.5);
			GridBorderSlot.SetPosition(FVector2D(-100, 0));
			GridBorderSlot.bAutoSize = true;
			CurrentContainerWidget.Initialize(Data.ContainerInventory, 50);
			CurrentContainerWidget.Refresh();
		}
		else
		{
			if (OwnerInventory != nullptr)
			{
				OwnerInventory.ServerResetSearchSession();
			}

			if (CurrentContainerWidget != nullptr)
				CurrentContainerWidget.RemoveFromParent();
			CloseContextMenu();
			CachedCurrentContainerMsg.Reset();
		}
	}

	UFUNCTION()
	// 双击快捷转移：容器->背包 或 背包->当前容器
	void OnReceivedSwapItemMessage(FGameplayTag ActualTag, FSwapGridItemMessage Data)
	{
		CloseContextMenu();

		if (!CachedCurrentContainerMsg.IsSet())
			return;
		auto CurrentInteractContainer = CachedCurrentContainerMsg.GetValue().ContainerInventory;

		if (CurrentInteractContainer != nullptr && Data.ItemInst.GetOuter() == OwnerInventory.GetOwner())
		{
			FIntPoint Tile;
			bool bRotated;
			FGameplayTag TargetRegionId;
			int32 TargetPocketIndex = -1;
			if (CurrentInteractContainer.FindFirstFitPlacement(Data.ItemInst.ItemRegistryId, TargetRegionId, TargetPocketIndex, Tile, bRotated))
			{
				int32 StackCount = OwnerInventory.GetStackCountByItemInstance(Data.ItemInst);
				FYcInventoryOperation Op;
				Op.OpType = n"Inventory.SwapGrid";
				Op.ItemInstance = Data.ItemInst;
				Op.SourceInventory = OwnerInventory;
				Op.TargetInventory = CurrentInteractContainer;
				Op.StackCount = StackCount;
				Op.GridTile = Tile;
				Op.bRotated = bRotated;
				Op.GridRegionId = TargetRegionId;
				Op.GridPocketIndex = TargetPocketIndex;
				OwnerInventory.GetItemPlacementRegion(Data.ItemInst, Op.SourceGridRegionId, Op.SourceGridPocketIndex);
				auto Router = UYcInventoryOperationRouterComponent::FindOrCreateRouter(GetOwningPlayer());
				if (Router != nullptr)
				{
					Router.SubmitInventoryOperation(OwnerInventory, Op, true);
				}
			}
		}
		else if (CurrentInteractContainer != nullptr)
		{
			FIntPoint Tile;
			bool bRotated;
			FGameplayTag TargetRegionId;
			int32 TargetPocketIndex = -1;
			if (OwnerInventory.FindFirstFitPlacement(Data.ItemInst.ItemRegistryId, TargetRegionId, TargetPocketIndex, Tile, bRotated))
			{
				int32 StackCount = CurrentInteractContainer.GetStackCountByItemInstance(Data.ItemInst);
				FYcInventoryOperation Op;
				Op.OpType = n"Inventory.SwapGrid";
				Op.ItemInstance = Data.ItemInst;
				Op.SourceInventory = CurrentInteractContainer;
				Op.TargetInventory = OwnerInventory;
				Op.StackCount = StackCount;
				Op.GridTile = Tile;
				Op.bRotated = bRotated;
				Op.GridRegionId = TargetRegionId;
				Op.GridPocketIndex = TargetPocketIndex;
				CurrentInteractContainer.GetItemPlacementRegion(Data.ItemInst, Op.SourceGridRegionId, Op.SourceGridPocketIndex);
				auto Router = UYcInventoryOperationRouterComponent::FindOrCreateRouter(GetOwningPlayer());
				if (Router != nullptr)
				{
					Router.SubmitInventoryOperation(OwnerInventory, Op, true);
				}
			}
		}
	}

	UFUNCTION(BlueprintOverride)
	FEventReply OnMouseButtonDoubleClick(FGeometry InMyGeometry, FPointerEvent InMouseEvent)
	{
		return FEventReply::Handled();
	}

	UFUNCTION(BlueprintOverride)
	FEventReply OnMouseButtonDown(FGeometry InMyGeometry, FPointerEvent InMouseEvent)
	{
		CloseContextMenu();
		return Widget::Unhandled();
	}

	UFUNCTION()
	void BuildOwnerRegionWidgets()
	{
		for (int32 i = 0; i < OwnerRegionWidgets.Num(); i++)
		{
			if (OwnerRegionWidgets[i] != nullptr && OwnerRegionWidgets[i] != GridInventory)
			{
				OwnerRegionWidgets[i].RemoveFromParent();
			}
		}
		OwnerRegionWidgets.Empty();
		TArray<FGridInventoryRegionRuntimeState> States = OwnerInventory.GetEnabledPocketStates();
		for (int32 i = 0; i < States.Num(); i++)
		{
			for (int32 j = i + 1; j < States.Num(); j++)
			{
				int32 IPriority = States[i].Priority;
				int32 JPriority = States[j].Priority;
				if (JPriority < IPriority)
				{
					auto Tmp = States[i];
					States[i] = States[j];
					States[j] = Tmp;
				}
			}
		}
		if (States.Num() <= 0)
		{
			GridInventory.Initialize(OwnerInventory, TileSize);
			GridInventory.Refresh();
			OwnerRegionWidgets.Add(GridInventory);
			return;
		}

		GridInventory.Initialize(OwnerInventory, TileSize, States[0].RegionId, States[0].PocketIndex);
		GridInventory.Refresh();
		OwnerRegionWidgets.Add(GridInventory);

		auto RootSlot = WidgetLayout::SlotAsCanvasSlot(GridInventory);
		if (RootSlot == nullptr)
		{
			if (Canvas != nullptr && GridInventory.GetParent() == nullptr)
			{
				Canvas.AddChildToCanvas(GridInventory);
				RootSlot = WidgetLayout::SlotAsCanvasSlot(GridInventory);
			}
			if (RootSlot == nullptr)
			{
				Warning("GridInventory root slot is not CanvasSlot.");
				return;
			}
		}
		FAnchors RootAnchors = RootSlot.GetAnchors();
		FVector2D RootAlignment = RootSlot.Alignment;
		FVector2D RootPos = RootSlot.GetPosition();
		FIntPoint BaseLayoutOffset = States[0].LayoutOffset;

		for (int32 i = 1; i < States.Num(); i++)
		{
			auto NewWidget = Cast<UGridInventoryWidget>(WidgetBlueprint::CreateWidget(GridInventory.GetClass(), GetOwningPlayer()));
			if (NewWidget == nullptr)
			{
				continue;
			}

			Canvas.AddChildToCanvas(NewWidget);
			auto Slot = WidgetLayout::SlotAsCanvasSlot(NewWidget);
			Slot.SetAnchors(RootAnchors);
			Slot.Alignment = RootAlignment;
			FIntPoint RelativeOffset = States[i].LayoutOffset - BaseLayoutOffset;
			Slot.SetPosition(RootPos + FVector2D(RelativeOffset.X * TileSize, RelativeOffset.Y * TileSize));
			Slot.bAutoSize = true;

			NewWidget.Initialize(OwnerInventory, TileSize, States[i].RegionId, States[i].PocketIndex);
			NewWidget.Refresh();
			OwnerRegionWidgets.Add(NewWidget);
		}
	}

	UFUNCTION()
	void OnOwnerEquipmentSlotChanged(FGameplayTag ActualTag, FYcEquipmentSlotChangedMessage Data)
	{
		if (OwnerInventory == nullptr)
		{
			return;
		}
		if (!IsOwnerActorMatched(Data.Owner, OwnerInventory.GetOwner()))
		{
			return;
		}
		BuildOwnerRegionWidgets();
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

	UFUNCTION()
	void OnReceivedContextMenuOpenMessage(FGameplayTag ActualTag, FGridItemContextMenuOpenMessage Data)
	{
		if (OwnerInventory == nullptr || Data.ItemInst == nullptr)
		{
			return;
		}

		TArray<FGridItemContextMenuAction> Actions;
		if (!OwnerInventory.GetContextMenuActionsForItem(Data.ItemInst, Actions))
		{
			CloseContextMenu();
			return;
		}
		if (ContextMenuWidgetClass == nullptr)
		{
			Warning("ContextMenuWidgetClass is nullptr.");
			return;
		}

		if (CurrentContextMenuWidget != nullptr)
		{
			CurrentContextMenuWidget.RemoveFromParent();
			CurrentContextMenuWidget = nullptr;
		}

		CurrentContextMenuWidget = Cast<UGridItemContextMenuWidget>(WidgetBlueprint::CreateWidget(ContextMenuWidgetClass, GetOwningPlayer()));
		if (CurrentContextMenuWidget == nullptr)
		{
			return;
		}

		Canvas.AddChildToCanvas(CurrentContextMenuWidget);
		CurrentContextMenuWidget.Initialize(Data.ItemInst, Actions);

		CurrentContextMenuWidget.ForceLayoutPrepass();
		FVector2D LocalPos = Canvas.GetCachedGeometry().AbsoluteToLocal(Data.ScreenPosition);
		FVector2D CanvasSize = Canvas.GetCachedGeometry().GetLocalSize();
		FVector2D MenuSize = CurrentContextMenuWidget.GetDesiredSize();
		if (MenuSize.X <= 1.0f || MenuSize.Y <= 1.0f)
		{
			// 保底尺寸，避免首次打开时DesiredSize尚未完成布局导致越界。
			MenuSize = FVector2D(260.0f, 180.0f);
		}

		float ClampedX = Math::Clamp(LocalPos.X, 0.0f, Math::Max(0.0f, CanvasSize.X - MenuSize.X));
		float ClampedY = Math::Clamp(LocalPos.Y, 0.0f, Math::Max(0.0f, CanvasSize.Y - MenuSize.Y));
		auto MenuSlot = WidgetLayout::SlotAsCanvasSlot(CurrentContextMenuWidget);
		MenuSlot.SetPosition(FVector2D(ClampedX, ClampedY));
		MenuSlot.bAutoSize = true;
	}

	UFUNCTION()
	void OnReceivedContextMenuClickMessage(FGameplayTag ActualTag, FGridItemContextMenuClickMessage Data)
	{
		if (OwnerInventory == nullptr || Data.ItemInst == nullptr || !Data.ActionTag.IsValid())
		{
			return;
		}

		OwnerInventory.ServerRequestExecuteItemContextAction(Data.ItemInst, Data.ActionTag);
		if (Data.bCloseMenuOnExecute)
		{
			CloseContextMenu();
		}
	}

	UFUNCTION()
	void OnReceivedContextMenuCloseMessage(FGameplayTag ActualTag, FGridItemContextMenuCloseMessage Data)
	{
		CloseContextMenu();
	}

	UFUNCTION()
	void CloseContextMenu()
	{
		if (CurrentContextMenuWidget != nullptr)
		{
			CurrentContextMenuWidget.RemoveFromParent();
			CurrentContextMenuWidget = nullptr;
		}
	}
}

USTRUCT()
struct FSwapGridItemMessage
{
	AActor ContainerOwner;
	UGridInventoryManagerComponent ContainerInventory;
	UYcInventoryItemInstance ItemInst;
}

USTRUCT()
struct FGridItemContainerMessage
{
	AActor Player;
	UGridInventoryManagerComponent ContainerInventory;
	TSubclassOf<UGridInventoryWidget> ContainerWidgetClass;
	bool bIsOpen;
}
