/**
 * 处理角色被伤害后的效果
 */
class AGameplayCueNotify_CharacterDamageTaken : AGameplayCueNotify_BurstLatent
{
	/** 被击中时生成的贴花大小 */
	UPROPERTY()
	FVector DecalSize = FVector(5, 8, 8);

	/** 被击中时生成的贴花生命周期 */
	UPROPERTY()
	float DecalLifeTime = 10.0f;

	UMaterialInterface DecalMaterial;
	ACharacter Target;
	FGameplayCueParameters Parameters;
	FHitResult HitResult;
	FVector HitLoc;
	FVector HitNormal;
	FName HitBoneName;

	UFUNCTION(BlueprintOverride)
	void OnBurst(AActor InTarget, FGameplayCueParameters InParameters,
				 FGameplayCueNotify_SpawnResult SpawnResults)
	{
		if (!IsValid(InTarget))
			return;
		this.Parameters = InParameters;
		this.Target = Cast<ACharacter>(InTarget);

		HitResult = AbilitySystem::GetHitResult(Parameters);
		HitLoc = HitResult.Location;
		HitNormal = HitResult.Normal;
		HitBoneName = HitResult.BoneName;

		// 处理被击中的动画
		DamageAnimation();

		// 手动生成贴花，这样我们就能正确地将其附着到骨骼上，并忽略默认表面
		if (IsValid(HitResult.PhysMaterial) && IsValid(DecalMaterial))
		{
			if (HitResult.PhysMaterial.SurfaceType != EPhysicalSurface::SurfaceType_Default)
			{

				// 计算出朝向碰撞表面法线方向的旋转,以正确在击中表面嵌入贴花
				FRotator DecalRot = FRotator::MakeFromX(-HitResult.Normal);
				FVector BoneSpacePos;
				FRotator BoneSapceRot;
				// 转换为骨骼空间
				this.Target.Mesh.TransformToBoneSpace(HitResult.BoneName, HitResult.Location, DecalRot, BoneSpacePos, BoneSapceRot);

				// 在人物网格体上生成贴花
				auto DecalComp = Gameplay::SpawnDecalAttached(DecalMaterial, DecalSize, this.Target.Mesh, HitResult.BoneName, BoneSpacePos, BoneSapceRot, EAttachLocation::KeepWorldPosition, DecalLifeTime);
				// 生成撞击标记，并设置其在 5 秒后淡出
				DecalComp.SetFadeOut(5.0f, 5.0f, false);
			}
		}

		// 激活方向命中反馈效果
		FVector HitDirection = HitResult.TraceStart - HitResult.TraceEnd;
		HitDirection.Normalize();
		TArray<UObject> CameraLensEffectObjects;
		SpawnResults.GetCameraLensEffectObjects(CameraLensEffectObjects); // 这个函数是YcAngelMixin插件提供的扩展函数

		for (auto Obj : CameraLensEffectObjects)
		{
			ANiagaraLensEffectBase LensEffect = Cast<ANiagaraLensEffectBase>(Obj);
			if (!IsValid(LensEffect))
				continue;

			LensEffect.NiagaraComponent.SetNiagaraVariableVec3("HitDirection", HitDirection);
			LensEffect.NiagaraComponent.SetVariablePosition(n"AttackWorldPosition", HitResult.TraceStart);
		}
	}

	void DamageAnimation()
	{
		auto GAS = AbilitySystem::GetAbilitySystemComponent(Target);
		if (!IsValid(GAS))
			return;

		auto Montage = SelectHitMontage(HitResult.Normal, Target);
		MontageHit(Target, Montage);
	}

	UAnimMontage SelectHitMontage(FVector InHitNormal, AActor InHitActor)
	{
		// @TODO 待实现根据收击方向选择不同的蒙太奇动画机制
		return nullptr;
	}

	// 多播击中蒙太奇动画
	UFUNCTION(NetMulticast)
	void MontageHit(ACharacter Character, UAnimMontage Montage)
	{
		Character.PlayAnimMontage(Montage);
	}
}