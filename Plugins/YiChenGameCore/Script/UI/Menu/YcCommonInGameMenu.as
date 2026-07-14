class UYcCommonInGameMenu : UYcActivatableWidget
{
	// 返回游戏按钮
	UPROPERTY(BindWidget)
	UYcButtonBase ReturnGameButton;

	// 游戏设置按钮
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UYcButtonBase OptionsButtons;

	// 返回主菜单按钮
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UYcButtonBase ReturnMenuButton;

	// 退出游戏
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UCommonHardwareVisibilityBorder EditorOnlyVisibilityBorder;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UYcButtonBase QuitGameButtonPIE;

	UPROPERTY(BindWidgetAnim)
	UWidgetAnimation OnActivatedAnim;

	UPROPERTY()
	TSoftClassPtr<UCommonActivatableWidget> OptionsWidgetClass;

	UPROPERTY()
	FGameplayTag OptionsLayerName;

	UPROPERTY()
	FText ReturnMainMenuConfirmationTitle;

	UPROPERTY()
	FText ReturnMainMenuConfirmationMessage;

	UPROPERTY()
	TSoftObjectPtr<UWorld> MainMenuWorld;

	UPROPERTY()
	FPrimaryAssetId MainMenuExperienceReference;

	default OptionsLayerName = GameplayTags::UI_Layer_Menu;
	default ReturnMainMenuConfirmationTitle = FText::FromString("Return Main Menu");
	default ReturnMainMenuConfirmationMessage = FText::FromString("Are you sure you want to return to the main menu?");
	default InputConfig = EYcWidgetInputMode::Menu;

	// Back设置
	default bIsBackHandler = true;
	default bIsBackActionDisplayedInActionBar = true;
	default OverrideBackActionDisplayName = FText::FromString("BackGame");

	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		ReturnGameButton.OnButtonBaseClicked.AddUFunction(this, n"OnReturnGameButtonClicked");
		QuitGameButtonPIE.OnButtonBaseClicked.AddUFunction(this, n"OnQuitGameButtonClickedPIE");
		OptionsButtons.OnButtonBaseClicked.AddUFunction(this, n"OnOptionsButtonsClicked");
		ReturnMenuButton.OnButtonBaseClicked.AddUFunction(this, n"OnReturnMenuButtonClicked");
	}

	UFUNCTION(BlueprintOverride)
	void OnActivated()
	{
		PlayAnimationForward(OnActivatedAnim);
	}

	// 返回游戏按钮点击处理函数
	UFUNCTION()
	void OnReturnGameButtonClicked(UCommonButtonBase Button)
	{
		DeactivateWidget();
	}

	// 游戏设置按钮点击处理函数
	UFUNCTION()
	void OnOptionsButtonsClicked(UCommonButtonBase Button)
	{
		if (OptionsWidgetClass.IsValid() == false)
		{
			Error("OptionsWidgetClass is not set");
			return;
		}
		auto Action = UAsyncAction_PushContentToLayerForPlayer::PushContentToLayerForPlayer(GetOwningPlayer(), OptionsWidgetClass, OptionsLayerName);
		Action.BeforePush.AddUFunction(this, n"OnBeforePushOptionsLayer");
	}

	UFUNCTION()
	void OnBeforePushOptionsLayer(UCommonActivatableWidget UserWidget)
	{
		DeactivateWidget();
	}

	// 返回主菜单按钮点击处理函数
	UFUNCTION()
	void OnReturnMenuButtonClicked(UCommonButtonBase Button)
	{
		auto Action = UAsyncAction_ShowConfirmation::ShowConfirmationYesNo(ReturnMainMenuConfirmationTitle, ReturnMainMenuConfirmationMessage);
		Action.OnResult.AddUFunction(this, n"OnReturnMenuButtonClickedResult");
		Action.Activate();
	}

	UFUNCTION()
	void OnReturnMenuButtonClickedResult(ECommonMessagingResult Result)
	{
		switch (Result)
		{
			case ECommonMessagingResult::Confirmed:
				if (MainMenuWorld.IsValid())
				{
					FString Options = f"Experience={MainMenuExperienceReference.PrimaryAssetName}";
					Gameplay::OpenLevelBySoftObjectPtr(MainMenuWorld, true, Options);
					DeactivateWidget();
				}
				break;
			default:
				break;
		}
	}

	UFUNCTION()
	void OnReturnMenuButtonClickedYes()
	{
		ActivateWidget();
	}

	UFUNCTION()
	void OnReturnMenuButtonClickedNo()
	{
		DeactivateWidget();
	}

	// 退出游戏按钮点击处理函数
	UFUNCTION()
	void OnQuitGameButtonClickedPIE(UCommonButtonBase Button)
	{

		UCommonInputSubsystem::Get(GetOwningPlayer()).SetCurrentInputType(ECommonInputType::MouseAndKeyboard);
		System::QuitGame(GetOwningPlayer(), EQuitPreference::Quit, false);
	}
}