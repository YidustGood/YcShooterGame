/**
 * 角色重生倒计时 Widget
 *
 * 功能：
 * - 显示角色死亡后的重生倒计时
 * - 监听重生消息并自动更新倒计时
 * - 支持进入/退出动画
 */
class UCharacterRespawnTimerWidget : UUserWidget
{
	// ========================================
	// UI 元素
	// ========================================

	/** 倒计时文本（显示剩余秒数） */
	UPROPERTY(BindWidget)
	UTextBlock RemainingTimeText;

	/** 进入动画（显示倒计时时播放） */
	UPROPERTY(BindWidgetAnim)
	UWidgetAnimation Intro;

	/** 退出动画（倒计时结束时播放） */
	UPROPERTY(BindWidgetAnim)
	UWidgetAnimation Outro;

	// ========================================
	// 内部变量
	// ========================================

	/** 重生总时长（秒） */
	float Duration;

	/** 开始重生的时间戳 */
	float StartTime;

	/** 是否正在重生中 */
	bool bRespawning;

	/** 重生时长变化消息监听器 */
	FGameplayMessageListenerHandle RespawnDurationAsyncAction;

	/** 重生完成消息监听器 */
	FGameplayMessageListenerHandle RespawnCompletedAsyncAction;

	// ========================================
	// 生命周期
	// ========================================

	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		ListenForDuration();
		SetVisibility(ESlateVisibility::Collapsed);
	}

	UFUNCTION(BlueprintOverride)
	void Destruct()
	{
		// 清理消息监听器
		RespawnDurationAsyncAction.Unregister();
		RespawnCompletedAsyncAction.Unregister();
	}

	// ========================================
	// 消息监听
	// ========================================

	/**
	 * 注册消息监听器
	 * 监听重生时长变化和重生完成消息
	 */
	void ListenForDuration()
	{
		// 监听重生时长变化消息
		RespawnDurationAsyncAction = UGameplayMessageSubsystem::Get().RegisterListener(
			GameplayTags::Character_Respawn_DurationChanged,
			this,
			n"OnRespawnDurationChanged",
			FYcInteractionDurationMessage(),
			EGameplayMessageMatch::ExactMatch);

		// 监听重生完成消息
		RespawnCompletedAsyncAction = UGameplayMessageSubsystem::Get().RegisterListener(
			GameplayTags::Ability_Respawn_Completed_Message,
			this,
			n"OnRespawnCompleted",
			FYcGameVerbMessage(),
			EGameplayMessageMatch::ExactMatch);
	}

	// ========================================
	// 消息处理
	// ========================================

	/**
	 * 收到重生时长变化消息
	 * 开始显示倒计时
	 */
	UFUNCTION()
	void OnRespawnDurationChanged(FGameplayTag Tag, FYcInteractionDurationMessage Data)
	{
		// 只处理本地玩家的消息
		if (Data.Instigator != GetOwningPlayer().PlayerState)
			return;

		Duration = Data.Duration;
		StartTime = Gameplay::GetTimeSeconds();

		// 首次进入重生状态，播放进入动画
		if (!bRespawning)
		{
			PlayAnimation(Intro);
		}

		bRespawning = true;

		// 立即更新一次文本
		UpdateRespawnText();

		// 启动定时器，每秒更新一次倒计时
		System::SetTimer(this, n"UpdateRespawnText", 1, true);

		SetVisibility(ESlateVisibility::Visible);
	}

	/**
	 * 更新倒计时文本
	 * 每秒调用一次
	 */
	UFUNCTION()
	void UpdateRespawnText()
	{
		// 计算剩余时间
		float TimeUntilRespawn = (StartTime + Duration) - Gameplay::GetTimeSeconds();

		if (TimeUntilRespawn <= 0)
		{
			// 倒计时结束，播放退出动画
			bRespawning = false;
			RemainingTimeText.SetText(FText::FromString("0"));
			PlayAnimation(Outro);
			BindToAnimationFinished(Outro, FWidgetAnimationDynamicEvent(this, n"OnTimerFinished"));
			return;
		}

		// 更新倒计时文本（向上取整）
		RemainingTimeText.SetText(FText::FromString(f"{Math::CeilToInt(TimeUntilRespawn)}"));
	}

	/**
	 * 收到重生完成消息
	 * 立即隐藏倒计时
	 */
	UFUNCTION()
	void OnRespawnCompleted(FGameplayTag Tag, FYcGameVerbMessage Data)
	{
		// 只处理本地玩家的消息
		if (Data.Instigator != GetOwningPlayer().PlayerState)
			return;

		bRespawning = false;
		OnTimerFinished();
	}

	/**
	 * 倒计时结束或重生完成
	 * 隐藏 Widget 并清理定时器
	 */
	UFUNCTION()
	void OnTimerFinished()
	{
		SetVisibility(ESlateVisibility::Collapsed);
		System::ClearTimer(this, "UpdateRespawnText");
	}
}