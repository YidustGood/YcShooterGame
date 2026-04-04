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
			FText FinalText = ActionDef.DisplayName;
			if (!ActionDef.bRuntimeCanExecute)
			{
				FString DisabledReason = ActionDef.RuntimeDisabledReason.ToString();
				if (DisabledReason.Len() > 0)
				{
					FinalText = FText::FromString(f"{ActionDef.DisplayName.ToString()} ({DisabledReason})");
				}

				ActionText.SetColorAndOpacity(FLinearColor(1.00, 0.50, 0.06, 1.0f));
			}
			else
			{
				ActionText.SetColorAndOpacity(FLinearColor::White);
			}
			ActionText.SetText(FinalText);
		}

		if (ActionIcon != nullptr)
		{
			ActionIcon.SetBrushFromTexture(ActionDef.Icon);
			if (!ActionDef.bRuntimeCanExecute)
			{
				ActionIcon.SetColorAndOpacity(FLinearColor(0.65f, 0.65f, 0.65f, 1.0f));
			}
			else
			{
				ActionIcon.SetColorAndOpacity(FLinearColor::White);
			}
		}

		if (ActionButton != nullptr)
		{
			ActionButton.SetIsEnabled(ActionDef.bRuntimeCanExecute);
		}
	}

	UFUNCTION()
	void OnActionClicked()
	{
		if (ItemInstance == nullptr || !ActionDef.ActionTag.IsValid() || !ActionDef.bRuntimeCanExecute)
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
