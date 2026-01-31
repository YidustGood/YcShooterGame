event void FCountDownUpdate(float Time);

/**
 * 射击游戏 Playing 阶段基类
 *
 * 功能说明：
 * - 管理比赛倒计时
 * - 监听击杀/助攻消息并处理计分
 * - 检查胜利条件并切换到赛后阶段
 * - 重置玩家状态
 *
 * 子类职责：
 * - 实现 ProcessScore() 处理具体的计分逻辑（团队/个人）
 * - 实现 CheckVictoryCondition() 检查胜利条件
 * - 实现 OnMatchTimeExpired() 处理时间耗尽逻辑
 */
class UGamePhase_ShooterGame_Playing : UYcGamePhaseAbility
{
	// ========================================
	// 配置属性
	// ========================================

	/** 比赛对局时长（秒） */
	UPROPERTY(EditDefaultsOnly, Category = "Match Settings")
	protected float MatchDuration = 600;

	/** 赢下比赛所需的目标得分 */
	UPROPERTY(EditDefaultsOnly, Category = "Match Settings")
	protected int TargetScore = 25;

	/** 每次击杀的得分增量 */
	UPROPERTY(EditDefaultsOnly, Category = "Scoring")
	protected int KillScoreIncrement = 1;

	// ========================================
	// 技能配置
	// ========================================

	default ActivationGroup = EYcAbilityActivationGroup::Independent;
	default GamePhaseTag = FGameplayTag::RequestGameplayTag(n"ShooterGame.GamePhase.Playing");

	// ========================================
	// 委托事件
	// ========================================

	/** 倒计时更新事件（每秒触发） */
	UPROPERTY(NotEditable, BlueprintReadOnly)
	FCountDownUpdate OnCountDownUpdateEvent;

	// ========================================
	// 内部变量
	// ========================================

	private UGameplayMessageSubsystem MessageSubsystem;
	private FGameplayMessageListenerHandle EliminationListener;
	private FGameplayMessageListenerHandle AssistListener;
	private FTimerHandle TimerCountdown;

	// ========================================
	// 生命周期函数
	// ========================================

	UFUNCTION(BlueprintOverride)
	void ActivateAbility()
	{
		Log(f"[{GetClass().GetName()}] 游戏对局正式开始");

		// 设置比赛配置到 GameState（供客户端读取）
		SetMatchConfigToGameState();

		// 重置所有玩家
		ResetAllActivePlayers();

		// 启动比赛倒计时
		StartMatchTimer();

		// 注册消息监听（只在 Playing 阶段监听）
		RegisterMessageListeners();
	}

	UFUNCTION(BlueprintOverride)
	void OnEndAbility(bool bWasCancelled)
	{
		Log(f"[{GetClass().GetName()}] 游戏对局阶段结束");

		// 清理资源
		CleanupTimers();
		UnregisterMessageListeners();
	}

	// ========================================
	// 比赛配置同步
	// ========================================

	/**
	 * 将比赛配置设置到 GameState
	 * 这样客户端就可以通过 GameState 读取目标分数和时间限制等信息
	 */
	protected void SetMatchConfigToGameState()
	{
		auto GameState = Cast<AYcGameState>(Gameplay::GetGameState());
		if (GameState == nullptr)
		{
			Warning(f"[{GetClass().GetName()}] 无法获取 GameState");
			return;
		}

		// 设置目标分数
		GameState.SetStatTagStack(GameplayTags::ShooterGame_Match_TargetScore, TargetScore);

		// 设置比赛时长（初始剩余时间）
		GameState.SetStatTagStack(GameplayTags::ShooterGame_Match_RemainingTime, int(MatchDuration));

		Log(f"[{GetClass().GetName()}] 已设置比赛配置: 目标分数={TargetScore}, 时长={MatchDuration}秒");
	}

	// ========================================
	// 比赛倒计时管理
	// ========================================

