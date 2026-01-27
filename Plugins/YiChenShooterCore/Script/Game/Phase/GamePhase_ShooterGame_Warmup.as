/**
 * 对局热身阶段 GameplayAbility
 * 
 * 功能说明：
 * - 在正式比赛开始前给玩家热身时间
 * - 显示倒计时UI提示玩家比赛即将开始
 * - 可应用热身阶段专属的 GameplayEffect（如无敌效果）
 * 
 * 执行流程：
 * 1. 等待玩家加入（WaitForPlayersDuration - CountdownDuration）
 * 2. 显示倒计时UI（CountdownDuration 秒）
 * 3. 广播倒计时消息
 * 4. 倒计时结束后进入下一阶段
 */
class UGamePhase_ShooterGame_Warmup : UYcGamePhaseAbility
{
	// ========================================
	// 配置属性
	// ========================================
	
	/** 热身阶段总时长（秒） */
	UPROPERTY()
	float WaitForPlayersDuration = 10.0f;

	/** 热身阶段倒计时显示时长（秒） */
	UPROPERTY()
	int CountdownDuration = 5;

	/** 热身阶段要应用的 GameplayEffect（如无敌效果） */
	UPROPERTY()
	TSubclassOf<UGameplayEffect> WarmupEffect;

	/** 热身倒计时 UI 类 */
	UPROPERTY()
	TSubclassOf<UPhaseCountdownWidget> WarmupWidgetClass;

	// ========================================
	// 技能配置
	// ========================================
	
	default ActivationGroup = EYcAbilityActivationGroup::Independent;
	default GamePhaseTag = FGameplayTag::RequestGameplayTag(n"ShooterGame.GamePhase.Warmup");

	// ========================================
	// 内部变量
	// ========================================
	
	/** 倒计时剩余秒数 */
	private int CountdownSecsRemaining = 0;

	// ========================================
	// 生命周期函数
	// ========================================
	
	UFUNCTION(BlueprintOverride)
	void ActivateAbility()
	{
		// 检查开发者设置是否跳过热身阶段
		if (UYcGameDeveloperSettings::ShouldSkipDirectlyToGameplay())
		{
			Log(f"[{GetClass().GetName()}] 开发者设置跳过热身阶段");
			TransitionToPlayingPhase();
			return;
		}

		// 验证并修正时长配置
		SanitizeDurations();

		// 应用热身阶段 GameplayEffect
		if (WarmupEffect != nullptr)
		{
			UYcGlobalAbilitySystem::Get().ApplyEffectToAll(WarmupEffect);
		}

		// 等待一段时间后开始倒计时
		float WaitTime = WaitForPlayersDuration - CountdownDuration;
		if (WaitTime > 0)
		{
			DelayForAs(n"StartCountdown", WaitTime);
		}
		else
		{
			StartCountdown();
		}
	}

	UFUNCTION(BlueprintOverride)
	void OnEndAbility(bool bWasCancelled)
	{
		Log(f"[{GetClass().GetName()}] 热身阶段结束");
	}

	// ========================================
	// 倒计时逻辑
	// ========================================
	
	/**
	 * 开始倒计时
	 * 创建倒计时 UI 并开始广播倒计时消息
	 */
	UFUNCTION()
	private void StartCountdown()
	{
		// 为所有玩家创建倒计时 UI
		CreateCountdownWidgetForAllPlayers();

		// 初始化倒计时
		CountdownSecsRemaining = CountdownDuration;
		BroadcastCountdown();
	}

	/**
	 * 为所有玩家创建倒计时 UI
	 */
	private void CreateCountdownWidgetForAllPlayers()
	{
		if (WarmupWidgetClass == nullptr)
		{
			Warning(f"[{GetClass().GetName()}] WarmupWidgetClass 未设置");
			return;
		}

		auto GameState = Gameplay::GetGameState();
		if (GameState == nullptr)
		{
			Warning(f"[{GetClass().GetName()}] 无法获取 GameState");
			return;
		}

		for (auto& PlayerState : GameState.PlayerArray)
		{
			auto YcPS = Cast<AYcPlayerStateScript>(PlayerState.Get());
			if (YcPS != nullptr)
			{
				YcPS.ClientCreateWidget(GameplayTags::HUD_Slot_PhaseMessage, WarmupWidgetClass);
			}
		}
	}

