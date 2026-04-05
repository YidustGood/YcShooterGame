/**
 * 近战武器攻击射线检测形状枚举
 */
enum EYcKnifeAttackTraceShape
{
	Line,
	Sphere,
	Capsule,
	Box
}

/**
 * 近战武器攻击GA
 * 需要为刀模型添加以AttackPoint为前缀的Socket,GA会自动获取到所有的AttackPoint, 例如{AttackPoint1, AttackPoint2, ...}
 * 攻击时会在所有的AttackPoint上进行射线检测
 */
class UYcGameplayAbility_KnifeAttack : UYcGameplayAbility_WeaponBase
{
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> DamageGE;

	UPROPERTY()
	EDrawDebugTrace DrawDebugTrace;

	UPROPERTY()
	float AttackCheckInterval = 0.1f;

	UPROPERTY(EditDefaultsOnly)
	EYcKnifeAttackTraceShape TraceShape;
	default TraceShape = EYcKnifeAttackTraceShape::Line;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "TraceShape == EYcKnifeAttackTraceShape::Capsule", ClampMin = "0.0"))
	float TraceCapsuleRadius = 4.0f;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "TraceShape == EYcKnifeAttackTraceShape::Capsule", ClampMin = "0.0"))
	float TraceCapsuleHalfHeight = 8.0f;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "TraceShape == EYcKnifeAttackTraceShape::Sphere", ClampMin = "0.0"))
	float TraceSphereRadius = 6.0f;

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "TraceShape == EYcKnifeAttackTraceShape::Box"))
	FVector TraceBoxHalfSize = FVector(4.0f, 4.0f, 4.0f);

	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "TraceShape == EYcKnifeAttackTraceShape::Box"))
	FRotator TraceBoxOrientation = FRotator();

	UPROPERTY()
	bool bLogHitActors;

	// 添加EventTrigger, 便于AI等系统触发射击能力
	// @TODO 目前近战武器攻击输入Tag复用的武器开火, 如果近战需要有不同的输入Tag, 则需要在GA中添加不同的输入Tag
	FAbilityTriggerData TriggerData;
	default TriggerData.TriggerTag = GameplayTags::InputTag_Weapon_Fire;
	default TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	default AbilityTriggers.Add(TriggerData);

	USkeletalMeshComponent ArmsMesh;
	USkeletalMeshComponent KnifeMesh;
	TArray<FName> KnifeSocketName;
	TArray<FVector> TracePoints;
	FTimerHandle AttackTimer;
	FYcWeaponActionVisual MeleeAction;

	UFUNCTION(BlueprintOverride)
	void OnAbilityAdded()
	{
		Super::OnAbilityAdded();

		if (!IsLocallyControlled())
			return;

		auto Avatar = GetAvatarActorFromActorInfo();
		ArmsMesh = Avatar.GetComponentsByTag(USkeletalMeshComponent, n"FPCharacter")[0];

		WeaponVisualDataCache.GetActionVisual(GameplayTags::Asset_Weapon_Action_Melee_Light, MeleeAction);
	}

	void UpdateTracePoints()
	{
		TracePoints.Empty();
		for (auto& SocketName : KnifeSocketName)
		{
			FVector SocketLocation = KnifeMesh.GetSocketLocation(SocketName);
			TracePoints.Add(SocketLocation);
		}
	}

	UFUNCTION(BlueprintOverride)
	void ActivateAbility()
	{
		if (!IsLocallyControlled())
			return;

		// 初始化刀模型
		if (KnifeMesh == nullptr)
		{
			TArray<USkeletalMeshComponent> Children = ArmsMesh.GetChildrenComponentsByClass(USkeletalMeshComponent, true);
			if (Children.Num() > 0)
			{
				KnifeMesh = Children[0];
				InitAttackPointsName();
			}
		}
		PlayAttachAnim();
	}

	void PlayAttachAnim()
	{
		auto PlayMontageCallbackProxy = ArmsMesh.PlayMontage(MeleeAction.FPCharacterAnimMontage.Get());
		PlayMontageCallbackProxy.OnCompleted.AddUFunction(this, n"OnAttackDone");
		PlayMontageCallbackProxy.OnBlendOut.AddUFunction(this, n"OnAttackDone");
		PlayMontageCallbackProxy.OnInterrupted.AddUFunction(this, n"OnAttackDone");
		PlayMontageCallbackProxy.OnNotifyBegin.AddUFunction(this, n"StartKnifeAttack");
		PlayMontageCallbackProxy.OnNotifyEnd.AddUFunction(this, n"EndKnifeAttack");

		// //@TODO 加入刀在攻击时本体需要有动画就在这里发送给刀Actor处理, 具体动画干脆由具体Actor选择, FP和TP是不一样的
		// // 通知FP刀Actor执行检视动画
		// FGameplayEventData Payload;
		// Payload.EventTag = GameplayTags::InputTag_Weapon_Inspect;
		// Payload.OptionalObject = InspectActionVisual.FPWeaponAnimMontage.Get();
		// AbilitySystem::SendGameplayEventToActor(GetAvatarActorFromActorInfo(), GameplayTags::InputTag_Weapon_Inspect, Payload);
	}

	UFUNCTION()
	void OnAttackDone(FName NotifyName)
	{
		EndAbility();
	}

	// 从刀骨骼网格体中提取所有AttackPoint
	void InitAttackPointsName()
	{
		KnifeSocketName.Empty();
		TArray<FName> AttackPoints = KnifeMesh.GetAllSocketNames();
		for (auto& SocketName : AttackPoints)
		{
			if (SocketName.ToString().StartsWith("AttackPoint"))
				KnifeSocketName.Add(SocketName);
		}
	}

	UFUNCTION(Server)
	void ServerSetUniqueHits(TArray<FHitResult>& Hits)
	{
		UniqueHits = Hits;
		ProcessHits();
	}

	// 攻击蒙太奇通知开始时调用
	UFUNCTION()
	void StartKnifeAttack(FName NotifyName)
	{
		if (!IsLocallyControlled())
			return;
		ActorHited.Empty();
		UniqueHits.Empty();

		UpdateTracePoints();
		DoTrace();
		AttackTimer = System::SetTimer(this, n"DoTrace", AttackCheckInterval, true);
	}

	// 攻击蒙太奇通知结束时调用
	UFUNCTION()
	void EndKnifeAttack(FName NotifyName)
	{
		if (!IsLocallyControlled())
			return;
		System::ClearAndInvalidateTimerHandle(AttackTimer);
		ServerSetUniqueHits(UniqueHits);
	}

	// UPROPERTY(ReplicatedUsing = OnRep_UniqueHits)
	TArray<FHitResult> UniqueHits;
	TMap<AActor, int> ActorHited;

	// 攻击蒙太奇定时器回调
	UFUNCTION()
	void DoTrace()
	{
		for (int i = 0; i < KnifeSocketName.Num(); i++)
		{
			FVector EndPos = KnifeMesh.GetSocketLocation(KnifeSocketName[i]);
			TArray<FHitResult> OutHits;
			TArray<EObjectTypeQuery> ObjectTypes;
			ObjectTypes.Add(EObjectTypeQuery::Pawn);
			TArray<AActor> ActorsToIgnore;
			ActorsToIgnore.Add(GetAvatarActorFromActorInfo());

			if (TraceShape == EYcKnifeAttackTraceShape::Sphere)
			{
				System::SphereTraceMultiForObjects(TracePoints[i],
												   EndPos,
												   TraceSphereRadius,
												   ObjectTypes,
												   true,
												   ActorsToIgnore,
												   DrawDebugTrace,
												   OutHits,
												   true);
			}
			else if (TraceShape == EYcKnifeAttackTraceShape::Capsule)
			{
				System::CapsuleTraceMultiForObjects(TracePoints[i],
													EndPos,
													TraceCapsuleRadius,
													TraceCapsuleHalfHeight,
													ObjectTypes,
													true,
													ActorsToIgnore,
													DrawDebugTrace,
													OutHits,
													true);
			}
			else if (TraceShape == EYcKnifeAttackTraceShape::Box)
			{
				System::BoxTraceMultiForObjects(TracePoints[i],
												EndPos,
												TraceBoxHalfSize,
												TraceBoxOrientation,
												ObjectTypes,
												true,
												ActorsToIgnore,
												DrawDebugTrace,
												OutHits,
												true);
			}
			else
			{
				System::LineTraceMultiForObjects(TracePoints[i],
												 EndPos,
												 ObjectTypes,
												 true,
												 ActorsToIgnore,
												 DrawDebugTrace,
												 OutHits,
												 true);
			}

			// 添加唯一的命中结果,通过引用避免复制
			for (auto& Hit : OutHits)
			{
				if (!Hit.bBlockingHit)
					continue;
				if (ActorHited.Contains(Hit.Actor))
					continue;
				UniqueHits.Add(Hit);
				ActorHited.Add(Hit.Actor, 1);
			}
		}
		UpdateTracePoints();
	}

	UFUNCTION()
	void ProcessHits()
	{
		UAbilitySystemComponent ASC = GetAbilitySystemComponentFromActorInfo();
		for (auto& Hit : UniqueHits)
		{
			if (bLogHitActors)
			{
				Print("KnifeAttack: " + Hit.Actor.ToString());
			}

			// 向目标ASC应用伤害GE
			UAbilitySystemComponent TargetASC = UYcAbilitySystemComponent::GetAbilitySystemComponentFromActor(Hit.Actor);
			if (TargetASC == nullptr)
				continue;
			FGameplayEffectContextHandle Context;
			if (DamageGE == nullptr)
			{
				Warning("UYcGameplayAbility_KnifeAttack: DamageGE is nullptr, cannot apply damage!");
				continue;
			}
			float BaseDamage = AssociatedItem.GetFloatTagStackValue(GameplayTags::Weapon_Stat_Damage_Base);

			auto EffectSpecHandle =
				AbilitySystem::AssignTagSetByCallerMagnitude(MakeOutgoingGameplayEffectSpec(DamageGE), GameplayTags::Weapon_Stat_Damage_Base, BaseDamage);
			ASC.ApplyGameplayEffectSpecToTarget(EffectSpecHandle, TargetASC);
		}
	}
}