	/**
	 * 启动比赛倒计时
	 */
	protected void StartMatchTimer()
	{
		if (MatchDuration <= 0)
		{
			Warning(f"[{GetClass().GetName()}] 倒计时时长无效: {MatchDuration}，跳过倒计时");
			return;
		}

		TimerCountdown = System::SetTimer(this, n"OnMatchTimerTick", 1.0f, true);
	}

	/**
	 * 比赛倒计时每秒触发
	 */
	UFUNCTION()
	protected void OnMatchTimerTick()
	{
		MatchDuration -= 1.0f;

		// 更新 GameState 中的剩余时间（供客户端读取）
		auto GameState = Cast<AYcGameState>(Gameplay::GetGameState());
		if (GameState != nullptr)
		{
			GameState.SetStatTagStack(GameplayTags::ShooterGame_Match_RemainingTime, int(MatchDuration));
		}

		// 广播倒计时更新事件
		OnCountDownUpdateEvent.Broadcast(MatchDuration);

		// 倒计时结束
		if (MatchDuration <= 0)
		{
			CleanupTimers();
			OnMatchTimeExpired();
		}
	}

	/**
	 * 比赛时间耗尽时调用
	 * 子类必须实现此方法来处理时间到的逻辑
	 */
	protected void OnMatchTimeExpired()
	{
		Warning(f"[{GetClass().GetName()}] OnMatchTimeExpired() 未被子类实现");
	}

	/**
	 * 清理所有定时器
	 */
	private void CleanupTimers()
	{
		System::ClearAndInvalidateTimerHandle(TimerCountdown);
	}

	// ========================================
	// 消息监听与处理
	// ========================================

	/**
	 * 注册游戏消息监听器
	 */
	private void RegisterMessageListeners()
	{
		MessageSubsystem = UGameplayMessageSubsystem::Get();
		if (MessageSubsystem == nullptr)
		{
			Error(f"[{GetClass().GetName()}] 无法获取 GameplayMessageSubsystem");
			return;
		}

		// 监听击杀消息
		EliminationListener = MessageSubsystem.RegisterListener(
			GameplayTags::Yc_Elimination_Message,
			this,
			n"OnReceivedElimination",
			FYcGameVerbMessage(),
			EGameplayMessageMatch::ExactMatch);

		// 监听助攻消息
		AssistListener = MessageSubsystem.RegisterListener(
			GameplayTags::ShooterGame_Assist_Message,
			this,
			n"OnReceivedAssist",
			FYcGameVerbMessage(),
			EGameplayMessageMatch::ExactMatch);
	}

	/**
	 * 注销游戏消息监听器
	 */
	private void UnregisterMessageListeners()
	{
		EliminationListener.Unregister();
		AssistListener.Unregister();
	}

	/**
	 * 收到击杀消息时的回调
	 */
	UFUNCTION()
	void OnReceivedElimination(FGameplayTag ActualTag, const FYcGameVerbMessage& Payload)
	{
		// 验证消息数据有效性
		if (!ValidateEliminationPayload(Payload))
		{
			return;
		}

		// 处理计分（子类实现具体逻辑）
		ProcessScore(Payload);

		// 检查胜利条件
		if (CheckVictoryCondition())
		{
			TransitionToPostGame();
		}
	}

	/**
	 * 收到助攻消息时的回调
	 */
	UFUNCTION()
	void OnReceivedAssist(FGameplayTag ActualTag, const FYcGameVerbMessage& Payload)
	{
		auto AssistInstigator = Cast<AYcPlayerState>(Payload.Instigator);
		if (AssistInstigator != nullptr)
		{
			AssistInstigator.AddStatTagStack(GameplayTags::ShooterGame_Score_Assists, 1);
		}
	}

