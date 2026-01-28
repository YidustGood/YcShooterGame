/**
 * 射击游戏 AI Bot 控制器
 * 
 * 功能说明：
 * - 管理 AI Bot 的行为树执行
 * - 处理 AI 感知系统（视觉、听觉、伤害感知）
 * - 管理武器装备和快捷栏
 * - 响应队伍变更和死亡事件
 * 
 * 使用方式：
 * 1. 创建蓝图子类继承此类
 * 2. 配置行为树资产（BTAsset）
 * 3. 配置 AI 感知组件（主要使用 AIPerceptionComponent）
 * 4. 设置视觉、听觉、伤害感知配置
 * 
 * 注意事项：
 * - 行为树会在 Experience 加载完成后自动启动
 * - Bot 死亡时会自动停止行为树逻辑
 * - 队伍变更时会自动更新 AI 感知的敌友判断
 */
class AYcShooterAIController : AYcAIController
{
	/** 行为树资产，定义 AI Bot 的行为逻辑 */
	UPROPERTY()
	UBehaviorTree BTAsset;

	/** 黑板中存储敌人目标的键名 */
	UPROPERTY()
	FName EnemyNameKey;

	/** Bot 卡住的时间（用于检测 Bot 是否被困住） */
	UPROPERTY()
	float StuckTime;

	default EnemyNameKey = n"TargetEnemy";
	default bReplicateUsingRegisteredSubObjectList = true;

	/** AI 感知组件，用于检测敌人、声音、伤害等 */
	UPROPERTY(DefaultComponent)
	UAIPerceptionComponent AIPerception;

	/** 物品栏管理组件，管理 Bot 的武器和道具 */
	UPROPERTY(DefaultComponent)
	UYcInventoryManagerComponent InventoryManager;

	/** 快捷栏组件，用于快速切换武器 */
	UPROPERTY(DefaultComponent)
	UYcQuickBarComponent QuickBar;

	/** 武器状态组件，跟踪当前武器的状态 */
	UPROPERTY(DefaultComponent)
	UYcWeaponStateComponent WeaponState;

	/**
	 * 游戏开始时调用
	 * 等待 Experience 加载完成后再初始化 AI 系统
	 */
	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		// 等待 Experience 就绪后再初始化 AI
		// 这样可以确保所有游戏系统（武器、能力等）都已加载
		auto WaitForExperienceReady = AsYcGameCoreAsync::WaitForExperienceReady();
		WaitForExperienceReady.OnReady.AddUFunction(this, n"OnReady");
		WaitForExperienceReady.Activate();
	}

	/**
	 * Experience 加载完成后的回调
	 * 初始化行为树和死亡事件监听
	 */
	UFUNCTION()
	private void OnReady()
	{
		// 运行行为树（会自动初始化黑板）
		RunBehaviorTree(BTAsset);

		// 监听 Bot 死亡事件
		auto HealthComp = GetControlledPawn().GetComponentByClass(UYcHealthComponent);
		HealthComp.OnDeathStarted.AddUFunction(this, n"OnDeathStarted");
	}

	/**
	 * Bot 死亡时的回调
	 * 清理黑板数据并停止 AI 逻辑
	 * 
	 * @param OwningActor 死亡的 Actor（Bot 的 Pawn）
	 */
	UFUNCTION()
	private void OnDeathStarted(AActor OwningActor)
	{
		// 清除黑板中的敌人目标
		auto CurrentBlackboard = AIHelper::GetBlackboard(this);
		CurrentBlackboard.ClearValue(EnemyNameKey);
		
		// 停止行为树逻辑
		BrainComponent.StopLogic("Dead");
	}

	/**
	 * 控制器获得 Pawn 控制权时调用
	 * 用于 Bot 生成或重生场景
	 * 
	 * @param PossessedPawn 被控制的 Pawn
	 */
	UFUNCTION(BlueprintOverride)
	void ReceivePossess(APawn PossessedPawn)
	{
		// 启动行为树逻辑（如果之前被停止）
		if (IsValid(BrainComponent))
		{
			BrainComponent.StartLogic();
		}
		
		// 监听队伍变更事件
		// 当 Bot 的队伍发生变化时，需要更新 AI 感知系统的敌友判断
		auto AsyncObserveTeam = AsYcGameCoreAsync::ObserveTeam(this);
		AsyncObserveTeam.OnTeamChanged.AddUFunction(this, n"OnTeamChanged");
	}

	/**
	 * 控制器失去 Pawn 控制权时调用
	 * 用于 Bot 死亡或被销毁场景
	 * 
	 * @param UnpossessedPawn 失去控制的 Pawn
	 */
	UFUNCTION(BlueprintOverride)
	void ReceiveUnPossess(APawn UnpossessedPawn)
	{
		// 执行死亡清理逻辑
		OnDeathStarted(UnpossessedPawn);
	}

	/**
	 * 队伍变更时的回调
	 * 请求 AI 感知系统更新刺激监听器，以便重新判断敌友关系
	 * 
	 * @param bTeamSet 是否成功设置队伍
	 * @param TeamId 新的队伍 ID
	 */
	UFUNCTION()
	void OnTeamChanged(bool bTeamSet, int32 TeamId)
	{
		// 请求更新 AI 感知系统的刺激监听器
		// 这会使队伍变更后的敌友判断立即生效
		AIPerception.RequestStimuliListenerUpdate();
	}
}
