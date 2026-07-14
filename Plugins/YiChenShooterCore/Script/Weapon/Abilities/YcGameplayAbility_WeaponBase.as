class UYcGameplayAbility_WeaponBase : UYcGameplayAbility_FromEquipment
{
	// 缓存WeaponVisualData资产, 方便获取动画之类的资产供技能使用
	UPROPERTY(VisibleInstanceOnly)
	UYcWeaponVisualData WeaponVisualDataCache;

	// ========================================
	// 音效配置
	// ========================================

	/** 技能对应的动作 Tag（用于播放音效，需要在子类中配置） */
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	FGameplayTag ActionTag;

	/** 是否在技能激活时自动播放音效 */
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	bool bAutoPlaySoundOnActivate = false;

	/** 是否在技能结束时自动停止音效 */
	UPROPERTY(EditDefaultsOnly, Category = "Audio")
	bool bAutoStopSoundOnEnd = false;

	/** 缓存最近一次实际播放音效时使用的武器 Actor，避免切枪后停止逻辑误停到新武器上。 */
	UPROPERTY(VisibleInstanceOnly)
	AYcWeaponActorScript CachedAudioWeaponActor;

	UFUNCTION(BlueprintOverride)
	void OnAbilityAdded()
	{
		WeaponVisualDataCache = YcWeapon::GetWeaponVisualData(GetAssociatedEquipment());
	}

	// ========================================
	// 音效辅助函数
	// ========================================

	/**
	 * 播放当前技能的音效
	 * 自动根据 ActionTag 查找对应的音效
	 */
	UFUNCTION()
	bool PlayAbilitySound()
	{
		if (!ActionTag.IsValid())
		{
			Print("PlayAbilitySound: ActionTag 未配置", 3.0f, FLinearColor::Yellow);
			return false;
		}

		AYcWeaponActorScript WeaponActor = GetWeaponActorFromAvatar();
		if (!IsValid(WeaponActor))
		{
			return false;
		}

		CachedAudioWeaponActor = WeaponActor;
		return WeaponActor.PlayActionSound(ActionTag);
	}

	/**
	 * 停止当前技能的音效
	 */
	UFUNCTION()
	void StopAbilitySound()
	{
		if (!ActionTag.IsValid())
		{
			return;
		}

		AYcWeaponActorScript WeaponActor = GetAudioWeaponActorForStop();
		if (!IsValid(WeaponActor))
		{
			return;
		}

		WeaponActor.StopActionSound(ActionTag);
	}

	/**
	 * 播放指定的音效
	 */
	UFUNCTION()
	bool PlaySound(FGameplayTag SoundTag, FGameplayTag ContextTag = FGameplayTag())
	{
		AYcWeaponActorScript WeaponActor = GetWeaponActorFromAvatar();
		if (!IsValid(WeaponActor) || !IsValid(WeaponActor.AudioComponent))
		{
			return false;
		}

		CachedAudioWeaponActor = WeaponActor;
		return WeaponActor.AudioComponent.PlaySound(SoundTag, ContextTag);
	}

	/**
	 * 停止指定的音效
	 */
	UFUNCTION()
	void StopSound(FGameplayTag ContextTag)
	{
		AYcWeaponActorScript WeaponActor = GetAudioWeaponActorForStop();
		if (!IsValid(WeaponActor) || !IsValid(WeaponActor.AudioComponent))
		{
			return;
		}

		WeaponActor.AudioComponent.StopSound(ContextTag);
	}

	private AYcWeaponActorScript GetAudioWeaponActorForStop()
	{
		if (IsValid(CachedAudioWeaponActor))
		{
			return CachedAudioWeaponActor;
		}

		return GetWeaponActorFromAvatar();
	}

	/**
	 * 从 Avatar Actor 获取武器 Actor
	 * 子类可以重写此方法以适配不同的获取方式
	 */
	UFUNCTION()
	AYcWeaponActorScript GetWeaponActorFromAvatar()
	{
		return Cast<AYcWeaponActorScript>(YcWeapon::GetPlayerFirstPersonWeaponActor(GetAvatarActorFromActorInfo()));
	}
}
