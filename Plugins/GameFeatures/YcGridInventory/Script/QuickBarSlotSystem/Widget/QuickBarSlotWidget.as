class UQuickBarSlotWidget : UUserWidget
{
	UPROPERTY()
	FGameplayTag SlotTag;

	UPROPERTY()
	int32 SlotIndex = 0;

	UPROPERTY(BindWidget)
	UImage ItemImage;

	UPROPERTY(BindWidget)
	UTextBlock ItemNameText;

	UPROPERTY()
	FText DefaultItemName = FText::FromString("快捷栏");

	UPROPERTY()
	UDragDropOperation DragDropOperation;

	UPROPERTY(NotVisible)
	UYcInventoryItemInstance ItemInstance;

	UYcQuickBarComponent QuickBarComponent;
	FGameplayMessageListenerHandle QuickBarSlotsChangedHandle;
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

		QuickBarComponent = UYcQuickBarComponent::FindQuickBarComponent(GetOwningPlayer());
		if (QuickBarComponent == nullptr)
		{
			Warning("UQuickBarSlotWidget::Construct - QuickBarComponent is nullptr, will retry lazily.");
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
	void OnDragDetected(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation& Operation)
	{
		if (ItemInstance == nullptr)
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

	UFUNCTION(BlueprintOverride)
	bool OnDrop(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation Operation)
	{
		auto DropItem = Cast<UYcInventoryItemInstance>(Operation.Payload);
		if (DropItem == nullptr)
			return true;

		auto EquippableFragment = DropItem.FindItemFragment(FInventoryFragment_Equippable);
		if (!EquippableFragment.IsValid())
		{
			return true;
		}

		auto QuickBarSlotFragment = YcEquipment::FindEquipmentFragment(
			EquippableFragment.Get(FInventoryFragment_Equippable).EquipmentDef,
			FEquipmentFragment_QuickBarSlot);
		if (!QuickBarSlotFragment.IsValid())
		{
			return true;
		}

		if (QuickBarSlotFragment.Get(FEquipmentFragment_QuickBarSlot).SlotIndex != SlotIndex)
		{
			return true;
		}

		auto InventoryManager = Cast<UYcInventoryManagerComponent>(YcInventory::GetInventoryManagerComponent(GetOwningPlayer()));
		if (InventoryManager != nullptr)
		{
			auto SourceInventory = Cast<UYcInventoryManagerComponent>(DropItem.GetActorOuter().GetComponentByClass(UYcInventoryManagerComponent));
			if (SourceInventory == nullptr)
			{
				SourceInventory = InventoryManager;
			}

			// QuickBar 引用模式仅允许绑定本人背包物品；容器物品需先转入背包再绑定。
			if (SourceInventory != InventoryManager)
			{
				if (QuickBarComponent == nullptr)
				{
					QuickBarComponent = UYcQuickBarComponent::FindQuickBarComponent(GetOwningPlayer());
				}
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
				if (QuickBarComponent == nullptr)
				{
					QuickBarComponent = UYcQuickBarComponent::FindQuickBarComponent(GetOwningPlayer());
				}
				if (QuickBarComponent != nullptr)
				{
					QuickBarComponent.ServerAddItemToSlot(SlotIndex, DropItem);
				}
			}
		}
		else
		{
			if (QuickBarComponent == nullptr)
			{
				QuickBarComponent = UYcQuickBarComponent::FindQuickBarComponent(GetOwningPlayer());
			}
			if (QuickBarComponent != nullptr)
			{
				QuickBarComponent.ServerAddItemToSlot(SlotIndex, DropItem);
			}
		}

		return true;
	}

	UFUNCTION(BlueprintOverride)
	FEventReply OnMouseButtonDoubleClick(FGeometry InMyGeometry, FPointerEvent InMouseEvent)
	{
		if (ItemInstance == nullptr)
			return FEventReply::Unhandled();

		// 发送槽位移除消息
		FYcQuickBarSlotRemovedMessage RemovedMessage;
		RemovedMessage.Owner = GetOwningPlayer();
		RemovedMessage.SlotIndex = SlotIndex;
		UGameplayMessageSubsystem::Get().BroadcastMessage(GameplayTags::Yc_QuickBar_Message_SlotRemoved, RemovedMessage);

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
				if (QuickBarComponent == nullptr)
				{
					QuickBarComponent = UYcQuickBarComponent::FindQuickBarComponent(GetOwningPlayer());
				}
				if (QuickBarComponent != nullptr)
				{
					QuickBarComponent.ServerRemoveItemFromSlot(SlotIndex);
				}
			}
		}
		else
		{
			if (QuickBarComponent == nullptr)
			{
				QuickBarComponent = UYcQuickBarComponent::FindQuickBarComponent(GetOwningPlayer());
			}
			if (QuickBarComponent != nullptr)
			{
				QuickBarComponent.ServerRemoveItemFromSlot(SlotIndex);
			}
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
			if (Data.Event == EYcInventoryOperationEvent::Acked || Data.Event == EYcInventoryOperationEvent::Nacked)
			{
				RefreshFromSlotState();
				return;
			}

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
		if (QuickBarComponent == nullptr)
		{
			QuickBarComponent = UYcQuickBarComponent::FindQuickBarComponent(GetOwningPlayer());
		}
		RefreshFromSlotState();
	}

	UFUNCTION()
	void RefreshFromSlotState()
	{
		if (QuickBarComponent == nullptr)
		{
			QuickBarComponent = UYcQuickBarComponent::FindQuickBarComponent(GetOwningPlayer());
			if (QuickBarComponent == nullptr)
			{
				ApplyItemVisual(nullptr);
				return;
			}
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
			ItemImage.SetVisibility(ESlateVisibility::Hidden);
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
			ItemImage.SetBrushFromMaterial(nullptr);
			ItemImage.SetVisibility(ESlateVisibility::Hidden);
			return;
		}

		auto DynamicMaterial = Material::CreateDynamicMaterialInstance(GridFragment.Icon);
		DynamicMaterial.SetTextureParameterValue(n"ItemIcon", GridFragment.IconTexture);
		ItemImage.SetBrushFromMaterial(DynamicMaterial);
		ItemImage.SetVisibility(ESlateVisibility::Visible);
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
