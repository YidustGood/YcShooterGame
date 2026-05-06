/**
 * 近战武器攻击GA基类
 * 需要为刀模型添加以AttackPoint为前缀的Socket, GA会自动获取到所有的AttackPoint, 例如{AttackPoint1, AttackPoint2, ...}
 * 攻击时会在所有的AttackPoint上进行射线检测
 */
class UYcGameplayAbility_KnifeAttackBase : UYcGameplayAbility_WeaponBase
{
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UGameplayEffect> DamageGE;

	UPROPERTY(EditDefaultsOnly)
	EYcMeleeAttackType AttackType = EYcMeleeAttackType::Light;

	UPROPERTY()
	EDrawDebugTrace DrawDebugTrace;

	UPROPERTY()
	bool bLogHitActors;

	USkeletalMeshComponent ArmsMesh;
	USkeletalMeshComponent KnifeMesh;
	TArray<FName> KnifeSocketName;
	TArray<FVector> TracePoints;
	FTimerHandle AttackTimer;
	FYcWeaponActionVisual MeleeAction;
	FYcMeleeAttackProfile CurrentAttackProfile;

	// UPROPERTY(ReplicatedUsing = OnRep_UniqueHits)
	TArray<FHitResult> UniqueHits;
	TMap<AActor, int> ActorHited;

	UFUNCTION(BlueprintOverride)
	void OnAbilityAdded()
	{
		Super::OnAbilityAdded();

		if (!IsLocallyControlled())
			return;

		auto Avatar = GetAvatarActorFromActorInfo();
		ArmsMesh = Avatar.GetComponentsByTag(USkeletalMeshComponent, n"FPCharacter")[0];
	}

	protected EYcMeleeAttackType GetAttackType()
	{
		return AttackType;
	}

	protected FGameplayTag GetAnimEventTag()
	{
		return GameplayTags::InputTag_Weapon_Fire;
	}

	protected FGameplayTag GetDefaultActionTag()
	{
		return GetAttackType() == EYcMeleeAttackType::Heavy ? GameplayTags::Asset_Weapon_Action_Melee_Heavy : GameplayTags::Asset_Weapon_Action_Melee_Light;
	}

	protected void RefreshAttackProfile()
	{
		auto MeleeAttackConfigFragment = FindEquipmentFragment(FYcFragment_MeleeAttackConfig);
		if (MeleeAttackConfigFragment.IsValid())
		{
			auto Fragment = MeleeAttackConfigFragment.Get(FYcFragment_MeleeAttackConfig);
			CurrentAttackProfile = GetAttackType() == EYcMeleeAttackType::Heavy ? Fragment.HeavyAttack : Fragment.LightAttack;
		}
		else
		{
			CurrentAttackProfile = FYcMeleeAttackProfile();
		}

		FGameplayTag ActionTag = CurrentAttackProfile.ActionTag;
		if (!ActionTag.IsValid())
		{
			ActionTag = GetDefaultActionTag();
		}

		if (WeaponVisualDataCache != nullptr)
		{
			WeaponVisualDataCache.GetActionVisual(ActionTag, MeleeAction);
		}
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

		RefreshAttackProfile();

		// 初始化刀模型
		if (KnifeMesh == nullptr)
		{
			// @TODO 这里目前是简单的遍历获得刀模型, 目前只会在首次攻击时调用, 如果确实是需要那么再做性能优化, 注意一定要给刀模型添加Tag(KnifeMesh)
			TArray<USkeletalMeshComponent> Children = ArmsMesh.GetChildrenComponentsByClass(USkeletalMeshComponent, true);
			for (auto& Child : Children)
			{
				if (Child.ComponentHasTag(n"KnifeMesh"))
				{
					KnifeMesh = Child;
					InitAttackPointsName();
					break;
				}
			}
		}
		PlayAttachAnim();
		PlayAbilitySound();
	}

