/**
 * Shooter 玩家控制器：
 * 负责快捷栏、背包交互、持久化流程以及玩家界面组件的挂载与初始化。
 */
class AYcShooterController : AYcPlayerController
{
	// 快捷栏组件，负责当前装备栏位与快捷物品切换。
	UPROPERTY(DefaultComponent)
	UYcQuickBarComponent QuickBarComp;

	// 交互提示组件，用于显示可交互目标的提示信息。
	UPROPERTY(DefaultComponent)
	UInteractionIndicatorComponent InteractionIndicatorComponent;

	// 背包操作路由组件，负责转发和整理背包相关操作请求。
	UPROPERTY(DefaultComponent)
	UYcInventoryOperationRouterComponent InventoryOperationRouterComponent;

	// 允许容器中的物品直接拖放到快捷栏。
	default QuickBarComp.bAllowDirectContainerDropToQuickBar = true;
	// 放入快捷栏后物品会离开原背包位置。
	default QuickBarComp.bItemsLeaveInventory = true;

	// 网格背包管理组件，负责玩家背包容器与物品管理。
	UPROPERTY(DefaultComponent)
	UGridInventoryManagerComponent GridInventoryManagerComponent;

	// 玩家持久化组件，负责局内外数据读写与结算流程。
	UPROPERTY(DefaultComponent)
	UYcPlayerPersistenceComponent Persistence;

	// 玩家屏幕 UI 组件，负责运行时玩家界面的管理与显示。
	UPROPERTY(DefaultComponent)
	UYcPlayerScreenUIComponent PlayerScreenUIComponent;

	// 是否根据当前场景自动判断持久化模式（局内 / 局外 / 禁用）。
	UPROPERTY(EditAnywhere, Category = "Persistence")
	bool bAutoResolvePersistenceSceneMode = true;

	// 当关闭自动判断时，使用这里配置的持久化场景模式。
	UPROPERTY(EditAnywhere, Category = "Persistence")
	EYcPlayerPersistenceSceneMode PersistenceSceneMode = EYcPlayerPersistenceSceneMode::InMatch;

	// @TODO: 目前是较直接的局内 / 局外判定与仓库 Actor 引用方式，后续可进一步收敛成更稳妥的统一逻辑。
	UPROPERTY(EditAnywhere, Category = "Persistence")
	AActor PersistenceStashActor;

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		// 开局时解析持久化场景模式，并完成仓库引用与持久化流程初始化。
		EYcPlayerPersistenceSceneMode ResolvedSceneMode = ResolvePersistenceSceneMode();
		Persistence.SetSceneMode(ResolvedSceneMode);
		Print("[Persistence] Controller=" + GetName() + " ResolvedSceneMode=" + SceneModeToString(ResolvedSceneMode) + " AutoResolve=" + (bAutoResolvePersistenceSceneMode ? "true" : "false"));

		if (ResolvedSceneMode == EYcPlayerPersistenceSceneMode::OutOfMatch)
		{
			if (PersistenceStashActor == nullptr)
			{
				PersistenceStashActor = Gameplay::GetActorOfClass(AGridStashContainer);
			}
			if (PersistenceStashActor == nullptr)
			{
				Warning("[Persistence] Resolved OutOfMatch but no AGridStashContainer was found.");
			}
			Persistence.SetOutOfMatchStashActor(PersistenceStashActor);
		}
		else
		{
			Persistence.SetOutOfMatchStashActor(nullptr);
		}

		Persistence.InitializePersistenceFlow();
	}

	UFUNCTION(BlueprintOverride)
	void EndPlay(EEndPlayReason EndPlayReason)
	{
		// 控制器结束时触发一次自动保存，避免玩家离开时丢失持久化数据。
		YcPersistence::RequestAutosave(this, FGameplayTag());
	}

	// 根据配置和场景内是否存在仓库容器，推断当前应该使用的持久化模式。
	private EYcPlayerPersistenceSceneMode ResolvePersistenceSceneMode() const
	{
		if (!bAutoResolvePersistenceSceneMode)
		{
			return PersistenceSceneMode;
		}

		AActor ResolvedStashActor = PersistenceStashActor;
		if (ResolvedStashActor == nullptr)
		{
			ResolvedStashActor = Gameplay::GetActorOfClass(AGridStashContainer);
		}

		if (ResolvedStashActor != nullptr)
		{
			return EYcPlayerPersistenceSceneMode::OutOfMatch;
		}

		return EYcPlayerPersistenceSceneMode::InMatch;
	}

	// 将持久化场景模式转成便于日志输出的字符串。
	private FString SceneModeToString(EYcPlayerPersistenceSceneMode SceneMode) const
	{
		if (SceneMode == EYcPlayerPersistenceSceneMode::Disabled)
		{
			return "Disabled";
		}
		if (SceneMode == EYcPlayerPersistenceSceneMode::OutOfMatch)
		{
			return "OutOfMatch";
		}
		return "InMatch";
	}
}
