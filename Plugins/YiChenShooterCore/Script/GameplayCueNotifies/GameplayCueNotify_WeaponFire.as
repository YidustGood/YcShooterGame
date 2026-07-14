class UGameplayCueNotify_WeaponFire : UGameplayCueNotify_Burst
{
	// 射击音效资产标签
	UPROPERTY()
	FGameplayTag FireSoundTag = GameplayTags::Asset_Weapon_Sound_Fire;

	UFUNCTION(BlueprintOverride)
	void OnBurst(AActor Target, FGameplayCueParameters Parameters,
				 FGameplayCueNotify_SpawnResult SpawnResults) const
	{
		// 播放射击音效
		SpawnFireSound(Target, Parameters);

		// @TODO 临时先为所有情况都生成特效, 后面再做第三人称第一人称区分
		SpawnFirstPersonFireEffect(Target, Parameters);

		// if (PlayerController.IsLocalController())
		// {
		// 	SpawnFirstPersonFireEffect(Target, Parameters);
		// }
		// else
		// {
		// 	SpawnThirdPersonFireEffect(Target, Parameters);
		// }
	}

	UFUNCTION()
	void SpawnFirstPersonFireEffect(AActor Target, FGameplayCueParameters Parameters) const
	{
		auto FPWeaponActor = GetFirstPersonWeaponActor(Target);
		if (FPWeaponActor == nullptr)
		{
			Warning("FPWeaponActor is nullptr, can't spawn first effect.");
			return;
		}

		TArray<FVector> ImpactPositions;
		TArray<FVector> ImpactNormals;
		TArray<EPhysicalSurface> ImpactSurfaceTypes;

		if (!YcWeapon::GetImpactDataFromGameplayCueParameters(Parameters, ImpactPositions, ImpactNormals, ImpactSurfaceTypes))
		{
			Warning("GameplayCueNotify_WeaponFire: failed to extract impact data from gameplay cue parameters.");
			return;
		}

		FPWeaponActor.SpawnFireEffects(ImpactPositions, ImpactNormals, ImpactSurfaceTypes);
	}

	void SpawnThirdPersonFireEffect(AActor Target, FGameplayCueParameters Parameters) const
	{
	}

	UFUNCTION()
	void SpawnFireSound(AActor Target, FGameplayCueParameters Parameters) const
	{
		auto FPWeaponActor = GetFirstPersonWeaponActor(Target);
		if (FPWeaponActor == nullptr)
		{
			Warning("FPWeaponActor is nullptr, can't spawn fire sound.");
			return;
		}

		if (!IsValid(FPWeaponActor.AudioComponent))
		{
			Warning("FPWeaponActor.AudioComponent is nullptr, can't spawn fire sound.");
			return;
		}

		// 开火音效允许连发时自然叠加，不在代码侧主动中断上一发，具体并发上限交给音频资源配置。
		FPWeaponActor.AudioComponent.PlaySound(FireSoundTag, FGameplayTag(), false);
	}

	private AYcWeaponActorScript GetFirstPersonWeaponActor(AActor Target) const
	{
		if (!IsValid(Target))
		{
			return nullptr;
		}

		return Cast<AYcWeaponActorScript>(YcWeapon::GetPlayerFirstPersonWeaponActor(Target));
	}
}
