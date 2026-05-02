// 快捷栏当前激活物品 HUD：显示当前装备物品的图标、名称与弹药信息。
class UQuickBarActiveItemHUDWidget : UCommonUserWidget
{
	// 物品图标显示控件。
	UPROPERTY(BindWidget)
	UImage ItemIconImage;

	// 物品名称文本控件。
	UPROPERTY(BindWidget)
	UTextBlock ItemNameText;

	// 当前弹匣子弹数文本控件。
	UPROPERTY(BindWidget)
	UTextBlock MagazineAmmoText;

	// 备用子弹数文本控件。
	UPROPERTY(BindWidget)
	UTextBlock SpareAmmoText;

	// 用于渲染物品图标的材质。
	UPROPERTY(Category = "HUD")
	UMaterialInterface ItemIconMaterial;

	// 设计时预览用的图标贴图。
	UPROPERTY(Category = "Design Time")
	UTexture2D DesignTimeIconTexture;

	// 设计时预览用的默认物品名称。
	UPROPERTY(Category = "Design Time")
	FText DefaultItemName = FText::FromString("Weapon Name");

	// 设计时预览用的默认弹匣子弹数文本。
	UPROPERTY(Category = "Design Time")
	FText DefaultMagazineAmmoText = FText::FromString("30");

	// 设计时预览用的默认备用子弹数文本。
	UPROPERTY(Category = "Design Time")
	FText DefaultSpareAmmoText = FText::FromString("90");

	// 当前正在 HUD 中显示的物品实例。
	UPROPERTY(NotVisible)
	UYcInventoryItemInstance CurrentItemInstance;

	// 运行时创建的物品图标动态材质实例。
	UPROPERTY(NotVisible)
	UMaterialInstanceDynamic ItemIconMaterialInstance;

	// 玩家身上的快捷栏组件引用。
	UYcQuickBarComponent QuickBarComponent;
	// 监听当前激活槽位变化的消息句柄。
	FGameplayMessageListenerHandle QuickBarActiveIndexChangedHandle;
	// 监听快捷栏槽位内容变化的消息句柄。
	FGameplayMessageListenerHandle QuickBarSlotsChangedHandle;

	UFUNCTION(BlueprintOverride)
	void PreConstruct(bool IsDesignTime)
	{
		if (IsDesignTime)
		{
			ApplyItemName(DefaultItemName);
			ApplyAmmoTexts(DefaultMagazineAmmoText, DefaultSpareAmmoText, true);
			ApplyDesignTimeIcon();
			return;
		}

		ApplyRuntimeEmptyState();
	}

	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		QuickBarComponent = UYcQuickBarComponent::FindQuickBarComponent(GetOwningPlayer());

		QuickBarActiveIndexChangedHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			GameplayTags::Yc_QuickBar_Message_ActiveIndexChanged,
			this,
			n"OnQuickBarActiveIndexChanged",
			FYcQuickBarActiveIndexChangedMessage(),
			EGameplayMessageMatch::ExactMatch);

		QuickBarSlotsChangedHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			GameplayTags::Yc_QuickBar_Message_SlotsChanged,
			this,
			n"OnQuickBarSlotsChanged",
			FYcQuickBarSlotsChangedMessage(),
			EGameplayMessageMatch::ExactMatch);

		RefreshFromQuickBarState();
	}

	UFUNCTION(BlueprintOverride)
	void Tick(FGeometry MyGeometry, float InDeltaTime)
	{
		if (CurrentItemInstance == nullptr)
		{
			return;
		}

		RefreshAmmoText();
	}

	UFUNCTION(BlueprintOverride)
	void Destruct()
	{
		QuickBarActiveIndexChangedHandle.Unregister();
		QuickBarSlotsChangedHandle.Unregister();
	}

	UFUNCTION()
	void OnQuickBarActiveIndexChanged(FGameplayTag ActualTag, FYcQuickBarActiveIndexChangedMessage Data)
	{
		if (Data.Owner != GetOwningPlayer())
		{
			return;
		}

		RefreshFromQuickBarState();
	}

	UFUNCTION()
	void OnQuickBarSlotsChanged(FGameplayTag ActualTag, FYcQuickBarSlotsChangedMessage Data)
	{
		if (Data.Owner != GetOwningPlayer())
		{
			return;
		}

		RefreshFromQuickBarState();
	}

	void RefreshFromQuickBarState()
	{
		if (QuickBarComponent == nullptr)
		{
			QuickBarComponent = UYcQuickBarComponent::FindQuickBarComponent(GetOwningPlayer());
			if (QuickBarComponent == nullptr)
			{
				ApplyItemState(nullptr);
				return;
			}
		}

		auto ActiveItem = QuickBarComponent.GetActiveSlotItem();
		if (ActiveItem == nullptr)
		{
			return;
		}

		ApplyItemState(ActiveItem);
	}

	void ApplyItemState(UYcInventoryItemInstance InItemInstance)
	{
		CurrentItemInstance = InItemInstance;

		if (CurrentItemInstance == nullptr)
		{
			ApplyRuntimeEmptyState();
			return;
		}

		FYcInventoryItemDefinition ItemDef;
		if (CurrentItemInstance.GetItemDef(ItemDef))
		{
			ApplyItemName(ItemDef.DisplayName);
		}
		else
		{
			ApplyItemName(FText());
		}

		RefreshAmmoText();
		RefreshItemIcon();
	}

	void RefreshAmmoText()
	{
		auto InitialItemStatsFragment = CurrentItemInstance.FindItemFragment(FInventoryFragment_InitialItemStats);
		const int32 MagazineSize = CurrentItemInstance.GetStatTagStackCount(GameplayTags::Weapon_Stat_MagazineSize);
		const int32 MagazineAmmo = CurrentItemInstance.GetStatTagStackCount(GameplayTags::Weapon_Stat_MagazineAmmo);
		const int32 SpareAmmo = CurrentItemInstance.GetStatTagStackCount(GameplayTags::Weapon_Stat_SpareAmmo);
		const bool bShowAmmoText = InitialItemStatsFragment.IsValid() && MagazineSize > 0;

		if (!bShowAmmoText)
		{
			ApplyAmmoTexts(DefaultMagazineAmmoText, DefaultSpareAmmoText, false);
			return;
		}

		ApplyAmmoTexts(
			FText::FromString(f"{MagazineAmmo}"),
			FText::FromString(f"{SpareAmmo}"),
			true);
	}

	void RefreshItemIcon()
	{
		auto HUDIconFragment = CurrentItemInstance.FindItemFragment(FYcInventoryFragment_QuickBarSlotHUDIcon);
		if (!HUDIconFragment.IsValid())
		{
			ApplyItemIcon(nullptr);
			return;
		}

		ApplyItemIcon(HUDIconFragment.Get(FYcInventoryFragment_QuickBarSlotHUDIcon).IconTexture);
	}

	void ApplyItemName(FText InText)
	{
		if (ItemNameText == nullptr)
		{
			return;
		}

		ItemNameText.SetText(InText);
	}

	void ApplyAmmoTexts(FText InMagazineAmmoText, FText InSpareAmmoText, bool bShouldShow)
	{
		if (MagazineAmmoText != nullptr)
		{
			MagazineAmmoText.SetText(InMagazineAmmoText);
			MagazineAmmoText.SetVisibility(bShouldShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}

		if (SpareAmmoText != nullptr)
		{
			SpareAmmoText.SetText(InSpareAmmoText);
			SpareAmmoText.SetVisibility(bShouldShow ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}

	void ApplyItemIcon(UTexture2D InIconTexture)
	{
		if (ItemIconImage == nullptr)
		{
			return;
		}

		if (InIconTexture == nullptr || ItemIconMaterial == nullptr)
		{
			ItemIconImage.SetBrushFromMaterial(nullptr);
			ItemIconImage.SetVisibility(ESlateVisibility::Collapsed);
			return;
		}

		if (ItemIconMaterialInstance == nullptr)
		{
			ItemIconMaterialInstance = Material::CreateDynamicMaterialInstance(ItemIconMaterial);
		}

		if (ItemIconMaterialInstance == nullptr)
		{
			ItemIconImage.SetBrushFromMaterial(nullptr);
			ItemIconImage.SetVisibility(ESlateVisibility::Collapsed);
			return;
		}

		ItemIconMaterialInstance.SetTextureParameterValue(n"ItemIcon", InIconTexture);
		ItemIconImage.SetBrushFromMaterial(ItemIconMaterialInstance);
		ItemIconImage.SetVisibility(ESlateVisibility::Visible);
	}

	void ApplyDesignTimeIcon()
	{
		if (ItemIconImage == nullptr)
		{
			return;
		}

		if (ItemIconMaterial != nullptr)
		{
			if (ItemIconMaterialInstance == nullptr)
			{
				ItemIconMaterialInstance = Material::CreateDynamicMaterialInstance(ItemIconMaterial);
			}

			if (ItemIconMaterialInstance != nullptr)
			{
				ItemIconMaterialInstance.SetTextureParameterValue(n"ItemIcon", DesignTimeIconTexture);
				ItemIconImage.SetBrushFromMaterial(ItemIconMaterialInstance);
			}
		}

		ItemIconImage.SetVisibility(ESlateVisibility::Visible);
	}

	void ApplyRuntimeEmptyState()
	{
		ApplyItemName(FText());
		ApplyAmmoTexts(FText(), FText(), false);
		ApplyItemIcon(nullptr);
	}
}
