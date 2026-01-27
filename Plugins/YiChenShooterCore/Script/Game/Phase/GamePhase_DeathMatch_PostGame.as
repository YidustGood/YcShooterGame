/**
 * 个人死斗模式 PostGame 阶段
 *
 * 功能说明：
 * - 显示个人胜利/平局 UI
 * - 传递胜利玩家信息给 GameplayCue
 */
class UGamePhase_DeathMatch_PostGame : UGamePhase_ShooterGame_PostGame
{
	// ========================================
	// 内部状态
	// ========================================

	/** 胜利玩家 PlayerState */
	private APlayerState WinningPlayer;

	// ========================================
	// 生命周期
	// ========================================

	UFUNCTION(BlueprintOverride)
	void ActivateAbility()
	{
		// 从上一个阶段获取胜利信息
		RetrieveMatchResult();

		// 调用基类显示 UI
		Super::ActivateAbility();
	}

	// ========================================
	// 比赛结果处理
	// ========================================

	/**
	 * 从 Playing 阶段获取比赛结果
	 */
	private void RetrieveMatchResult()
	{
		// 找出分数最高的玩家
		int HighestScore = 0;
		auto TopPlayers = GetTopScoringPlayers(HighestScore);

		if (TopPlayers.Num() == 0)
		{
			Warning(f"[{GetClass().GetName()}] 没有找到有效玩家");
			return;
		}

		if (TopPlayers.Num() > 1)
		{
			Log(f"[{GetClass().GetName()}] 检测到平局，{TopPlayers.Num()} 名玩家同分 ({HighestScore} 分)");
		}
		else
		{
			Log(f"[{GetClass().GetName()}] 玩家 {TopPlayers[0].GetPlayerName()} 获得胜利 ({HighestScore} 分)");
		}

		// 取第一个玩家作为胜者
		WinningPlayer = TopPlayers[0];
	}

	/**
	 * 获取分数最高的玩家列表
	 */
	private TArray<APlayerState> GetTopScoringPlayers(int32& OutHighestScore) const
	{
		TArray<APlayerState> TopPlayers;
		int32 HighestScore = 0;

		auto GameState = Gameplay::GetGameState();
		if (GameState == nullptr)
		{
			Warning(f"[{GetClass().GetName()}] 无法获取 GameState");
			OutHighestScore = 0;
			return TopPlayers;
		}

		for (auto& PlayerState : GameState.PlayerArray)
		{
			auto YcPS = Cast<AYcPlayerState>(PlayerState.Get());
			if (YcPS == nullptr)
			{
				continue;
			}

			int32 CurrentScore = YcPS.GetStatTagStackCount(GameplayTags::ShooterGame_Score_Eliminations);

			if (CurrentScore > HighestScore)
			{
				HighestScore = CurrentScore;
				TopPlayers.Empty();
				TopPlayers.Add(YcPS);
			}
			else if (CurrentScore == HighestScore && CurrentScore > 0)
			{
				TopPlayers.Add(YcPS);
			}
		}

		OutHighestScore = HighestScore;
		return TopPlayers;
	}

	/**
	 * 设置比赛结果参数
	 * 通过 Instigator 传递胜利玩家
	 */
	protected void SetMatchResultParameters(FGameplayCueParameters& Parameters) override
	{
		if (WinningPlayer != nullptr)
		{
			Parameters.Instigator = WinningPlayer;
		}
	}

	/**
	 * 获取胜利玩家
	 */
	UFUNCTION(BlueprintPure)
	APlayerState GetWinningPlayer() const
	{
		return WinningPlayer;
	}
}
