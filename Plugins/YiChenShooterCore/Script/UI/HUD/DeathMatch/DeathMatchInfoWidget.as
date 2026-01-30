/**
 * 死亡竞赛比赛信息 HUD Widget
 *
 * 功能说明：
 * - 中间显示比赛剩余时间（带时间警告效果）和获胜目标分数
 * - 左边显示当前最高击杀数（带玩家名称）
 * - 右边显示自己的击杀数
 * - 所有文本都包含标签名称
 * - 支持动态颜色变化
 * - 时间少于 30 秒时变红色警告
 * - 显示最高击杀玩家的名称
 * - 可配置的时间警告阈值
 */
class UDeathMatchInfoWidget : UUserWidget
{
	// ========================================
	// UI 元素绑定（在 UMG 中绑定）
	// ========================================

	/** 剩余时间文本（中间上） */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock RemainingTimeText;

	/** 获胜目标分数文本（中间下） */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock TargetScoreText;

	/** 最高击杀数文本（左边） */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock TopKillsText;

	/** 最高击杀玩家名称（左边下方，可选） */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	UTextBlock TopPlayerNameText;

	/** 我的击杀数文本（右边） */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
	UTextBlock MyKillsText;

	// ========================================
	// 配置属性
	// ========================================

	/** 更新间隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match Info")
	float UpdateInterval = 0.1f;

	/** 时间警告阈值（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match Info")
	int TimeWarningThreshold = 30;

	/** 是否显示最高击杀玩家名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Match Info")
	bool bShowTopPlayerName = true;

	// ========================================
	// 颜色配置
	// ========================================

	/** 正常时间颜色 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
	FLinearColor NormalTimeColor = FLinearColor(1.0f, 1.0f, 1.0f, 1.0f);

	/** 警告时间颜色 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Colors")
	FLinearColor WarningTimeColor = FLinearColor(1.0f, 0.0f, 0.0f, 1.0f);

	// ========================================
	// 内部变量
	// ========================================

	private float AccumulatedTime = 0.0f;
	private AYcGameState CachedGameState;
	private AYcPlayerState CachedLocalPlayerState;

	// ========================================
	// 生命周期
	// ========================================

	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		CachedGameState = Cast<AYcGameState>(Gameplay::GetGameState());
		CachedLocalPlayerState = GetLocalPlayerState();
		UpdateAllDisplays();
	}

	UFUNCTION(BlueprintOverride)
	void Tick(FGeometry MyGeometry, float InDeltaTime)
	{
		AccumulatedTime += InDeltaTime;

		if (AccumulatedTime >= UpdateInterval)
		{
			AccumulatedTime = 0.0f;
			UpdateAllDisplays();
		}
	}

	// ========================================
	// 更新逻辑
	// ========================================

	private void UpdateAllDisplays()
	{
		if (CachedGameState == nullptr)
		{
			CachedGameState = Cast<AYcGameState>(Gameplay::GetGameState());
		}

		if (CachedLocalPlayerState == nullptr)
		{
			CachedLocalPlayerState = GetLocalPlayerState();
		}

		UpdateRemainingTime();
		UpdateTargetScore();
		UpdateTopKills();
		UpdateMyKills();
	}

	/**
	 * 更新剩余时间显示（带警告效果）
	 */
	private void UpdateRemainingTime()
	{
		if (RemainingTimeText == nullptr || CachedGameState == nullptr)
		{
			return;
		}

		int RemainingSeconds = CachedGameState.GetStatTagStackCount(GameplayTags::ShooterGame_Match_RemainingTime);

		if (RemainingSeconds > 0)
		{
			int Minutes = Math::IntegerDivisionTrunc(RemainingSeconds, 60);
			int Seconds = RemainingSeconds % 60;
			FString TimeString = FormatTime(Minutes, Seconds);
			RemainingTimeText.SetText(FText::FromString(f"剩余时间: {TimeString}"));

			// 时间警告效果
			if (RemainingSeconds <= TimeWarningThreshold)
			{
				RemainingTimeText.SetColorAndOpacity(WarningTimeColor);
			}
			else
			{
				RemainingTimeText.SetColorAndOpacity(NormalTimeColor);
			}
		}
		else
		{
			RemainingTimeText.SetText(FText::FromString("剩余时间: 00:00"));
			RemainingTimeText.SetColorAndOpacity(WarningTimeColor);
		}
	}

