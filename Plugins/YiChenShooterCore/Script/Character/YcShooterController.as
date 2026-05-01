/**
 * 射手角色控制器
 */
class AYcShooterController : AYcPlayerController
{
	UPROPERTY(DefaultComponent)
	UYcQuickBarComponent QuickBarComp;

	UPROPERTY(DefaultComponent)
	UInteractionIndicatorComponent InteractionIndicatorComponent;

	UPROPERTY(DefaultComponent)
	UYcInventoryOperationRouterComponent InventoryOperationRouterComponent;

	default QuickBarComp.bAllowDirectContainerDropToQuickBar = true;
	default QuickBarComp.bItemsLeaveInventory = true;

	UPROPERTY(DefaultComponent)
	UGridInventoryManagerComponent GridInventoryManagerComponent;

	UPROPERTY(DefaultComponent)
	UYcPlayerPersistenceComponent Persistence;

	UPROPERTY(EditAnywhere, Category = "Persistence")
	bool bAutoResolvePersistenceSceneMode = true;

	UPROPERTY(EditAnywhere, Category = "Persistence")
	EYcPlayerPersistenceSceneMode PersistenceSceneMode = EYcPlayerPersistenceSceneMode::InMatch;

	UPROPERTY(EditAnywhere, Category = "Persistence")
	AActor PersistenceStashActor;

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
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
		YcPersistence::RequestAutosave(this, FGameplayTag());
	}

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
