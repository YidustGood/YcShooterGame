class AInteractableActorBase : AActor
{
	/** 根场景组件。 */
	UPROPERTY(DefaultComponent, RootComponent)
	USceneComponent SceneRoot;

	/** 交互组件，负责交互配置和焦点事件。 */
	UPROPERTY(DefaultComponent)
	UYcInteractableComponent YcInteractableComp;

	/** 交互物的展示模型组件。 */
	UPROPERTY(DefaultComponent, Attach = SceneRoot)
	UStaticMeshComponent DisplayMeshComp;

	/** 玩家靠近后显示 3D 提示图标的触发范围。 */
	UPROPERTY(DefaultComponent, Attach = SceneRoot)
	USphereComponent WorldInteractHintTriggerComp;
	default WorldInteractHintTriggerComp.bHiddenInGame = true;
	default WorldInteractHintTriggerComp.SetCollisionProfileName(n"Trigger");

	/** 世界空间中的 3D 提示图标组件。 */
	UPROPERTY(DefaultComponent, Attach = SceneRoot)
	UBillboardComponent WorldInteractHintBillboardComp;
	default WorldInteractHintBillboardComp.bHiddenInGame = false;
	default WorldInteractHintBillboardComp.RelativeLocation = FVector(0.0f, 0.0f, 20.0f);
	default WorldInteractHintBillboardComp.RelativeScale3D = FVector(0.25f, 0.25f, 0.25f);
	default WorldInteractHintBillboardComp.bIsScreenSizeScaled = false;

	/** 提示图标相对展示模型世界位置的偏移。 */
	UPROPERTY(EditAnywhere, Category = "Interaction Hint|Visual")
	FVector WorldInteractHintOffset = FVector(0.0f, 0.0f, 20.0f);

	/** 玩家进入该范围后开始显示交互提示图标。 */
	UPROPERTY(EditAnywhere, Category = "Interaction Hint", meta = (ClampMin = "0.0"))
	float WorldInteractHintTriggerRadius = 500.0f;

	/** 触发范围是否跟随展示模型的位置，避免 Actor 根节点与模型位置分离时无法触发。 */
	UPROPERTY(EditAnywhere, Category = "Interaction Hint")
	bool bWorldInteractHintTriggerFollowDisplayMesh = true;

	/** 是否启用世界空间交互提示图标。 */
	UPROPERTY(EditAnywhere, Category = "Interaction Hint")
	bool bEnableWorldInteractHint = true;

	/** 玩家聚焦到交互物时是否隐藏 3D 提示图标。 */
	UPROPERTY(EditAnywhere, Category = "Interaction Hint")
	bool bHideWorldInteractHintWhenFocused = false;

	/** 本地玩家当前是否位于提示触发范围内。 */
	private bool bLocalPlayerInHintRange = false;
	/** 本地玩家当前是否正聚焦在该交互物上。 */
	private bool bLocalPlayerFocusedInteractable = false;

	// 要配置具体的YcInteractableComp.Option,来影响Actor的交互表现
	// YcInteractableComp.Option.InteractionAbilityToGrant - 交互时激活的技能, 可以通过GA组织不同的交互效果
	// YcInteractableComp.Option.InteractionWidgetClass - 视角焦点看向这个交互物时显示的UI类

	UFUNCTION(BlueprintOverride)
	void ConstructionScript()
	{
		WorldInteractHintTriggerRadius = Math::Max(0.0f, WorldInteractHintTriggerRadius);
		ApplyWorldInteractHintTriggerDefaults();
		UpdateWorldInteractHintComponentsWorldTransform();
	}

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		ApplyWorldInteractHintTriggerDefaults();
		UpdateWorldInteractHintComponentsWorldTransform();

		if (YcInteractableComp != nullptr)
		{
			YcInteractableComp.OnPlayerFocusBeginEvent.AddUFunction(this, n"HandlePlayerFocusBegin");
			YcInteractableComp.OnPlayerFocusEndEvent.AddUFunction(this, n"HandlePlayerFocusEnd");
		}

		if (WorldInteractHintTriggerComp != nullptr)
		{
			WorldInteractHintTriggerComp.OnComponentBeginOverlap.AddUFunction(this, n"HandleWorldInteractHintTriggerBeginOverlap");
			WorldInteractHintTriggerComp.OnComponentEndOverlap.AddUFunction(this, n"HandleWorldInteractHintTriggerEndOverlap");
		}

		RefreshWorldInteractHintRangeState();
		RefreshWorldInteractHintVisibility();
		SetActorTickEnabled(bLocalPlayerInHintRange);
	}

	UFUNCTION(BlueprintOverride)
	void Tick(float DeltaSeconds)
	{
		UpdateWorldInteractHintComponentsWorldTransform();
	}

	UFUNCTION(BlueprintOverride)
	void EndPlay(EEndPlayReason Reason)
	{
		if (YcInteractableComp != nullptr)
		{
			YcInteractableComp.OnPlayerFocusBeginEvent.Unbind(this, n"HandlePlayerFocusBegin");
			YcInteractableComp.OnPlayerFocusEndEvent.Unbind(this, n"HandlePlayerFocusEnd");
		}
	}

	UFUNCTION(BlueprintCallable)
	void RefreshWorldInteractHintVisibility()
	{
		const bool bShouldShowHint = bEnableWorldInteractHint && bLocalPlayerInHintRange && (!bHideWorldInteractHintWhenFocused || !bLocalPlayerFocusedInteractable);

		SetActorTickEnabled(bLocalPlayerInHintRange);

		if (WorldInteractHintBillboardComp != nullptr)
		{
			WorldInteractHintBillboardComp.SetHiddenInGame(!bShouldShowHint);
		}
	}

	UFUNCTION(BlueprintCallable)
	void SetWorldInteractHintEnabled(bool bEnabled)
	{
		bEnableWorldInteractHint = bEnabled;
		RefreshWorldInteractHintVisibility();
	}

	private void ApplyWorldInteractHintTriggerDefaults()
	{
		if (WorldInteractHintTriggerComp == nullptr)
		{
			return;
		}

		WorldInteractHintTriggerComp.SetSphereRadius(WorldInteractHintTriggerRadius);
		WorldInteractHintTriggerComp.SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		WorldInteractHintTriggerComp.SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		WorldInteractHintTriggerComp.SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		WorldInteractHintTriggerComp.SetGenerateOverlapEvents(true);
	}

	private void UpdateWorldInteractHintComponentsWorldTransform()
	{
		FVector BaseLocation = ActorLocation;
		if (DisplayMeshComp != nullptr)
		{
			BaseLocation = DisplayMeshComp.WorldLocation;
		}
		else if (SceneRoot != nullptr)
		{
			BaseLocation = SceneRoot.WorldLocation;
		}

		if (WorldInteractHintTriggerComp != nullptr && bWorldInteractHintTriggerFollowDisplayMesh)
		{
			WorldInteractHintTriggerComp.SetWorldLocation(BaseLocation);
		}

		if (WorldInteractHintBillboardComp != nullptr)
		{
			WorldInteractHintBillboardComp.SetWorldLocation(BaseLocation + WorldInteractHintOffset);
			WorldInteractHintBillboardComp.SetWorldRotation(FRotator::ZeroRotator);
		}
	}

	private void RefreshWorldInteractHintRangeState()
	{
		bLocalPlayerInHintRange = false;

		if (WorldInteractHintTriggerComp == nullptr)
		{
			return;
		}

		TArray<AActor> OverlappingActors;
		WorldInteractHintTriggerComp.GetOverlappingActors(OverlappingActors, APawn);

		for (auto OverlappingActor : OverlappingActors)
		{
			if (IsLocalHumanPlayerActor(OverlappingActor))
			{
				bLocalPlayerInHintRange = true;
				break;
			}
		}
	}

	UFUNCTION()
	private void HandleWorldInteractHintTriggerBeginOverlap(UPrimitiveComponent OverlappedComponent, AActor OtherActor, UPrimitiveComponent OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult&in SweepResult)
	{
		if (!IsLocalHumanPlayerActor(OtherActor))
		{
			return;
		}

		bLocalPlayerInHintRange = true;
		SetActorTickEnabled(true);
		RefreshWorldInteractHintVisibility();
	}

	UFUNCTION()
	private void HandleWorldInteractHintTriggerEndOverlap(UPrimitiveComponent OverlappedComponent, AActor OtherActor, UPrimitiveComponent OtherComp, int32 OtherBodyIndex)
	{
		if (!IsLocalHumanPlayerActor(OtherActor))
		{
			return;
		}

		bLocalPlayerInHintRange = false;
		bLocalPlayerFocusedInteractable = false;
		SetActorTickEnabled(false);
		RefreshWorldInteractHintVisibility();
	}

	UFUNCTION()
	private void HandlePlayerFocusBegin(const FYcInteractionQuery&in InteractQuery)
	{
		bLocalPlayerFocusedInteractable = true;
		RefreshWorldInteractHintVisibility();
	}

	UFUNCTION()
	private void HandlePlayerFocusEnd(const FYcInteractionQuery&in InteractQuery)
	{
		bLocalPlayerFocusedInteractable = false;
		RefreshWorldInteractHintVisibility();
	}

	private bool IsLocalHumanPlayerActor(AActor OtherActor) const
	{
		auto Pawn = Cast<APawn>(OtherActor);
		if (Pawn == nullptr)
		{
			return false;
		}

		auto PlayerState = Pawn.PlayerState;
		if (PlayerState == nullptr || PlayerState.IsABot())
		{
			return false;
		}

		auto PlayerController = PlayerState.GetPlayerController();
		return PlayerController != nullptr && PlayerController.IsLocalController();
	}
}
