// 网格中单个物品的可视化与交互入口（双击/拖拽）
class UGridItemWidget : UUserWidget
{
	UPROPERTY(BindWidget)
	USizeBox SB_Background;

	UPROPERTY(BindWidget)
	UBorder BackgroundBorder;

	UPROPERTY(BindWidget)
	UImage ItemImage;

	// 未解锁时使用的占位图标（可选）
	UPROPERTY(Category = "Search")
	UTexture2D UnknownIconTexture;

	float TileSize;
	FVector2D Size;

	// 当前UI所关联的ItemInst
	UPROPERTY()
	UYcInventoryItemInstance ItemInstance;

	UPROPERTY()
	UDragDropOperation DragDropOperation;

	UFUNCTION()
	void Initialize(UYcInventoryItemInstance InItemInstance, float InTileSize)
	{
		ItemInstance = InItemInstance;
		TileSize = InTileSize;
		Refresh();
	}

	UFUNCTION()
	UGridInventoryManagerComponent GetOwningPlayerGridInventory() const
	{
		return Cast<UGridInventoryManagerComponent>(YcInventory::GetInventoryManagerComponent(GetOwningPlayer()));
	}

	UFUNCTION(BlueprintPure)
	bool IsItemRevealed() const
	{
		auto PlayerInventory = GetOwningPlayerGridInventory();
		if (PlayerInventory == nullptr)
		{
			return true;
		}
		return PlayerInventory.IsItemRevealedForCurrentSession(ItemInstance);
	}

	// 刷新当前物品在UI中的显示位置和状态
	UFUNCTION()
	void Refresh()
	{
		if (ItemInstance == nullptr)
		{
			return;
		}

		FItemFragment_GridItem IF_Grid = ItemInstance.FindItemFragment(FItemFragment_GridItem).Get(FItemFragment_GridItem);
		Size.X = IF_Grid.Dimensions.X * TileSize;
		Size.Y = IF_Grid.Dimensions.Y * TileSize;
		SB_Background.SetWidthOverride(Size.X);
		SB_Background.SetHeightOverride(Size.Y);
		auto ItemImageSlot = WidgetLayout::SlotAsCanvasSlot(ItemImage);
		ItemImageSlot.SetSize(Size);

		if (!IsItemRevealed())
		{
			if (UnknownIconTexture != nullptr)
			{
				ItemImage.SetBrushFromTexture(UnknownIconTexture);
			}
			else
			{
				ItemImage.SetColorAndOpacity(FLinearColor(0.07f, 0.07f, 0.07f, 1.0f));
			}
			return;
		}

		auto DynamicMaterial = Material::CreateDynamicMaterialInstance(IF_Grid.Icon);
		DynamicMaterial.SetTextureParameterValue(n"ItemIcon", IF_Grid.IconTexture);
		ItemImage.SetBrushFromMaterial(DynamicMaterial);
		ItemImage.SetColorAndOpacity(FLinearColor::White);
	}

	// 将当前UI所关联的Item从其库存管理器中移除
	UFUNCTION()
	bool RemoveGridItem()
	{
		auto Inventory = Cast<UGridInventoryManagerComponent>(ItemInstance.GetActorOuter().GetComponentByClass(UGridInventoryManagerComponent));
		return Inventory.RemoveGridItem(ItemInstance);
	}

	UFUNCTION(BlueprintOverride)
	void OnMouseEnter(FGeometry MyGeometry, FPointerEvent MouseEvent)
	{
		BackgroundBorder.SetBrushColor(FLinearColor(0.05, 0.05, 0.05, 1));
	}

	UFUNCTION(BlueprintOverride)
	void OnMouseLeave(FPointerEvent MouseEvent)
	{
		BackgroundBorder.SetBrushColor(FLinearColor(0, 0, 0, 1));
	}

	UFUNCTION(BlueprintOverride)
	FEventReply OnMouseButtonDown(FGeometry InMyGeometry, FPointerEvent InMouseEvent)
	{
		// 未识别状态下直接禁止拖拽起手（起手即拦截，避免无效拖拽反馈）
		if (!IsItemRevealed())
		{
			return Widget::Handled();
		}
		return Widget::Unhandled();
	}

	UFUNCTION(BlueprintOverride)
	FEventReply OnMouseButtonDoubleClick(FGeometry InMyGeometry, FPointerEvent InMouseEvent)
	{
		if (!IsItemRevealed())
		{
			return Widget::Handled();
		}

		FSwapGridItemMessage Msg;
		Msg.ContainerInventory = Cast<UGridInventoryManagerComponent>(ItemInstance.GetActorOuter().GetComponentByClass(UGridInventoryManagerComponent));
		Msg.ItemInst = ItemInstance;
		Msg.ContainerOwner = ItemInstance.GetActorOuter();
		UGameplayMessageSubsystem::Get().BroadcastMessage(GameplayTags::Yc_Inventory_Message_Grid_Container_SwapItem, Msg);
		return Widget::Handled();
	}

	UFUNCTION(BlueprintOverride)
	void OnDragDetected(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation& Operation)
	{
		if (!IsItemRevealed())
		{
			return;
		}
		CreateDragDropOperation();
		Operation = DragDropOperation;
	}

	// 创建拖拽操作, AS不支持创建拖拽操作, 在UMG中实现这个创建函数
	UFUNCTION(BlueprintEvent)
	void CreateDragDropOperation()
	{
		// 创建拖拽操作
	}
}


