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

		RefreshFromSlotState();
	}

	UFUNCTION(BlueprintOverride)
	void Destruct()
	{
		EquipmentSlotChangedHandle.Unregister();
	}

	UFUNCTION(BlueprintOverride)
	bool OnDrop(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation Operation)
	{
		auto DropItem = Cast<UYcInventoryItemInstance>(Operation.Payload);
		if (DropItem == nullptr)
			return true;

		if (DropItem.FindItemFragment(FInventoryFragment_Equippable).Get(FInventoryFragment_Equippable).EquipmentDef.EquipmentSlot == EquipmentSlotTag)
		{
			EquipmentSlotComponent.ServerEquipItem(DropItem);
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

		EquipmentSlotComponent.ServerUnequipSlot(EquipmentSlotTag);
		return FEventReply::Handled();
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
