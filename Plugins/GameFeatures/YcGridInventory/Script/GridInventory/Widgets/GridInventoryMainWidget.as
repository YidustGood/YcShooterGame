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

	// 当前的物品容器界面
	private UGridInventoryWidget CurrentContainerWidget;
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

		GridInventory.Initialize(OwnerInventory, TileSize);
		GridInventory.Refresh();

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

		if (CurrentContainerWidget != nullptr)
			CurrentContainerWidget.RemoveFromParent();
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
			if (CurrentInteractContainer.FindFirstFitPosition(Data.ItemInst.ItemRegistryId, Tile, bRotated))
			{
				int32 StackCount = OwnerInventory.GetStackCountByItemInstance(Data.ItemInst);
				OwnerInventory.ServerSwapGridItem(CurrentInteractContainer, Data.ItemInst, StackCount, Tile);
			}
		}
		else if (CurrentInteractContainer != nullptr)
		{
			FIntPoint Tile;
			bool bRotated;
			if (OwnerInventory.FindFirstFitPosition(Data.ItemInst.ItemRegistryId, Tile, bRotated))
			{
				int32 StackCount = CurrentInteractContainer.GetStackCountByItemInstance(Data.ItemInst);
				OwnerInventory.ServerSwapGridItem(OwnerInventory, Data.ItemInst, StackCount, Tile);
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



