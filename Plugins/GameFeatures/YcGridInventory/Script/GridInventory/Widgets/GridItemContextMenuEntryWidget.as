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

				ActionText.SetColorAndOpacity(FLinearColor(1.00, 1, 1, 1.0f));
			}
			else
			{
				ActionText.SetColorAndOpacity(FLinearColor::White);
			}
			ActionText.SetText(FinalText);
		}

		if (ActionIcon != nullptr)
		{
			if (ActionDef.Icon != nullptr)
			{
				ActionIcon.SetVisibility(ESlateVisibility::Visible);
				ActionIcon.SetBrushFromTexture(ActionDef.Icon);
				if (!ActionDef.bRuntimeCanExecute)
				{
					ActionIcon.SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f));
				}
				else
				{
					ActionIcon.SetColorAndOpacity(FLinearColor::White);
				}
			}
			else
			{
				ActionIcon.SetVisibility(ESlateVisibility::Collapsed);
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