	/**
	 * 广播倒计时消息
	 * 每秒递归调用，直到倒计时结束
	 */
	UFUNCTION()
	private void BroadcastCountdown()
	{
		if (CountdownSecsRemaining < 0)
		{
			OnCountdownFinished();
			return;
		}

		// 广播倒计时消息
		BroadcastCountdownMessage(CountdownSecsRemaining);

		// 递减倒计时并延迟1秒后继续
		CountdownSecsRemaining--;
		DelayForAs(n"BroadcastCountdown", 1.0f);
	}

	/**
	 * 广播倒计时消息到所有客户端
	 * @param SecsRemaining 剩余秒数
	 */
	private void BroadcastCountdownMessage(int SecsRemaining)
	{
		auto GameState = Cast<AYcGameState>(Gameplay::GetGameState());
		if (GameState == nullptr)
		{
			Warning(f"[{GetClass().GetName()}] 无法获取 YcGameState");
			return;
		}

		// 构造倒计时消息
		FYcGameVerbMessage Message;
		Message.Verb = GameplayTags::ShooterGame_GamePhase_MatchBeginCountdown;
		Message.Magnitude = SecsRemaining;

		// 通过 GameState 广播消息
		GameState.MulticastMessageToClients(Message);

		// 在非专用服务器上也本地广播
		if (!System::IsDedicatedServer())
		{
			UGameplayMessageSubsystem::Get().BroadcastMessage(
				GameplayTags::ShooterGame_GamePhase_MatchBeginCountdown, 
				Message
			);
		}

		Log(f"[{GetClass().GetName()}] 比赛开始倒计时: {SecsRemaining} 秒");
	}

	/**
	 * 倒计时结束处理
	 */
	private void OnCountdownFinished()
	{
		Log(f"[{GetClass().GetName()}] 倒计时结束，准备进入游玩阶段");

		// 移除热身阶段 GameplayEffect
		if (WarmupEffect != nullptr)
		{
			UYcGlobalAbilitySystem::Get().RemoveEffectFromAll(WarmupEffect);
		}

		// 切换到游玩阶段
		TransitionToPlayingPhase();
	}

	/**
	 * 切换到游玩阶段
	 */
	private void TransitionToPlayingPhase()
	{
		auto PhaseComponent = UYcGamePhaseComponent::GetGamePhaseComponent();
		if (PhaseComponent == nullptr)
		{
			Error(f"[{GetClass().GetName()}] 无法获取 YcGamePhaseComponent");
			return;
		}

		PhaseComponent.StartNextPhase();
	}

	// ========================================
	// 配置验证
	// ========================================
	
	/**
	 * 验证并修正时长配置
	 * 确保配置值在合理范围内
	 */
	private void SanitizeDurations()
	{
		float OriginalWaitDuration = WaitForPlayersDuration;
		int OriginalCountdownDuration = CountdownDuration;

		// 修正为合理值
		WaitForPlayersDuration = Math::Max(WaitForPlayersDuration, 0.0f);
		CountdownDuration = Math::Min(Math::Max(CountdownDuration, 0), int(WaitForPlayersDuration));

		// 检查是否有修改
		bool bWaitDurationChanged = WaitForPlayersDuration != OriginalWaitDuration;
		bool bCountdownDurationChanged = CountdownDuration != OriginalCountdownDuration;

		if (bWaitDurationChanged || bCountdownDurationChanged)
		{
			Warning(f"[{GetClass().GetName()}] 热身阶段时长配置已自动修正为合理范围");
			Log(f"  WaitForPlayersDuration: {OriginalWaitDuration} -> {WaitForPlayersDuration}");
			Log(f"  CountdownDuration: {OriginalCountdownDuration} -> {CountdownDuration}");
		}
	}
}
