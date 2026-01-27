/**
 * 游戏对局结束后阶段 GameplayAbility
 * 
 * 功能说明：
 * - 在正式对局结束后激活
 * - 显示比赛结果和统计数据
 * - 等待一段时间后开始新的一局游戏
 * 
 * 执行流程：
 * 1. 显示胜利/平局 UI
 * 2. 应用游戏结束 GameplayEffect（如禁用移动、禁用攻击等）
 * 3. 等待指定时间（NewGameTime）
 * 4. 清除胜利 UI
 * 5. 应用新游戏开始 GameplayEffect
 * 6. 请求开始新的一局游戏
 */
class UGamePhase_ShooterGame_PostGame : UYcGamePhaseAbility
{
	// ========================================
	// 配置属性
	// ========================================
	
	/** 等待多久后开始新的一局游戏（秒） */
	UPROPERTY()
	float NewGameTime = 10.0f;

	/** 游戏结束后要应用的 GameplayEffect 列表 */
	UPROPERTY()
	TArray<TSubclassOf<UGameplayEffect>> GameEndEffects;

	/** 请求开始新游戏时要应用的 GameplayEffect 列表 */
	UPROPERTY()
	TArray<TSubclassOf<UGameplayEffect>> NewGameStartEffects;

	// ========================================
	// 技能配置
	// ========================================
	
	default GamePhaseTag = FGameplayTag::RequestGameplayTag(n"ShooterGame.GamePhase.PostGame");
	default ActivationGroup = EYcAbilityActivationGroup::Independent;

	// ========================================
	// 内部变量
	// ========================================

	/** 当前激活的用户提示 Cue */
	private FGameplayTag ActiveUserFacingCue;

	// ========================================
	// 生命周期函数
	// ========================================
	
	UFUNCTION(BlueprintOverride)
	void ActivateAbility()
	{
		Log(f"[{GetClass().GetName()}] 进入赛后阶段，{NewGameTime} 秒后将开始新的一局");

		// 显示胜利/平局 UI
		ShowMatchResultUI();

		// 应用游戏结束 GameplayEffect
		ApplyGameEndEffects();

		// 等待指定时间后开始新游戏
		if (NewGameTime > 0)
		{
			DelayForAs(n"PrepareNewGame", NewGameTime);
		}
		else
		{
			PrepareNewGame();
		}
	}

	UFUNCTION(BlueprintOverride)
	void OnEndAbility(bool bWasCancelled)
	{
		// 清除胜利 UI
		ClearMatchResultUI();
		
		Log(f"[{GetClass().GetName()}] 赛后阶段结束");
	}

	// ========================================
	// UI 管理
	// ========================================

	/**
	 * 显示比赛结果 UI
	 * 子类可以覆写此方法来显示不同的 UI
	 */
	protected void ShowMatchResultUI()
	{
		FGameplayCueParameters Parameters;
		
		// 子类可以通过覆写此方法来设置不同的参数
		// 例如：团队模式设置 AbilityLevel 为胜利队伍 ID
		//      个人模式设置 Instigator 为胜利玩家
		SetMatchResultParameters(Parameters);
		
		ActivateUserFacingCue(
			GameplayTags::GameplayCue_ShooterGame_UserMessage_MatchDecided,
			Parameters);
	}

	/**
	 * 设置比赛结果参数
	 * 子类覆写此方法来传递胜利信息
	 */
	protected void SetMatchResultParameters(FGameplayCueParameters& Parameters)
	{
		// 基类不设置参数，子类实现
	}

	/**
	 * 激活用户提示 Cue
	 */
	private void ActivateUserFacingCue(FGameplayTag GameplayCueTag, FGameplayCueParameters Parameters)
	{
		ClearMatchResultUI();
		ActiveUserFacingCue = GameplayCueTag;
		GameplayCue::AddGameplayCueOnActor(Gameplay::GetGameState(), ActiveUserFacingCue, Parameters);
	}

	/**
	 * 清除比赛结果 UI
	 */
	private void ClearMatchResultUI()
	{
		if (!GameplayTag::IsGameplayTagValid(ActiveUserFacingCue))
		{
			return;
		}

		FGameplayCueParameters Parameters;
		GameplayCue::RemoveGameplayCueOnActor(Gameplay::GetGameState(), ActiveUserFacingCue, Parameters);
		ActiveUserFacingCue = FGameplayTag();
	}

	// ========================================
	// 游戏结束处理
	// ========================================
	
	/**
	 * 应用游戏结束 GameplayEffect
	 * 例如：禁用移动、禁用攻击、显示结算UI等
	 */
	private void ApplyGameEndEffects()
	{
		if (GameEndEffects.Num() == 0)
		{
			return;
		}

		auto GlobalAbilitySystem = UYcGlobalAbilitySystem::Get();
		if (GlobalAbilitySystem == nullptr)
		{
			Warning(f"[{GetClass().GetName()}] 无法获取 GlobalAbilitySystem");
			return;
		}

		for (auto Effect : GameEndEffects)
		{
			if (Effect != nullptr)
			{
				GlobalAbilitySystem.ApplyEffectToAll(Effect);
			}
		}

		Log(f"[{GetClass().GetName()}] 已应用 {GameEndEffects.Num()} 个游戏结束效果");
	}

	/**
	 * 准备开始新游戏
	 * 应用新游戏开始 GameplayEffect 并请求开始新游戏
	 */
	UFUNCTION()
	private void PrepareNewGame()
	{
		Log(f"[{GetClass().GetName()}] 准备开始新的一局游戏");

		// 应用新游戏开始 GameplayEffect
		ApplyNewGameStartEffects();

		// 延迟一帧后开始新游戏（确保 Effect 已应用）
		DelayUntilNextTickForAs(n"StartNewGame");
	}

	/**
	 * 应用新游戏开始 GameplayEffect
	 * 例如：移除游戏结束效果、重置玩家状态等
	 */
	private void ApplyNewGameStartEffects()
	{
		if (NewGameStartEffects.Num() == 0)
		{
			return;
		}

		auto GlobalAbilitySystem = UYcGlobalAbilitySystem::Get();
		if (GlobalAbilitySystem == nullptr)
		{
			Warning(f"[{GetClass().GetName()}] 无法获取 GlobalAbilitySystem");
			return;
		}

		for (auto Effect : NewGameStartEffects)
		{
			if (Effect != nullptr)
			{
				GlobalAbilitySystem.ApplyEffectToAll(Effect);
			}
		}

		Log(f"[{GetClass().GetName()}] 已应用 {NewGameStartEffects.Num()} 个新游戏开始效果");
	}

	/**
	 * 开始新的一局游戏
	 */
	UFUNCTION()
	private void StartNewGame()
	{
		Log(f"[{GetClass().GetName()}] 请求开始新的一局游戏");

		// 调用游戏系统开始新游戏
		YcGameSystem::PlayNextGame();

		// 结束当前技能
		EndAbility();
	}
}