	/**
	 * 验证击杀消息负载数据的有效性
	 */
	private bool ValidateEliminationPayload(const FYcGameVerbMessage& Payload) const
	{
		if (Payload.Instigator == nullptr)
		{
			Warning(f"[{GetClass().GetName()}] 击杀消息的 Instigator 为空");
			return false;
		}
		if (Payload.Target == nullptr)
		{
			Warning(f"[{GetClass().GetName()}] 击杀消息的 Target 为空");
			return false;
		}
		return true;
	}

	// ========================================
	// 抽象接口（子类实现）
	// ========================================

	/**
	 * 处理计分逻辑
	 * 子类必须实现此方法来处理团队/个人计分
	 *
	 * @param Payload 击杀消息负载
	 */
	protected void ProcessScore(const FYcGameVerbMessage& Payload)
	{
		Warning(f"[{GetClass().GetName()}] ProcessScore() 未被子类实现");
	}

	/**
	 * 检查胜利条件
	 * 子类必须实现此方法来判断是否达到胜利条件
	 *
	 * @return true 表示达到胜利条件，false 表示继续比赛
	 */
	protected bool CheckVictoryCondition()
	{
		return false;
	}

	// ========================================
	// 阶段切换
	// ========================================

	/**
	 * 切换到赛后阶段
	 * 注意：不在这里激活胜利 UI，而是在 PostGame 阶段激活
	 */
	protected void TransitionToPostGame()
	{
		Log(f"[{GetClass().GetName()}] 比赛结束，切换到赛后阶段");

		// 结束当前阶段，自动切换到下一个阶段（PostGame）
		UYcGamePhaseComponent::GetGamePhaseComponent().StartNextPhase();
	}

	// ========================================
	// 玩家管理
	// ========================================

	/**
	 * 重置所有已激活的玩家
	 * 通过 PlayerState 遍历，避免使用 GetAllActorsOfClass 的性能问题
	 */
	protected void ResetAllActivePlayers()
	{
		auto GameState = Gameplay::GetGameState();
		if (GameState == nullptr)
		{
			Warning(f"[{GetClass().GetName()}] 无法获取 GameState");
			return;
		}

		int ResetCount = 0;
		for (auto& PlayerState : GameState.PlayerArray)
		{
			if (ResetPlayerByPlayerState(PlayerState.Get()))
			{
				ResetCount++;
			}
		}

		Log(f"[{GetClass().GetName()}] 已重置 {ResetCount} 个玩家");
	}

	/**
	 * 通过 PlayerState 重置单个玩家
	 *
	 * @param PlayerState 要重置的玩家状态
	 * @return true 表示重置成功，false 表示跳过
	 */
	private bool ResetPlayerByPlayerState(APlayerState PlayerState)
	{
		if (PlayerState == nullptr)
		{
			return false;
		}

		// 获取玩家的 Pawn
		auto Pawn = PlayerState.GetPawn();
		if (Pawn == nullptr)
		{
			return false;
		}

		auto Character = Cast<AYcCharacter>(Pawn);
		if (Character == nullptr)
		{
			return false;
		}

		// 验证 ASC 和 Controller
		auto ASC = AbilitySystem::GetAbilitySystemComponent(Character);
		if (!IsValid(ASC) || !IsValid(Character.GetController()))
		{
			return false;
		}

		// 跳过正在生成中的玩家
		FGameplayTagContainer OwnedTags;
		ASC.GetOwnedGameplayTags(OwnedTags);
		if (OwnedTags.HasTag(GameplayTags::Status_SpawningIn))
		{
			return false;
		}

		// 发送重置事件
		AbilitySystem::SendGameplayEventToActor(
			Character,
			GameplayTags::GameplayEvent_RequestReset,
			FGameplayEventData());

		return true;
	}

	// ========================================
	// 公共访问器
	// ========================================

	UFUNCTION(BlueprintPure)
	int GetTargetScore() const
	{
		return TargetScore;
	}

	UFUNCTION(BlueprintPure)
	float GetMatchDuration() const
	{
		return MatchDuration;
	}
}