	void PlayAttachAnim()
	{
		auto PlayMontageCallbackProxy = ArmsMesh.PlayMontage(MeleeAction.FPCharacterAnimMontage.Get());
		PlayMontageCallbackProxy.OnCompleted.AddUFunction(this, n"OnAttackDone");
		PlayMontageCallbackProxy.OnBlendOut.AddUFunction(this, n"OnAttackDone");
		PlayMontageCallbackProxy.OnInterrupted.AddUFunction(this, n"OnAttackDone");
		PlayMontageCallbackProxy.OnNotifyBegin.AddUFunction(this, n"StartKnifeAttack");
		PlayMontageCallbackProxy.OnNotifyEnd.AddUFunction(this, n"EndKnifeAttack");

		if (KnifeMesh != nullptr && MeleeAction.FPWeaponAnimMontage.IsValid())
		{
			KnifeMesh.PlayMontage(MeleeAction.FPWeaponAnimMontage.Get());
		}
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

	int GetHitQualityScore(const FHitResult& Hit)
	{
		int Score = 0;

		if (Cast<USkeletalMeshComponent>(Hit.Component) != nullptr)
		{
			Score += 2;
		}

		if (Hit.BoneName.ToString().Len() > 0)
		{
			Score += 2;
		}

		if (IsValid(Hit.PhysMaterial) && Hit.PhysMaterial.SurfaceType != EPhysicalSurface::SurfaceType_Default)
		{
			Score += 1;
		}

		return Score;
	}

	void AddOrUpdateUniqueHit(const FHitResult& Hit)
	{
		int ExistingIndex = -1;
		if (ActorHited.Find(Hit.Actor, ExistingIndex))
		{
			if (GetHitQualityScore(Hit) > GetHitQualityScore(UniqueHits[ExistingIndex]))
			{
				UniqueHits[ExistingIndex] = Hit;
			}
			return;
		}

		UniqueHits.Add(Hit);
		ActorHited.Add(Hit.Actor, UniqueHits.Num() - 1);
		BroadcastLocalHitFeedback(Hit);
	}

	// 广播本地命中反馈
	void BroadcastLocalHitFeedback(const FHitResult& Hit)
	{
		if (!IsLocallyControlled())
		{
			return;
		}

		auto Hitter = GetControllerFromActorInfo();
		if (Hitter == nullptr || Hit.Actor == nullptr)
		{
			return;
		}

		FHitCharacterMessage HitMessage;
		HitMessage.Hitter = Hitter;
		HitMessage.HitResult = Hit;

		UGameplayMessageSubsystem::Get().BroadcastMessage(GameplayTags::GameplayCue_Character_DamageTaken, HitMessage);
	}

	void DrawTraceDebugShape(const FVector& StartPos, const FVector& EndPos)
	{
		if (DrawDebugTrace == EDrawDebugTrace::None)
		{
			return;
		}

		TArray<EObjectTypeQuery> ObjectTypes;
		ObjectTypes.Add(EObjectTypeQuery::Pawn);

		TArray<AActor> ActorsToIgnore;
		ActorsToIgnore.Add(GetAvatarActorFromActorInfo());
		TArray<FHitResult> DebugHits;

		if (CurrentAttackProfile.TraceShape == EYcMeleeTraceShape::Sphere)
		{
			System::SphereTraceMultiForObjects(StartPos,
											   EndPos,
											   CurrentAttackProfile.TraceSphereRadius,
											   ObjectTypes,
											   true,
											   ActorsToIgnore,
											   DrawDebugTrace,
											   DebugHits,
											   true);
		}
		else if (CurrentAttackProfile.TraceShape == EYcMeleeTraceShape::Capsule)
		{
			System::CapsuleTraceMultiForObjects(StartPos,
												EndPos,
												CurrentAttackProfile.TraceCapsuleRadius,
												CurrentAttackProfile.TraceCapsuleHalfHeight,
												ObjectTypes,
												true,
												ActorsToIgnore,
												DrawDebugTrace,
												DebugHits,
												true);
		}
		else if (CurrentAttackProfile.TraceShape == EYcMeleeTraceShape::Box)
		{
			System::BoxTraceMultiForObjects(StartPos,
											EndPos,
											CurrentAttackProfile.TraceBoxHalfSize,
											CurrentAttackProfile.TraceBoxOrientation,
											ObjectTypes,
											true,
											ActorsToIgnore,
											DrawDebugTrace,
											DebugHits,
											true);
		}
		else
		{
			System::LineTraceMultiForObjects(StartPos,
											 EndPos,
											 ObjectTypes,
											 true,
											 ActorsToIgnore,
											 DrawDebugTrace,
											 DebugHits,
											 true);
		}
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
		AttackTimer = System::SetTimer(this, n"DoTrace", CurrentAttackProfile.AttackCheckInterval, true);
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

	// 攻击蒙太奇定时器回调
	UFUNCTION()
	void DoTrace()
	{
		FCollisionQueryParams TraceParams;
		TraceParams.bTraceComplex = true;
		TraceParams.bReturnPhysicalMaterial = true;
		TraceParams.AddIgnoredActor(GetAvatarActorFromActorInfo());

		for (int i = 0; i < KnifeSocketName.Num(); i++)
		{
			FVector EndPos = KnifeMesh.GetSocketLocation(KnifeSocketName[i]);
			TArray<FHitResult> OutHits;
			DrawTraceDebugShape(TracePoints[i], EndPos);

			if (CurrentAttackProfile.TraceShape == EYcMeleeTraceShape::Sphere)
			{
				System::SweepMultiByChannel(OutHits,
											TracePoints[i],
											EndPos,
											FQuat::Identity,
											ECollisionChannel::ECC_GameTraceChannel2,
											FCollisionShape::MakeSphere(CurrentAttackProfile.TraceSphereRadius),
											TraceParams);
			}
			else if (CurrentAttackProfile.TraceShape == EYcMeleeTraceShape::Capsule)
			{
				System::SweepMultiByChannel(OutHits,
											TracePoints[i],
											EndPos,
											FQuat::Identity,
											ECollisionChannel::ECC_GameTraceChannel2,
											FCollisionShape::MakeCapsule(CurrentAttackProfile.TraceCapsuleRadius, CurrentAttackProfile.TraceCapsuleHalfHeight),
											TraceParams);
			}
			else if (CurrentAttackProfile.TraceShape == EYcMeleeTraceShape::Box)
			{
				System::SweepMultiByChannel(OutHits,
											TracePoints[i],
											EndPos,
											CurrentAttackProfile.TraceBoxOrientation.Quaternion(),
											ECollisionChannel::ECC_GameTraceChannel2,
											FCollisionShape::MakeBox(CurrentAttackProfile.TraceBoxHalfSize),
											TraceParams);
			}
			else
			{
				System::LineTraceMultiByChannel(OutHits,
												TracePoints[i],
												EndPos,
												ECollisionChannel::ECC_GameTraceChannel2,
												TraceParams);
			}

			// 添加唯一的命中结果,通过引用避免复制
			for (auto& Hit : OutHits)
			{
				if (!Hit.bBlockingHit)
					continue;
				if (Cast<USkeletalMeshComponent>(Hit.Component) == nullptr)
					continue;
				AddOrUpdateUniqueHit(Hit);
			}
		}
		UpdateTracePoints();
	}

	UFUNCTION()
	void ProcessHits()
	{
		RefreshAttackProfile();

		TArray<FInstancedStruct> RuntimePayloads;
		FYcMeleeAttackRuntimePayload AttackPayload;
		AttackPayload.AttackType = GetAttackType();
		RuntimePayloads.Add(FInstancedStruct::Make(AttackPayload));

		if (CurrentAttackProfile.BackstabDamageMultiplier > 1.0f)
		{
			FYcBackstabRuntimePayload BackstabPayload;
			BackstabPayload.DamageMultiplier = CurrentAttackProfile.BackstabDamageMultiplier;
			BackstabPayload.MaxAngleDegrees = CurrentAttackProfile.BackstabMaxAngleDegrees;
			RuntimePayloads.Add(FInstancedStruct::Make(BackstabPayload));
		}

		for (auto& Hit : UniqueHits)
		{
			if (bLogHitActors)
			{
				Print("KnifeAttack: " + Hit.Actor.ToString());
			}

			if (DamageGE == nullptr)
			{
				Warning("UYcGameplayAbility_KnifeAttackBase: DamageGE is nullptr, cannot apply damage!");
				continue;
			}

			FGameplayAbilityTargetDataHandle TargetData = YcWeapon::MakeSingleTargetHitTargetData(Hit, RuntimePayloads, CurrentAttackProfile.DamageTypeTag, -1);
			float BaseDamage = CurrentAttackProfile.BaseDamage;
			if (BaseDamage <= 0.0f && AssociatedItem != nullptr)
			{
				BaseDamage = AssociatedItem.GetFloatTagStackValue(GameplayTags::Weapon_Stat_Damage_Base);
			}

			auto EffectSpecHandle =
				AbilitySystem::AssignTagSetByCallerMagnitude(MakeOutgoingGameplayEffectSpec(DamageGE), GameplayTags::Weapon_Stat_Damage_Base, BaseDamage);
			ApplyGameplayEffectSpecToTarget(EffectSpecHandle, TargetData);
		}
	}
}

/**
 * 近战武器轻击
 */
class UYcGameplayAbility_KnifeAttack : UYcGameplayAbility_KnifeAttackBase
{
	default AbilityTags.AddTag(GameplayTags::InputTag_Weapon_Fire);
	default ActivationOwnedTags.AddTag(GameplayTags::InputTag_Weapon_Fire);
	default AttackType = EYcMeleeAttackType::Light;

	// 添加EventTrigger, 便于AI等系统触发攻击能力
	FAbilityTriggerData TriggerData;
	default TriggerData.TriggerTag = GameplayTags::InputTag_Weapon_Fire;
	default TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	default AbilityTriggers.Add(TriggerData);

	protected FGameplayTag GetAnimEventTag() override
	{
		return GameplayTags::InputTag_Weapon_Fire;
	}
}
