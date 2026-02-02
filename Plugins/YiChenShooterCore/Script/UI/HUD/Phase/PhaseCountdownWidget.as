class UPhaseCountdownWidget : UUserWidget
{

	UPROPERTY(BindWidget)
	UCommonTextBlock CountdownText;

	FGameplayMessageListenerHandle CountdownListenerHandle;
	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		// 创建出来后默认隐藏, 收到倒计时消息时显示
		SetVisibility(ESlateVisibility::Collapsed);
		CountdownListenerHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			GameplayTags::ShooterGame_GamePhase_MatchBeginCountdown,
			this,
			n"OnReceivedMatchBeginCountdown",
			FYcGameVerbMessage(),
			EGameplayMessageMatch::ExactMatch);
	}

	UFUNCTION(BlueprintOverride)
	void Destruct()
	{
		CountdownListenerHandle.Unregister();
		CountdownText.SetText(FText());
	}

	UFUNCTION()
	private void OnReceivedMatchBeginCountdown(FGameplayTag ActualTag, FYcGameVerbMessage Data)
	{
		// 设置倒计时文本
		CountdownText.SetText(FText::FromString(f"{Data.Magnitude}"));
		SetVisibility(ESlateVisibility::Visible);
	}
}