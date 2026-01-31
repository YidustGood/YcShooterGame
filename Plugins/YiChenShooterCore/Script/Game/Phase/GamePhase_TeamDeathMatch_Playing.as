/**
 * 团队死斗模式 Playing 阶段
 * 
 * 功能说明：
 * - 基于团队击杀数判定胜负
 * - 支持目标分数和时间限制两种胜利条件
 * 
 * 胜利条件：
 * 1. 任意队伍达到目标击杀数
 * 2. 比赛时间耗尽，取最高分队伍
 */
class UGamePhase_TeamDeathMatch_Playing : UGamePhase_ShooterGame_Playing
{
	// ========================================
	// 常量定义
	// ========================================
	
	/** 最大队伍数量 */
	const int MAX_TEAMS = 2;

	// ========================================
	// 内部状态
	// ========================================
	
	/** 胜利队伍 ID（0 表示平局） */
	private int WinningTeamId = 0;

	// ========================================
	// 计分逻辑实现
	// ========================================
	
	/**
	 * 处理团队和个人计分
	 */
	protected void ProcessScore(const FYcGameVerbMessage& Payload) override
	{
		// 处理团队得分
		ProcessTeamScore(Payload);
		
		// 处理个人得分
		ProcessIndividualScore(Payload);
	}

	/**
	 * 处理团队得分
	 */
	private void ProcessTeamScore(const FYcGameVerbMessage& Payload)
	{
		auto TeamSubsystem = UYcTeamSubsystem::Get();
		if (TeamSubsystem == nullptr)
		{
			Warning(f"[{GetClass().GetName()}] 无法获取 TeamSubsystem");
			return;
		}

		int TeamIdA = -1;
		int TeamIdB = -1;
		auto CompareResult = TeamSubsystem.CompareTeamsOut(Payload.Instigator, Payload.Target, TeamIdA, TeamIdB);

		// 只有不同队伍击杀才计分
		if (CompareResult == EYcTeamComparison::DifferentTeams)
		{
			TeamSubsystem.AddTeamTagStack(TeamIdA, GameplayTags::ShooterGame_Score_Eliminations, KillScoreIncrement);
			Log(f"[{GetClass().GetName()}] 队伍 {TeamIdA} 获得 {KillScoreIncrement} 分");
		}
	}

	/**
	 * 处理个人得分
	 */
	private void ProcessIndividualScore(const FYcGameVerbMessage& Payload)
	{
		auto Attacker = Cast<AYcPlayerState>(Payload.Instigator);
		auto Victim = Cast<AYcPlayerState>(Payload.Target);

		// 增加攻击者击杀数（排除自杀和友军击杀）
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
	 * 检查是否有队伍达到胜利条件
	 */
	protected bool CheckVictoryCondition() override
	{
		auto TeamSubsystem = UYcTeamSubsystem::Get();
		if (TeamSubsystem == nullptr)
		{
			return false;
		}

		// 检查所有队伍是否有达到目标分数的
		for (int TeamId = 0; TeamId < MAX_TEAMS; ++TeamId)
		{
			int CurrentScore = TeamSubsystem.GetTeamTagStackCount(TeamId, GameplayTags::ShooterGame_Score_Eliminations);
			
			if (CurrentScore >= TargetScore)
			{
				WinningTeamId = TeamId;
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
		auto TeamSubsystem = UYcTeamSubsystem::Get();
		if (TeamSubsystem == nullptr)
		{
			Warning(f"[{GetClass().GetName()}] 无法获取 TeamSubsystem");
			TransitionToPostGame();
			return;
		}

		// 获取两队得分
		int Team1Score = TeamSubsystem.GetTeamTagStackCount(0, GameplayTags::ShooterGame_Score_Eliminations);
		int Team2Score = TeamSubsystem.GetTeamTagStackCount(1, GameplayTags::ShooterGame_Score_Eliminations);

		// 根据得分判定胜负
		if (Team1Score == Team2Score)
		{
			WinningTeamId = 0; // 平局
			Log(f"[{GetClass().GetName()}] 比赛平局");
		}
		else
		{
			WinningTeamId = Team1Score > Team2Score ? 0 : 1;
			Log(f"[{GetClass().GetName()}] 队伍 {WinningTeamId} 获得胜利");
		}

		BroadcastVictoryMessage();
		TransitionToPostGame();
	}

	// ========================================
	// 辅助函数
	// ========================================
	
	/**
	 * 广播胜利消息
	 * 注意：不在这里激活 UI，而是传递胜利信息给 PostGame 阶段
	 */
	private void BroadcastVictoryMessage()
	{
		// 胜利 UI 将在 PostGame 阶段显示
		// 这里只记录胜利信息
	}

	/**
	 * 获取指定队伍的得分
	 * @param TeamId 队伍 ID
	 * @param OutCurrentScore 输出当前得分
	 * @param bOutVictory 输出是否达到胜利条件
	 */
	UFUNCTION(BlueprintCallable)
	void GetTeamScore(int TeamId, int&out OutCurrentScore, bool&out bOutVictory)
	{
		auto TeamSubsystem = UYcTeamSubsystem::Get();
		if (TeamSubsystem == nullptr)
		{
			Warning(f"[{GetClass().GetName()}] 无法获取 TeamSubsystem");
			OutCurrentScore = 0;
			bOutVictory = false;
			return;
		}

		OutCurrentScore = TeamSubsystem.GetTeamTagStackCount(TeamId, GameplayTags::ShooterGame_Score_Eliminations);
		bOutVictory = OutCurrentScore >= TargetScore;
	}

	/**
	 * 获取胜利队伍 ID
	 */
	UFUNCTION(BlueprintPure)
	int GetWinningTeamId() const
	{
		return WinningTeamId;
	}
}
