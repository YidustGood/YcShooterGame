/** 玩家自己可见的击杀反馈UI, 当前实现为击杀图标反馈 */
class UEliminationFeedWidget : UCommonUserWidget
{
	UPROPERTY(NotEditable, Transient, meta = (BindWidgetAnim), Category = "Animation")
	UWidgetAnimation Elimination;

	FGameplayMessageListenerHandle EliminationListennerHandler;

	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		auto GameplayMessage = UGameplayMessageSubsystem::Get();
		// 监听消灭玩家消息
		EliminationListennerHandler = GameplayMessage.RegisterListener(
			FGameplayTag::RequestGameplayTag(n"Yc.Elimination.Message"),
			this,
			n"OnEliminate",
			FYcGameVerbMessage(),
			EGameplayMessageMatch::ExactMatch);
	}

	UFUNCTION(BlueprintOverride)
	void Destruct()
	{
		EliminationListennerHandler.Unregister();
	}

	// 当我们消灭了一个玩家时做出动画
	UFUNCTION()
	private void OnEliminate(FGameplayTag ActualTag, FYcGameVerbMessage Data)
	{
		auto PS = Cast<AYcPlayerState>(Data.Instigator);
		bool bFlag1 = GetOwningPlayer() == PS.GetYcPlayerController();
		bool bFlag2 = Data.Target != PS;
		if (bFlag1 && bFlag2)
		{
			if (IsAnimationPlayingForward(Elimination))
			{
				StopAnimation(Elimination);
				SetAnimationCurrentTime(Elimination, 0);
			}
			PlayAnimationForward(Elimination);
		}
	}
}