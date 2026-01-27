/**
 * 个人死斗模式 Playing 阶段
 *
 * 功能说明：
 * - 基于个人击杀数判定胜负
 * - 支持目标分数和时间限制两种胜利条件
 *
 * 胜利条件：
 * 1. 任意玩家达到目标击杀数
 * 2. 比赛时间耗尽，取最高分玩家
 */
class UGamePhase_DeathMatch_Playing : UGamePhase_ShooterGame_Playing
{
	// ========================================
	// 内部状态
	// ========================================

	/** 胜利玩家 PlayerState */
	private APlayerState WinningPlayer;

	// ========================================
	// 计分逻辑实现
	// ========================================

	/**
	 * 处理个人计分（不处理团队计分）
	 */
	protected void ProcessScore(const FYcGameVerbMessage& Payload) override
	{
		auto Attacker = Cast<AYcPlayerState>(Payload.Instigator);
		auto Victim = Cast<AYcPlayerState>(Payload.Target);

		// 增加攻击者击杀数（排除自杀）
		if (Attacker != nullptr && Attacker != Victim)
		{
			Attacker.AddStatTagStack(GameplayTags::ShooterGame_Score_Eliminations, 1);
		}

		// 增加受害者死亡数
		if (Victim != nullptr)
		{
			Victim.AddStatTagStack(GameplayTags::ShooterGame_Score_Deaths, 1);
		}
	}

	// ========================================
	// 胜利判定实现
	// ========================================

	/**
	 * 检查是否有玩家达到胜利条件
	 */
	protected bool CheckVictoryCondition() override
	{
		auto GameState = Gameplay::GetGameState();
		if (GameState == nullptr)
		{
			return false;
		}

		// 遍历所有玩家，检查是否有人达到目标分数
		for (auto& PlayerState : GameState.PlayerArray)
		{
			auto YcPS = Cast<AYcPlayerState>(PlayerState.Get());
			if (YcPS == nullptr)
			{
				continue;
			}

			int Score = YcPS.GetStatTagStackCount(GameplayTags::ShooterGame_Score_Eliminations);
			if (Score >= TargetScore)
			{
				WinningPlayer = YcPS;
				BroadcastVictoryMessage();
				return true;
			}
		}

		return false;
	}

	/**
	 * 比赛时间耗尽时的处理
	 */
	protected void OnMatchTimeExpired() override
	{
		// 获取分数最高的玩家
		int HighestScore = 0;
		auto TopPlayers = GetTopScoringPlayers(HighestScore);

		if (TopPlayers.Num() == 0)
		{
			Warning(f"[{GetClass().GetName()}] 比赛结束但没有找到有效玩家");
			BroadcastDrawMessage();
			TransitionToPostGame();
			return;
		}

		// 处理平局或胜利
		if (TopPlayers.Num() > 1)
		{
			Log(f"[{GetClass().GetName()}] 检测到平局，{TopPlayers.Num()} 名玩家同分 ({HighestScore} 分)");
			// 暂时取第一个玩家作为胜者（可以扩展为加时赛等逻辑）
			WinningPlayer = TopPlayers[0];
		}
		else
		{
			WinningPlayer = TopPlayers[0];
			Log(f"[{GetClass().GetName()}] 玩家 {WinningPlayer.GetPlayerName()} 获得胜利 ({HighestScore} 分)");
		}

		BroadcastVictoryMessage();
		TransitionToPostGame();
	}

	// ========================================
	// 辅助函数
	// ========================================

	/**
	 * 获取分数最高的玩家列表
	 * @param OutHighestScore 输出最高分数
	 * @return 分数最高的玩家列表（可能有多个同分玩家）
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
	 * 广播胜利消息
	 * 注意：胜利 UI 将在 PostGame 阶段显示
	 */
	private void BroadcastVictoryMessage()
	{
		// 胜利信息已记录在 WinningPlayer 中
		// PostGame 阶段会读取并显示
	}

	/**
	 * 广播平局消息
	 * 注意：平局 UI 将在 PostGame 阶段显示
	 */
	private void BroadcastDrawMessage()
	{
		// PostGame 阶段会自动判定平局并显示
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
