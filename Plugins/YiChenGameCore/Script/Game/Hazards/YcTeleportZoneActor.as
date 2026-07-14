/**
 * 地图传送区域。
 * 放置在关卡中后，进入触发盒的 Pawn 会立即被传送到预设目标点。
 * 典型用法是把玩家从危险区域、掉落区域或解谜重置点传回指定位置。
 */
class AYcTeleportZoneActor : AActor
{
	UPROPERTY(DefaultComponent, RootComponent)
	USceneComponent SceneRoot;

	UPROPERTY(DefaultComponent, Attach = SceneRoot)
	UBoxComponent TriggerBox;
	default TriggerBox.bHiddenInGame = true;
	default TriggerBox.SetCollisionProfileName(n"Trigger");

	/** 传送目标点。调整该组件的 Transform 即可配置传送落点。 */
	UPROPERTY(DefaultComponent, Attach = SceneRoot)
	USceneComponent TeleportTargetPoint;

	/** 触发盒范围，单位 cm。 */
	UPROPERTY(EditAnywhere, Category = "Teleport Zone")
	FVector TriggerBoxExtent = FVector(500.0f, 500.0f, 100.0f);

	/** 是否只影响玩家控制的 Pawn。 */
	UPROPERTY(EditAnywhere, Category = "Teleport Zone")
	bool bAffectPlayerControlledOnly = true;

	/** 是否同步传送朝向。 */
	UPROPERTY(EditAnywhere, Category = "Teleport Zone")
	bool bApplyTargetRotation = true;

	/** 是否输出调试日志。 */
	UPROPERTY(EditAnywhere, Category = "Teleport Zone|Debug")
	bool bEnableDebugLog = false;

	private TArray<APawn> PendingTeleportPawns;

	UFUNCTION(BlueprintOverride)
	void ConstructionScript()
	{
		TriggerBoxExtent.X = Math::Max(1.0f, TriggerBoxExtent.X);
		TriggerBoxExtent.Y = Math::Max(1.0f, TriggerBoxExtent.Y);
		TriggerBoxExtent.Z = Math::Max(1.0f, TriggerBoxExtent.Z);

		ApplyTriggerBoxExtent();
	}

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		ApplyTriggerBoxExtent();

		if (!HasAuthority())
		{
			return;
		}

		TriggerBox.OnComponentBeginOverlap.AddUFunction(this, n"HandleTriggerBeginOverlap");
		TriggerBox.OnComponentEndOverlap.AddUFunction(this, n"HandleTriggerEndOverlap");
	}

	UFUNCTION(BlueprintOverride)
	void EndPlay(EEndPlayReason Reason)
	{
		if (!HasAuthority())
		{
			return;
		}

		PendingTeleportPawns.Empty();
	}

	private void ApplyTriggerBoxExtent()
	{
		if (TriggerBox == nullptr)
		{
			return;
		}

		TriggerBox.SetBoxExtent(TriggerBoxExtent);
		TriggerBox.SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		TriggerBox.SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		TriggerBox.SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECollisionResponse::ECR_Overlap);
		TriggerBox.SetGenerateOverlapEvents(true);
	}

	UFUNCTION()
	private void HandleTriggerBeginOverlap(UPrimitiveComponent OverlappedComponent, AActor OtherActor, UPrimitiveComponent OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult&in SweepResult)
	{
		if (!HasAuthority())
		{
			return;
		}

		auto VictimPawn = ResolveTargetPawn(OtherActor);
		if (VictimPawn == nullptr || PendingTeleportPawns.Contains(VictimPawn))
		{
			return;
		}

		PendingTeleportPawns.Add(VictimPawn);
		TeleportPawnToTarget(VictimPawn);
	}

	UFUNCTION()
	private void HandleTriggerEndOverlap(UPrimitiveComponent OverlappedComponent, AActor OtherActor, UPrimitiveComponent OtherComp, int32 OtherBodyIndex)
	{
		if (!HasAuthority())
		{
			return;
		}

		auto VictimPawn = Cast<APawn>(OtherActor);
		if (VictimPawn == nullptr)
		{
			return;
		}

		PendingTeleportPawns.Remove(VictimPawn);
	}

	private APawn ResolveTargetPawn(AActor OtherActor) const
	{
		auto Pawn = Cast<APawn>(OtherActor);
		if (Pawn == nullptr)
		{
			return nullptr;
		}

		if (bAffectPlayerControlledOnly && !Pawn.IsPlayerControlled())
		{
			return nullptr;
		}

		return Pawn;
	}

	private void TeleportPawnToTarget(APawn Pawn)
	{
		if (Pawn == nullptr || TeleportTargetPoint == nullptr)
		{
			return;
		}

		FVector TargetLocation = TeleportTargetPoint.WorldLocation;
		FRotator TargetRotation = bApplyTargetRotation ? TeleportTargetPoint.WorldRotation : Pawn.ActorRotation;
		Pawn.SetActorLocationAndRotation(TargetLocation, TargetRotation);

		if (bEnableDebugLog)
		{
			Log(f"[TeleportZone] Teleport pawn={Pawn.GetName()} zone={GetName()} location={TargetLocation}");
		}
	}
}
