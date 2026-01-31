/**
 * 团队死斗模式 PostGame 阶段
 * 
 * 功能说明：
 * - 显示团队胜利/平局 UI
 * - 传递胜利队伍信息给 GameplayCue
 */
class UGamePhase_TeamDeathMatch_PostGame : UGamePhase_ShooterGame_PostGame
{
	// ========================================
	// 内部状态
	// ========================================

	/** 胜利队伍 ID（0 表示平局） */
	private int WinningTeamId = 0;

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
		// 通过查询队伍分数来判定胜负
		auto TeamSubsystem = UYcTeamSubsystem::Get();
		if (TeamSubsystem == nullptr)
		{
			Warning(f"[{GetClass().GetName()}] 无法获取 TeamSubsystem");
			return;
		}

		int Team1Score = TeamSubsystem.GetTeamTagStackCount(0, GameplayTags::ShooterGame_Score_Eliminations);
		int Team2Score = TeamSubsystem.GetTeamTagStackCount(1, GameplayTags::ShooterGame_Score_Eliminations);

		if (Team1Score == Team2Score)
		{
			WinningTeamId = 0; // 平局
			Log(f"[{GetClass().GetName()}] 比赛平局 ({Team1Score} : {Team2Score})");
		}
		else
		{
			WinningTeamId = Team1Score > Team2Score ? 0 : 1;
			Log(f"[{GetClass().GetName()}] 队伍 {WinningTeamId} 获得胜利 ({Team1Score} : {Team2Score})");
		}
	}

	/**
	 * 设置比赛结果参数
	 * 通过 AbilityLevel 传递胜利队伍 ID
	 */
	protected void SetMatchResultParameters(FGameplayCueParameters& Parameters) override
	{
		Parameters.AbilityLevel = WinningTeamId;
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
