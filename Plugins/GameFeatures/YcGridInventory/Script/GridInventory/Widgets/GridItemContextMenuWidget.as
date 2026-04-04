class UGridItemContextMenuWidget : UUserWidget
{
	UPROPERTY(BindWidget)
	UVerticalBox ActionsBox;

	UPROPERTY()
	TSubclassOf<UGridItemContextMenuEntryWidget> EntryWidgetClass;

	UYcInventoryItemInstance ItemInstance;

	UFUNCTION()
	void Initialize(UYcInventoryItemInstance InItemInstance, TArray<FGridItemContextMenuAction> InActions)
	{
		ItemInstance = InItemInstance;

		if (ActionsBox == nullptr)
		{
			return;
		}

		ActionsBox.ClearChildren();
		if (EntryWidgetClass == nullptr)
		{
			Warning("UGridItemContextMenuWidget::Initialize - EntryWidgetClass is nullptr");
			return;
		}

		for (int32 i = 0; i < InActions.Num(); i++)
		{
			auto Entry = Cast<UGridItemContextMenuEntryWidget>(WidgetBlueprint::CreateWidget(EntryWidgetClass, GetOwningPlayer()));
			if (Entry == nullptr)
			{
				continue;
			}
			Entry.Initialize(ItemInstance, InActions[i]);
			ActionsBox.AddChild(Entry);
		}
	}
}
