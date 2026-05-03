// GridItem 悬停提示基类：
// - 负责从 HoverTooltip payload 中读取物品实例
// - 默认显示物品名称和描述
// - 负责把提示框定位到鼠标附近
//
// 推荐做法：
// 1. 在 UMG 中继承该脚本类
// 2. 绑定 ItemNameText / ItemDescriptionText，并自行扩展其他视觉节点
// 3. 在 GridItemWidget 对应的蓝图默认值里，将 HoverTooltipWidgetClass 指向这个 UMG 子类
class UGridItemHoverTooltipWidget : UYcHoverTooltipWidgetBase
{
	UPROPERTY(BindWidget)
	UTextBlock ItemNameText;

	UPROPERTY(BindWidget)
	UTextBlock ItemDescriptionText;

	private UYcInventoryItemInstance CurrentItemInstance;
	private FText CurrentItemName;
	private FText CurrentItemDescription;

	UFUNCTION(BlueprintOverride)
	void OnHoverTooltipPayloadUpdated()
	{
		RefreshFromHoverPayload();
		SetVisibility(ESlateVisibility::HitTestInvisible);
	}

	UFUNCTION(BlueprintOverride)
	void OnHoverTooltipPayloadHidden()
	{
		CurrentItemInstance = nullptr;
		CurrentItemName = FText();
		CurrentItemDescription = FText();
		SetVisibility(ESlateVisibility::Collapsed);
	}

	UFUNCTION(BlueprintCallable)
	UYcInventoryItemInstance GetCurrentTooltipItemInstance() const
	{
		return CurrentItemInstance;
	}

	UFUNCTION(BlueprintCallable)
	FText GetCurrentTooltipItemName() const
	{
		return CurrentItemName;
	}

	UFUNCTION(BlueprintCallable)
	FText GetCurrentTooltipItemDescription() const
	{
		return CurrentItemDescription;
	}

	UFUNCTION(BlueprintCallable)
	void RefreshScreenPosition()
	{
		RefreshHoverTooltipViewportPosition();
	}

	private void RefreshFromHoverPayload()
	{
		FInstancedStruct Payload;
		GetHoverTooltipContentPayload(Payload);
		if (!Payload.IsValid() || !Payload.Contains(FYcGridItemHoverTooltipData))
		{
			return;
		}

		FYcGridItemHoverTooltipData Data = Payload.Get(FYcGridItemHoverTooltipData);
		CurrentItemInstance = Data.ItemInstance;
		CurrentItemName = ResolveItemName(CurrentItemInstance);
		CurrentItemDescription = ResolveItemDescription(CurrentItemInstance);

		if (ItemNameText != nullptr)
		{
			ItemNameText.SetText(CurrentItemName);
		}

		if (ItemDescriptionText != nullptr)
		{
			const bool bShouldShowDescription = ShouldShowItemDescription(CurrentItemName, CurrentItemDescription);
			ItemDescriptionText.SetText(CurrentItemDescription);
			ItemDescriptionText.SetVisibility(bShouldShowDescription ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
	}

	private FText ResolveItemName(UYcInventoryItemInstance ItemInstance) const
	{
		if (ItemInstance == nullptr)
		{
			return FText();
		}

		FYcInventoryItemDefinition ItemDef;
		if (ItemInstance.GetItemDef(ItemDef))
		{
			return ItemDef.DisplayName;
		}

		return FText::FromString(ItemInstance.ItemRegistryId.ItemName.ToString());
	}

	private FText ResolveItemDescription(UYcInventoryItemInstance ItemInstance) const
	{
		if (ItemInstance == nullptr)
		{
			return FText();
		}

		FYcInventoryItemDefinition ItemDef;
		if (ItemInstance.GetItemDef(ItemDef))
		{
			return ItemDef.Description;
		}

		return FText();
	}

	private bool ShouldShowItemDescription(const FText& ItemName, const FText& ItemDescription) const
	{
		if (ItemDescription.IsEmpty())
		{
			return false;
		}

		return !ItemName.EqualTo(ItemDescription);
	}
}
