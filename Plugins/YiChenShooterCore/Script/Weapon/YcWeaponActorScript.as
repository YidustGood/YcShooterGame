// 武器 Actor 脚本 - 简化版，特效和音效管理委托给组件
class AYcWeaponActorScript : AYcWeaponActor
{
	/** 武器Actor辅助组件 */
	UPROPERTY(DefaultComponent)
	UYcWeaponActorComponent YcWeaponComponent;

	/** 特效管理组件 */
	UPROPERTY(DefaultComponent)
	UYcWeaponEffectComponent EffectComponent;

	/** 音效管理组件 */
	UPROPERTY(DefaultComponent)
	UYcWeaponAudioComponent AudioComponent;

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		// 武器被卸下后，主动收口仍挂在旧武器上的音效，避免切枪后旧武器尾音继续完整播放。
		EquipmentActorComponent.OnUnequipped.AddUFunction(this, n"OnWeaponUnequipped");
	}

	// ========================================
	// 公共接口 - 特效
	// ========================================

	/** 生成射击特效（由 GameplayCue 或 Ability 调用） */
	UFUNCTION()
	void SpawnFireEffects(
		TArray<FVector> ImpactPositions,
		TArray<FVector> ImpactNormals,
		TArray<EPhysicalSurface> ImpactSurfaceTypes) const
	{
		if (IsValid(EffectComponent))
		{
			EffectComponent.SpawnFireEffects(ImpactPositions, ImpactNormals, ImpactSurfaceTypes);
		}
	}

	/** 清理所有特效（武器卸载时调用） */
	UFUNCTION()
	void CleanupEffects()
	{
		if (IsValid(EffectComponent))
		{
			EffectComponent.CleanupAllEffectors();
		}
	}

	// ========================================
	// 公共接口 - 音效
	// ========================================

	/** 播放动作音效 */
	UFUNCTION()
	bool PlayActionSound(FGameplayTag ActionTag)
	{
		if (IsValid(AudioComponent))
		{
			return AudioComponent.PlayActionSound(ActionTag);
		}
		return false;
	}

	/** 停止动作音效 */
	UFUNCTION()
	void StopActionSound(FGameplayTag ActionTag)
	{
		if (IsValid(AudioComponent))
		{
			AudioComponent.StopActionSound(ActionTag);
		}
	}

	/** 停止所有音效 */
	UFUNCTION()
	void StopAllSounds()
	{
		if (IsValid(AudioComponent))
		{
			AudioComponent.StopAllSounds();
		}
	}

	UFUNCTION()
	void OnWeaponUnequipped(UYcEquipmentInstance EquipmentInst)
	{
		StopAllSounds();
	}
}
