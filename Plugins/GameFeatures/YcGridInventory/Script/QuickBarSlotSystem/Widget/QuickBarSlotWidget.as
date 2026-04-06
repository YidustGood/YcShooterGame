class UQuickBarSlotWidget : UUserWidget
{
	UPROPERTY()
	FGameplayTag SlotTag;

	UPROPERTY()
	int32 SlotIndex = 0;

	UPROPERTY(BindWidget)
	UImage ItemImage;

	UPROPERTY()
	UYcInventoryItemInstance ItemInstance;

	UYcQuickBarComponent QuickBarComponent;
	FGameplayMessageListenerHandle QuickBarSlotsChangedHandle;
	FGameplayMessageListenerHandle InventoryOperationStateHandle;

	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		QuickBarComponent = UYcQuickBarComponent::FindQuickBarComponent(GetOwningPlayer());
		if (QuickBarComponent == nullptr)
		{
			Error("UQuickBarSlotWidget::Construct - QuickBarComponent is nullptr");
			return;
		}

		QuickBarSlotsChangedHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			GameplayTags::Yc_QuickBar_Message_SlotsChanged,
			this,
			n"OnQuickBarSlotsChanged",
			FYcQuickBarSlotsChangedMessage(),
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
		QuickBarSlotsChangedHandle.Unregister();
		InventoryOperationStateHandle.Unregister();
	}

	UFUNCTION(BlueprintOverride)
	bool OnDrop(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation Operation)
	{
		auto DropItem = Cast<UYcInventoryItemInstance>(Operation.Payload);
		if (DropItem == nullptr)
			return true;

		auto InventoryManager = Cast<UYcInventoryManagerComponent>(YcInventory::GetInventoryManagerComponent(GetOwningPlayer()));
		if (InventoryManager != nullptr)
		{
			auto SourceInventory = Cast<UYcInventoryManagerComponent>(DropItem.GetActorOuter().GetComponentByClass(UYcInventoryManagerComponent));
			if (SourceInventory == nullptr)
			{
				SourceInventory = InventoryManager;
			}

			// QuickBar 引用模式只允许绑定本人背包物品；容器物品需先转入背包再绑定。
			if (SourceInventory != InventoryManager)
			{
				if (QuickBarComponent == nullptr || !QuickBarComponent.IsDirectContainerDropEnabled())
				{
					return true;
				}
			}

			FYcInventoryOperation Op;
			Op.OpType = n"QuickBar.Add";
			Op.ItemInstance = DropItem;
			Op.SlotIndex = SlotIndex;
			Op.SourceInventory = SourceInventory;
			Op.TargetInventory = InventoryManager;
			auto Router = UYcInventoryOperationRouterComponent::FindOrCreateRouter(GetOwningPlayer());
			if (Router != nullptr)
			{
				Router.SubmitInventoryOperation(InventoryManager, Op, true);
			}
			else
			{
				QuickBarComponent.ServerAddItemToSlot(SlotIndex, DropItem);
			}

			ApplyItemVisual(DropItem); // 立即反馈预测视觉
		}
		else
		{
			QuickBarComponent.ServerAddItemToSlot(SlotIndex, DropItem);
		}

		return true;
	}

	UFUNCTION(BlueprintOverride)
	FEventReply OnMouseButtonDoubleClick(FGeometry InMyGeometry, FPointerEvent InMouseEvent)
	{
		if (ItemInstance == nullptr)
			return FEventReply::Unhandled();

		auto InventoryManager = Cast<UYcInventoryManagerComponent>(YcInventory::GetInventoryManagerComponent(GetOwningPlayer()));
		if (InventoryManager != nullptr)
		{
			FYcInventoryOperation Op;
			Op.OpType = n"QuickBar.Remove";
			Op.ItemInstance = ItemInstance;
			Op.SlotIndex = SlotIndex;
			Op.SourceInventory = InventoryManager;
			Op.TargetInventory = InventoryManager;
			auto Router = UYcInventoryOperationRouterComponent::FindOrCreateRouter(GetOwningPlayer());
			if (Router != nullptr)
			{
				Router.SubmitInventoryOperation(InventoryManager, Op, true);
			}
			else
			{
				QuickBarComponent.ServerRemoveItemFromSlot(SlotIndex);
			}
		}
		else
		{
			QuickBarComponent.ServerRemoveItemFromSlot(SlotIndex);
		}
		return FEventReply::Handled();
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

		if ((Data.Operation.OpType == n"QuickBar.Add" || Data.Operation.OpType == n"QuickBar.Remove") && Data.Operation.SlotIndex == SlotIndex)
		{
			TObjectPtr<UYcInventoryItemInstance> ProjectedItem = nullptr;
			if (Data.ProjectedState.QuickBarSlots.Find(SlotIndex, ProjectedItem))
			{
				ApplyItemVisual(ProjectedItem);
			}
			else
			{
				RefreshFromSlotState();
			}
		}
	}

	UFUNCTION()
	void OnQuickBarSlotsChanged(FGameplayTag ActualTag, FYcQuickBarSlotsChangedMessage Data)
	{
		if (QuickBarComponent == nullptr || Data.Owner != QuickBarComponent.GetOwner())
		{
			return;
		}
		RefreshFromSlotState();
	}

	UFUNCTION()
	void RefreshFromSlotState()
	{
		if (QuickBarComponent == nullptr)
		{
			return;
		}

		auto NewItem = QuickBarComponent.GetSlotItem(SlotIndex);
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
			ItemImage.SetBrushFromMaterial(nullptr);
			return;
		}

		auto GridFragment = GetItemFragmentGrid();
		if (GridFragment.Icon == nullptr)
		{
			ItemImage.SetBrushFromMaterial(nullptr);
			return;
		}

		auto DynamicMaterial = Material::CreateDynamicMaterialInstance(GridFragment.Icon);
		DynamicMaterial.SetTextureParameterValue(n"ItemIcon", GridFragment.IconTexture);
		ItemImage.SetBrushFromMaterial(DynamicMaterial);
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
