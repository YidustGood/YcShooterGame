/**
 * 地图死亡区域。
 * 放置在关卡中后，进入触发盒的 Pawn 会立即受到满额自毁伤害。
 * 典型用法是放在地图下方，角色掉出地图时直接死亡。
 */
class AYcDeathZoneActor : AActor
{
	UPROPERTY(DefaultComponent, RootComponent)
	USceneComponent SceneRoot;

	UPROPERTY(DefaultComponent, Attach = SceneRoot)
	UBoxComponent TriggerBox;
	default TriggerBox.bHiddenInGame = true;
	default TriggerBox.SetCollisionProfileName(n"Trigger");

	/** 触发盒范围，单位 cm。 */
	UPROPERTY(EditAnywhere, Category = "Death Zone")
	FVector TriggerBoxExtent = FVector(500.0f, 500.0f, 100.0f);

	/** 是否只影响玩家控制的 Pawn。 */
	UPROPERTY(EditAnywhere, Category = "Death Zone")
	bool bAffectPlayerControlledOnly = true;

	/** 是否输出调试日志。 */
	UPROPERTY(EditAnywhere, Category = "Death Zone|Debug")
	bool bEnableDebugLog = false;

	private TArray<APawn> PendingVictims;

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

		PendingVictims.Empty();
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

		auto VictimPawn = ResolveVictimPawn(OtherActor);
		if (VictimPawn == nullptr || PendingVictims.Contains(VictimPawn))
		{
			return;
		}

		auto HealthComp = UYcHealthComponent::FindHealthComponent(VictimPawn);
		if (HealthComp == nullptr || HealthComp.IsDeadOrDying())
		{
			return;
		}

		PendingVictims.Add(VictimPawn);
		HealthComp.DamageSelfDestruct(true);

		if (bEnableDebugLog)
		{
			Log(f"[DeathZone] Kill victim={VictimPawn.GetName()} zone={GetName()}");
		}
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

		PendingVictims.Remove(VictimPawn);
	}

	private APawn ResolveVictimPawn(AActor OtherActor) const
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
}