	/**
	 * 更新获胜目标分数显示
	 */
	private void UpdateTargetScore()
	{
		if (TargetScoreText == nullptr || CachedGameState == nullptr)
		{
			return;
		}

		int TargetScore = CachedGameState.GetStatTagStackCount(GameplayTags::ShooterGame_Match_TargetScore);

		if (TargetScore > 0)
		{
			TargetScoreText.SetText(FText::FromString(f"获胜分数: {TargetScore}"));
		}
		else
		{
			TargetScoreText.SetText(FText::FromString("获胜分数: --"));
		}
	}

	/**
	 * 更新最高击杀数显示（带玩家名称）
	 */
	private void UpdateTopKills()
	{
		if (TopKillsText == nullptr || CachedGameState == nullptr)
		{
			return;
		}

		// 获取最高击杀数和玩家
		int TopKills = 0;
		AYcPlayerState TopPlayer = GetTopPlayer(TopKills);

		// 显示最高击杀数
		TopKillsText.SetText(FText::FromString(f"最高击杀: {TopKills}"));

		// 显示最高击杀玩家名称（如果启用）
		if (bShowTopPlayerName && TopPlayerNameText != nullptr && TopPlayer != nullptr)
		{
			FString PlayerName = TopPlayer.GetPlayerName();
			TopPlayerNameText.SetText(FText::FromString(PlayerName));
		}
	}

	/**
	 * 更新我的击杀数显示
	 */
	private void UpdateMyKills()
	{
		if (MyKillsText == nullptr || CachedLocalPlayerState == nullptr)
		{
			return;
		}

		int MyKills = CachedLocalPlayerState.GetStatTagStackCount(GameplayTags::ShooterGame_Score_Eliminations);
		MyKillsText.SetText(FText::FromString(f"我的击杀: {MyKills}"));
	}

	// ========================================
	// 辅助函数
	// ========================================

	/**
	 * 获取最高击杀玩家和击杀数
	 */
	private AYcPlayerState GetTopPlayer(int& OutTopKills) const
	{
		OutTopKills = 0;
		AYcPlayerState TopPlayer = nullptr;

		if (CachedGameState == nullptr)
		{
			return nullptr;
		}

		for (auto& PlayerState : CachedGameState.PlayerArray)
		{
			auto YcPS = Cast<AYcPlayerState>(PlayerState.Get());
			if (YcPS == nullptr)
			{
				continue;
			}

			int CurrentKills = YcPS.GetStatTagStackCount(GameplayTags::ShooterGame_Score_Eliminations);
			if (CurrentKills > OutTopKills)
			{
				OutTopKills = CurrentKills;
				TopPlayer = YcPS;
			}
		}

		return TopPlayer;
	}

	private AYcPlayerState GetLocalPlayerState() const
	{
		auto PlayerController = Gameplay::GetPlayerController(0);
		if (PlayerController == nullptr)
		{
			return nullptr;
		}

		return Cast<AYcPlayerState>(PlayerController.PlayerState);
	}

	private FString FormatTime(int Minutes, int Seconds) const
	{
		FString MinutesStr = (Minutes < 10) ? f"0{Minutes}" : f"{Minutes}";
		FString SecondsStr = (Seconds < 10) ? f"0{Seconds}" : f"{Seconds}";
		return f"{MinutesStr}:{SecondsStr}";
	}

	// ========================================
	// 公共接口
	// ========================================

	UFUNCTION(BlueprintCallable, Category = "Match Info")
	void RefreshDisplay()
	{
		UpdateAllDisplays();
	}

	UFUNCTION(BlueprintCallable, Category = "Match Info")
	void SetUpdateInterval(float NewInterval)
	{
		UpdateInterval = Math::Max(0.01f, NewInterval);
	}

	UFUNCTION(BlueprintCallable, Category = "Match Info")
	void SetTimeWarningThreshold(int NewThreshold)
	{
		TimeWarningThreshold = Math::Max(0, NewThreshold);
	}
}
