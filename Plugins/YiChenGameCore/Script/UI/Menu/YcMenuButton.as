class UYcMenuButton : UYcButtonBase
{
	// 文本/图标显示切换器
	UPROPERTY(BindWidget, NotVisible)
	UCommonActivatableWidgetSwitcher TextIconSwitcher;

	UPROPERTY(BindWidget, NotVisible)
	UCommonTextBlock ButtonTextBlock;

	UPROPERTY(BindWidget, NotVisible)
	UImage ButtonIconImage;

	UPROPERTY(BindWidget, NotVisible)
	UOverlay ButtonIconOverlay;

	UPROPERTY(BindWidget, NotVisible)
	UOverlay ButtonTextOverlay;

	UPROPERTY()
	bool bUseIconOverride;

	UPROPERTY()
	FSlateFontInfo ButtonTextFont;

	UPROPERTY()
	ETextTransformPolicy ButtonTextTransformPolicy;

	UPROPERTY()
	FSlateBrush IconBrush;

	UFUNCTION(BlueprintOverride)
	void PreConstruct(bool IsDesignTime)
	{
		PreUpdateTextStyle();
		PreUpdateButtonStyle();
	}

	UFUNCTION(BlueprintOverride)
	void UpdateButtonText(FText InText)
	{
		ButtonTextBlock.Text = InText;
	}

	void PreUpdateTextStyle()
	{
		ButtonTextBlock.SetFont(ButtonTextFont);
		ButtonTextBlock.SetTextTransformPolicy(ButtonTextTransformPolicy);
	}

	void PreUpdateButtonStyle()
	{
		if (bUseIconOverride)
		{
			TextIconSwitcher.SetActiveWidget(ButtonIconOverlay);
			ButtonIconImage.SetBrush(IconBrush);
		}
		else
		{
			TextIconSwitcher.SetActiveWidget(ButtonTextOverlay);
		}
	}
}