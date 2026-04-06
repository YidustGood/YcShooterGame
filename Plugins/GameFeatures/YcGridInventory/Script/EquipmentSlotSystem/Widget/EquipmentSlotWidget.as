class UEquipmentSlotWidget : UUserWidget
{
	UPROPERTY()
	FGameplayTag EquipmentSlotTag;

	UPROPERTY(BindWidget)
	UImage EquipmentItemImage;

	UPROPERTY()
	UYcInventoryItemInstance ItemInstance;

	UYcEquipmentSlotComponent EquipmentSlotComponent;
	FGameplayMessageListenerHandle EquipmentSlotChangedHandle;
	FGameplayMessageListenerHandle InventoryOperationStateHandle;

	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		EquipmentSlotComponent = UYcEquipmentSlotComponent::FindEquipmentSlotComponent(GetOwningPlayer());
		if (EquipmentSlotComponent == nullptr)
		{
			Error("UEquipmentSlotWidget::Construct - EquipmentSlotComponent is nullptr");
			return;
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
	}

	UFUNCTION(BlueprintOverride)
	bool OnDrop(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation Operation)
	{
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
					EquipmentSlotComponent.ServerEquipItem(DropItem);
				}

				ApplyItemVisual(DropItem); // 立即反馈预测视觉
			}
			else
			{
				EquipmentSlotComponent.ServerEquipItem(DropItem);
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
				EquipmentSlotComponent.ServerUnequipSlot(EquipmentSlotTag);
			}
		}
		else
		{
			EquipmentSlotComponent.ServerUnequipSlot(EquipmentSlotTag);
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
		if (EquipmentSlotComponent == nullptr || Data.Owner != EquipmentSlotComponent.GetOwner())
		{
			return;
		}
		if (Data.SlotTag != EquipmentSlotTag)
		{
			return;
		}
		RefreshFromSlotState();
	}

	UFUNCTION()
	void RefreshFromSlotState()
	{
		if (EquipmentSlotComponent == nullptr)
		{
			return;
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
			return;
		}

		auto GridFragment = GetItemFragmentGrid();
		if (GridFragment.Icon == nullptr)
		{
			EquipmentItemImage.SetBrushFromMaterial(nullptr);
			return;
		}

		auto DynamicMaterial = Material::CreateDynamicMaterialInstance(GridFragment.Icon);
		DynamicMaterial.SetTextureParameterValue(n"ItemIcon", GridFragment.IconTexture);
		EquipmentItemImage.SetBrushFromMaterial(DynamicMaterial);
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
