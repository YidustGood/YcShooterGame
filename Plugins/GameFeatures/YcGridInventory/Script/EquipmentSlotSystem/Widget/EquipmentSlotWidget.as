USTRUCT()
struct FYcEquipmentSlotHoverTooltipData
{
	UPROPERTY()
	UYcInventoryItemInstance ItemInstance;

	UPROPERTY()
	FGameplayTag EquipmentSlotTag;

	UPROPERTY()
	FText SlotDisplayName;

	UPROPERTY()
	bool bHasItem = false;
}

class UEquipmentSlotWidget : UYcHoverTooltipProviderWidget
{
	UPROPERTY()
	FGameplayTag EquipmentSlotTag;

	UPROPERTY(BindWidget)
	UImage EquipmentItemImage;

	UPROPERTY(BindWidget)
	UTextBlock ItemNameText;

	UPROPERTY()
	FText DefaultItemName = FText::FromString("装备栏");

	UPROPERTY()
	UDragDropOperation DragDropOperation;

	UPROPERTY(NotVisible)
	UYcInventoryItemInstance ItemInstance;

	UYcEquipmentSlotComponent EquipmentSlotComponent;
	FGameplayMessageListenerHandle EquipmentSlotChangedHandle;
	FGameplayMessageListenerHandle InventoryOperationStateHandle;

	UFUNCTION(BlueprintOverride)
	void PreConstruct(bool IsDesignTime)
	{
		ApplyItemName(DefaultItemName);
	}

	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		ApplyItemVisual(nullptr);

		EquipmentSlotComponent = UYcEquipmentSlotComponent::FindEquipmentSlotComponent(GetOwningPlayer());
		if (EquipmentSlotComponent == nullptr)
		{
			Warning("UEquipmentSlotWidget::Construct - EquipmentSlotComponent is nullptr, will retry lazily.");
		}

		EquipmentSlotChangedHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			GameplayTags::Yc_EquipmentSlot_Message_SlotChanged,
			this,
			n"OnEquipmentSlotChanged",
			FYcEquipmentSlotChangedMessage(),
			EGameplayMessageMatch::ExactMatch);

		InventoryOperationStateHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			FGameplayTag::RequestGameplayTag(n"Yc.Inventory.Message.ProjectedStateChanged"),
			this,
			n"OnProjectedStateChanged",
			FYcInventoryProjectedStateChangedMessage(),
			EGameplayMessageMatch::ExactMatch);

		RefreshFromSlotState();
	}

	UFUNCTION(BlueprintOverride)
	void Destruct()
	{
		EquipmentSlotChangedHandle.Unregister();
		InventoryOperationStateHandle.Unregister();
		YcHoverTooltip::CancelHoverTooltipForWidget(this);
	}

	UFUNCTION(BlueprintOverride)
	void OnMouseEnter(FGeometry MyGeometry, FPointerEvent MouseEvent)
	{
		YcHoverTooltip::NotifyHoverEnter(this, MouseEvent.GetScreenSpacePosition());
	}

	UFUNCTION(BlueprintOverride)
	void OnMouseLeave(FPointerEvent MouseEvent)
	{
		YcHoverTooltip::NotifyHoverLeave(this);
	}

	UFUNCTION(BlueprintOverride)
	FEventReply OnMouseMove(FGeometry MyGeometry, FPointerEvent MouseEvent)
	{
		YcHoverTooltip::NotifyHoverMove(this, MouseEvent.GetScreenSpacePosition());
		return Widget::Unhandled();
	}

	UFUNCTION(BlueprintOverride)
	void OnDragDetected(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation& Operation)
	{
		if (ItemInstance == nullptr)
		{
			return;
		}

		YcHoverTooltip::CancelHoverTooltipForWidget(this);
		CreateDragDropOperation();
		Operation = DragDropOperation;
	}

	// 创建拖拽操作, AS不支持创建拖拽操作, 在UMG中实现这个创建函数
	UFUNCTION(BlueprintEvent)
	void CreateDragDropOperation()
	{
		// 创建拖拽操作
	}

	UFUNCTION(BlueprintOverride)
	bool OnDrop(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation Operation)
	{
		YcHoverTooltip::CancelHoverTooltipForWidget(this);
		auto DropItem = Cast<UYcInventoryItemInstance>(Operation.Payload);
		if (DropItem == nullptr)
			return true;

		auto EquippableFragment = DropItem.FindItemFragment(FInventoryFragment_Equippable);
		if (EquippableFragment.IsValid() && EquippableFragment.Get(FInventoryFragment_Equippable).EquipmentDef.EquipmentSlot == EquipmentSlotTag)
		{
			auto InventoryManager = Cast<UYcInventoryManagerComponent>(YcInventory::GetInventoryManagerComponent(GetOwningPlayer()));
			if (InventoryManager != nullptr)
			{
				auto SourceInventory = Cast<UYcInventoryManagerComponent>(DropItem.GetActorOuter().GetComponentByClass(UYcInventoryManagerComponent));
				if (SourceInventory == nullptr)
				{
					SourceInventory = InventoryManager;
				}

				FYcInventoryOperation Op;
				Op.OpType = n"Equipment.Equip";
				Op.ItemInstance = DropItem;
				Op.SlotTag = EquipmentSlotTag;
				Op.SourceInventory = SourceInventory;
				Op.TargetInventory = InventoryManager;
				auto Router = UYcInventoryOperationRouterComponent::FindOrCreateRouter(GetOwningPlayer());
				if (Router != nullptr)
				{
					Router.SubmitInventoryOperation(InventoryManager, Op, true);
				}
				else
				{
					if (EquipmentSlotComponent == nullptr)
					{
						EquipmentSlotComponent = UYcEquipmentSlotComponent::FindEquipmentSlotComponent(GetOwningPlayer());
					}
					if (EquipmentSlotComponent != nullptr)
					{
						EquipmentSlotComponent.ServerEquipItem(DropItem);
					}
				}

				ApplyItemVisual(DropItem);
			}
			else
			{
				if (EquipmentSlotComponent == nullptr)
				{
					EquipmentSlotComponent = UYcEquipmentSlotComponent::FindEquipmentSlotComponent(GetOwningPlayer());
				}
				if (EquipmentSlotComponent != nullptr)
				{
					EquipmentSlotComponent.ServerEquipItem(DropItem);
				}
			}
		}
		else
		{
			Print("Can't EquipItem: " + DropItem.ItemRegistryId.ItemName);
		}

		return true;
	}

	UFUNCTION(BlueprintOverride)
	FEventReply OnMouseButtonDoubleClick(FGeometry InMyGeometry, FPointerEvent InMouseEvent)
	{
		YcHoverTooltip::CancelHoverTooltipForWidget(this);
		if (ItemInstance == nullptr)
			return FEventReply::Unhandled();

		auto InventoryManager = Cast<UYcInventoryManagerComponent>(YcInventory::GetInventoryManagerComponent(GetOwningPlayer()));
		if (InventoryManager != nullptr)
		{
			FYcInventoryOperation Op;
			Op.OpType = n"Equipment.Unequip";
			Op.ItemInstance = ItemInstance;
			Op.SlotTag = EquipmentSlotTag;
			Op.SourceInventory = InventoryManager;
			Op.TargetInventory = InventoryManager;
			auto Router = UYcInventoryOperationRouterComponent::FindOrCreateRouter(GetOwningPlayer());
			if (Router != nullptr)
			{
				Router.SubmitInventoryOperation(InventoryManager, Op, true);
			}
			else
			{
				if (EquipmentSlotComponent == nullptr)
				{
					EquipmentSlotComponent = UYcEquipmentSlotComponent::FindEquipmentSlotComponent(GetOwningPlayer());
				}
				if (EquipmentSlotComponent != nullptr)
				{
					EquipmentSlotComponent.ServerUnequipSlot(EquipmentSlotTag);
				}
			}
		}
		else
		{
			if (EquipmentSlotComponent == nullptr)
			{
				EquipmentSlotComponent = UYcEquipmentSlotComponent::FindEquipmentSlotComponent(GetOwningPlayer());
			}
			if (EquipmentSlotComponent != nullptr)
			{
				EquipmentSlotComponent.ServerUnequipSlot(EquipmentSlotTag);
			}
		}
		return FEventReply::Handled();
	}

	UFUNCTION(BlueprintOverride)
	bool CanShowHoverTooltip() const
	{
		return HoverTooltipWidgetClass != nullptr && (ItemInstance != nullptr || !DefaultItemName.IsEmpty());
	}

	UFUNCTION(BlueprintOverride)
	FInstancedStruct BuildHoverTooltipPayload() const
	{
		FYcEquipmentSlotHoverTooltipData Data;
		Data.ItemInstance = ItemInstance;
		Data.EquipmentSlotTag = EquipmentSlotTag;
		Data.SlotDisplayName = DefaultItemName;
		Data.bHasItem = ItemInstance != nullptr;
		return FInstancedStruct::Make(Data);
	}

	UFUNCTION()
	void OnProjectedStateChanged(FGameplayTag ActualTag, FYcInventoryProjectedStateChangedMessage Data)
	{
		auto LocalInventoryManager = Cast<UYcInventoryManagerComponent>(YcInventory::GetInventoryManagerComponent(GetOwningPlayer()));
		if (LocalInventoryManager == nullptr)
		{
			return;
		}
		if (Data.Operation.SourceInventory != LocalInventoryManager && Data.Operation.TargetInventory != LocalInventoryManager)
		{
			return;
		}

		if (Data.Operation.OpType == n"Equipment.Equip" || Data.Operation.OpType == n"Equipment.Unequip")
		{
			if (Data.Operation.SlotTag == EquipmentSlotTag)
			{
				TObjectPtr<UYcInventoryItemInstance> ProjectedItem = nullptr;
				if (Data.ProjectedState.EquipmentSlots.Find(EquipmentSlotTag, ProjectedItem))
				{
					ApplyItemVisual(ProjectedItem);
				}
				else
				{
					RefreshFromSlotState();
				}
			}
		}
	}

	UFUNCTION()
	void OnEquipmentSlotChanged(FGameplayTag ActualTag, FYcEquipmentSlotChangedMessage Data)
	{
		if (Data.SlotTag != EquipmentSlotTag)
		{
			return;
		}

		if (EquipmentSlotComponent == nullptr)
		{
			EquipmentSlotComponent = UYcEquipmentSlotComponent::FindEquipmentSlotComponent(GetOwningPlayer());
		}
		RefreshFromSlotState();
	}

	UFUNCTION()
	void RefreshFromSlotState()
	{
		if (EquipmentSlotComponent == nullptr)
		{
			EquipmentSlotComponent = UYcEquipmentSlotComponent::FindEquipmentSlotComponent(GetOwningPlayer());
			if (EquipmentSlotComponent == nullptr)
			{
				ApplyItemVisual(nullptr);
				return;
			}
		}

		auto NewItem = EquipmentSlotComponent.GetItemInSlot(EquipmentSlotTag);
		if (ItemInstance == NewItem)
		{
			return;
		}

		ItemInstance = NewItem;
		ApplyItemVisual(ItemInstance);
	}

	void ApplyItemVisual(UYcInventoryItemInstance InItem)
	{
		ItemInstance = InItem;
		if (ItemInstance == nullptr)
		{
			EquipmentItemImage.SetBrushFromMaterial(nullptr);
			EquipmentItemImage.SetVisibility(ESlateVisibility::Hidden);
			ApplyItemName(DefaultItemName);
			return;
		}

		FYcInventoryItemDefinition ItemDef;
		if (ItemInstance.GetItemDef(ItemDef))
		{
			ApplyItemName(ItemDef.DisplayName);
		}
		else
		{
			ApplyItemName(DefaultItemName);
		}

		auto GridFragment = GetItemFragmentGrid();
		if (GridFragment.Icon == nullptr)
		{
			EquipmentItemImage.SetBrushFromMaterial(nullptr);
			EquipmentItemImage.SetVisibility(ESlateVisibility::Hidden);
			return;
		}

		auto DynamicMaterial = Material::CreateDynamicMaterialInstance(GridFragment.Icon);
		DynamicMaterial.SetTextureParameterValue(n"ItemIcon", GridFragment.IconTexture);
		EquipmentItemImage.SetBrushFromMaterial(DynamicMaterial);
		EquipmentItemImage.SetVisibility(ESlateVisibility::Visible);
	}

	void ApplyItemName(FText InText)
	{
		if (ItemNameText == nullptr)
		{
			return;
		}

		ItemNameText.SetText(InText);
	}

	FInventoryFragment_Equippable GetEquippableFragment()
	{
		auto InstancedStruct = ItemInstance.FindItemFragment(FInventoryFragment_Equippable);
		auto EquippableFragment = InstancedStruct.Get(FInventoryFragment_Equippable);
		return EquippableFragment;
	}

	FItemFragment_GridItem GetItemFragmentGrid()
	{
		FInstancedStruct Result = ItemInstance.FindItemFragment(FItemFragment_GridItem);
		FItemFragment_GridItem GridItemFragment = Result.Get(FItemFragment_GridItem);
		return GridItemFragment;
	}
}
