class UPhaseCountdownWidget : UUserWidget
{

	UPROPERTY(BindWidget)
	UCommonTextBlock CountdownText;

	FGameplayMessageListenerHandle CountdownListenerHandle;
	UFUNCTION(BlueprintOverride)
	void Construct()
	{
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
	}
}