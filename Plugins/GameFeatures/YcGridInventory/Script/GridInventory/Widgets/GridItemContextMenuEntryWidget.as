class UGridItemContextMenuEntryWidget : UUserWidget
{
	UPROPERTY(BindWidget)
	UButton ActionButton;

	UPROPERTY(BindWidget)
	UTextBlock ActionText;

	UPROPERTY(BindWidget)
	UImage ActionIcon;

	UYcInventoryItemInstance ItemInstance;

	FGridItemContextMenuAction ActionDef;

	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		if (ActionButton != nullptr)
		{
			ActionButton.OnClicked.AddUFunction(this, n"OnActionClicked");
		}
	}

	UFUNCTION()
	void Initialize(UYcInventoryItemInstance InItemInstance, FGridItemContextMenuAction InActionDef)
	{
		ItemInstance = InItemInstance;
		ActionDef = InActionDef;

		if (ActionText != nullptr)
		{
			ActionText.SetText(ActionDef.DisplayName);
		}

		if (ActionIcon != nullptr)
		{
			ActionIcon.SetBrushFromTexture(ActionDef.Icon);
		}
	}

	UFUNCTION()
	void OnActionClicked()
	{
		if (ItemInstance == nullptr || !ActionDef.ActionTag.IsValid())
		{
			return;
		}

		FGridItemContextMenuClickMessage ClickMsg;
		ClickMsg.ItemInst = ItemInstance;
		ClickMsg.ActionTag = ActionDef.ActionTag;
		ClickMsg.bCloseMenuOnExecute = ActionDef.bCloseMenuOnExecute;
		FGameplayTag ClickTag = FGameplayTag::RequestGameplayTag(n"Yc.Inventory.Message.Grid.ContextMenu.Click");
		UGameplayMessageSubsystem::Get().BroadcastMessage(ClickTag, ClickMsg);
	}
}
