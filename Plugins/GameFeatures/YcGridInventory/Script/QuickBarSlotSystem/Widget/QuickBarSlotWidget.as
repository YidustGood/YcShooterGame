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

		RefreshFromSlotState();
	}

	UFUNCTION(BlueprintOverride)
	void Destruct()
	{
		QuickBarSlotsChangedHandle.Unregister();
	}

	UFUNCTION(BlueprintOverride)
	bool OnDrop(FGeometry MyGeometry, FPointerEvent PointerEvent, UDragDropOperation Operation)
	{
		auto DropItem = Cast<UYcInventoryItemInstance>(Operation.Payload);
		if (DropItem == nullptr)
			return true;

		QuickBarComponent.ServerAddItemToSlot(SlotIndex, DropItem);

		return true;
	}

	UFUNCTION(BlueprintOverride)
	FEventReply OnMouseButtonDoubleClick(FGeometry InMyGeometry, FPointerEvent InMouseEvent)
	{
		if (ItemInstance == nullptr)
			return FEventReply::Unhandled();

		QuickBarComponent.ServerRemoveItemFromSlot(SlotIndex);
		return FEventReply::Handled();
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
