/**
 * [通用类可迁移至C++实现, 便于非AS项目使用]
 *
 * 这个widget是用于处理专属于当前装备的武器的UI
 * 通过获取ItemFragment中配置的UI,在这里进行添加，并且控制其显示/隐藏
 * 使用方式是在讲这个UI挂载到需要的UI扩展点上,然后会自动生成相关UI
 */
class UWeaponReticleHostWidget : UCommonUserWidget
{
	// Widget Binds
	UPROPERTY(BindWidget)
	USizeBox VisWrapper;

	UPROPERTY(BindWidget)
	UOverlay WidgetStack;

	FGameplayMessageListenerHandle QuickBarActiveItemListenerHandle;
	TArray<UUserWidget> SpawnedWidgets;

	UFUNCTION(BlueprintOverride)
	void OnInitialized()
	{
		// AS脚本RegisterListener返回的handler必须在EndPlay或者合适的时机被清理掉,否则会引起崩溃!!!
		QuickBarActiveItemListenerHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			GameplayTags::Yc_QuickBar_Message_ActiveIndexChanged,
			this,
			n"OnQuickBarActiveIndexChanged",
			FYcQuickBarActiveIndexChangedMessage(),
			EGameplayMessageMatch::ExactMatch);
	}

	/**
	 * 收到QuickBar激活的武器改变消息后尝试匹配更新准星UI
	 */
	UFUNCTION()
	private void OnQuickBarActiveIndexChanged(FGameplayTag ActualTag, FYcQuickBarActiveIndexChangedMessage Data)
	{
		auto QuickBar = UYcQuickBarComponent::FindQuickBarComponent(GetOwningPlayerPawn());
		if (QuickBar == nullptr)
			return;

		UYcEquipmentInstance EquipmentInst = QuickBar.GetActiveEquipmentInstance();
		FYcEquipmentFragment_ReticleConfig ReticleConfig;
		YcWeapon::GetWeaponReticleConfig(EquipmentInst, ReticleConfig);

		if (EquipmentInst == nullptr)
			return;

		if (ReticleConfig.ReticleWidgets.Num() == 0)
		{
			Warning("UWeaponReticleHostWidget::OnQuickBarActiveIndexChanged: 武器准星Widget配置为空, 无法创建准星UI");
			return;
		}

		ClearExistingWidgets();

		// 将武器ReticleConfig中的所有widget进行创建添加

		for (auto Widget : ReticleConfig.ReticleWidgets)
		{
			UUserWidget UserWidget = WidgetBlueprint::CreateWidget(Widget, OwningPlayer);
			SpawnedWidgets.Add(UserWidget);
			auto ReticleWidget = Cast<UWeaponReticleWidget>(UserWidget);
			// ReticleWidget.InitializeFromWeapon(NewWeapon);
			auto OverlaySlot = WidgetStack.AddChildToOverlay(UserWidget);
			OverlaySlot.SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
			OverlaySlot.SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
		}
	}

	// 清空已生成的Widgets
	void ClearExistingWidgets()
	{
		for (auto Widget : SpawnedWidgets)
		{
			WidgetStack.RemoveChild(Widget);
		}
		SpawnedWidgets.Empty();
	}
}