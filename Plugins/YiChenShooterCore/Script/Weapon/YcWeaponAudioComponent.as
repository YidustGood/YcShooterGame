// 武器音效管理组件 - 负责管理所有武器相关的音效播放和中断
class UYcWeaponAudioComponent : UActorComponent
{
	// ========================================
	// 内部状态
	// ========================================

	private AYcWeaponActor CachedWeaponActor;
	private UYcWeaponVisualData CachedWeaponVisualData;

	/** 当前正在播放的音效组件（按 Tag 索引，用于中断） */
	private TMap<FGameplayTag, UAudioComponent> ActiveAudioComponents;

	// ========================================
	// 生命周期
	// ========================================

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		CachedWeaponActor = Cast<AYcWeaponActor>(Owner);
		if (!IsValid(CachedWeaponActor))
		{
			Print("YcWeaponAudioComponent: Owner is not a YcWeaponActor!", 5.0f, FLinearColor::Red);
			return;
		}

		// 缓存 WeaponVisualData
		auto WeaponInst = CachedWeaponActor.GetWeaponInstance();
		if (IsValid(WeaponInst))
		{
			CachedWeaponVisualData = YcWeapon::GetWeaponVisualData(WeaponInst);
		}
	}

	UFUNCTION(BlueprintOverride)
	void EndPlay(EEndPlayReason Reason)
	{
		StopAllSounds();
	}

	// ========================================
	// 公共接口 - 按动作类型播放
	// ========================================

	/**
	 * 播放动作音效（自动从 VisualData 获取）
	 * @param ActionTag 动作标签（如 Asset.Weapon.Action.Reload）
	 * @param bAutoStop 是否在动作结束时自动停止（用于循环音效）
	 * @return 是否成功播放
	 */
	UFUNCTION()
	bool PlayActionSound(FGameplayTag ActionTag, bool bAutoStop = false)
	{
		// 将动作 Tag 转换为音效 Tag
		// 例如: Asset.Weapon.Action.Reload -> Asset.Weapon.Sound.Reload
		FGameplayTag SoundTag = ConvertActionTagToSoundTag(ActionTag);
		return PlaySound(SoundTag, ActionTag, bAutoStop);
	}

	/**
	 * 停止动作音效
	 * @param ActionTag 动作标签
	 */
	UFUNCTION()
	void StopActionSound(FGameplayTag ActionTag)
	{
		StopSound(ActionTag);
	}

	// ========================================
	// 公共接口 - 直接播放音效
	// ========================================

	/**
	 * 播放指定的音效
	 * @param SoundTag 音效标签（如 Asset.Weapon.Sound.Fire）
	 * @param ContextTag 上下文标签（用于停止，通常是动作 Tag）
	 * @param bAutoStop 是否在音效播放完毕后自动清理
	 * @return 是否成功播放
	 */
	UFUNCTION()
	bool PlaySound(FGameplayTag SoundTag, FGameplayTag ContextTag = FGameplayTag(), bool bAutoStop = false)
	{
		if (!SoundTag.IsValid())
		{
			Print("YcWeaponAudioComponent: Invalid SoundTag", 3.0f, FLinearColor::Yellow);
			return false;
		}

		// 如果没有提供 ContextTag，使用 SoundTag 作为 Key
		FGameplayTag Key = ContextTag.IsValid() ? ContextTag : SoundTag;

		// 停止之前的音效（如果存在）
		StopSound(Key);

		// 从 VisualData 获取音效资产
		USoundBase Sound = GetSoundAsset(SoundTag);
		if (Sound == nullptr)
		{
			Print("YcWeaponAudioComponent: 未找到音效 " + SoundTag.ToString() + "，请在 WeaponVisualData 中配置", 3.0f, FLinearColor::Yellow);
			return false;
		}

		// 播放音效
		UAudioComponent AudioComp = Gameplay::SpawnSoundAttached(
			Sound,
			CachedWeaponActor.WeaponMesh,
			n"None",
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true,
			1.0f,
			1.0f,
			0.0f,
			nullptr,
			nullptr,
			bAutoStop);

		if (!IsValid(AudioComp))
		{
			Print("YcWeaponAudioComponent: 音效组件创建失败", 3.0f, FLinearColor::Red);
			return false;
		}

		// 总是缓存音效组件（即使 bAutoStop = true，也需要能够手动停止）
		ActiveAudioComponents.Add(Key, AudioComp);

		return true;
	}

	/**
	 * 停止指定的音效
	 * @param ContextTag 上下文标签（通常是动作 Tag）
	 */
	UFUNCTION()
	void StopSound(FGameplayTag ContextTag)
	{
		if (!ContextTag.IsValid())
		{
			return;
		}

		UAudioComponent AudioComp;
		if (ActiveAudioComponents.Find(ContextTag, AudioComp))
		{
			if (IsValid(AudioComp))
			{
				AudioComp.Stop();
				AudioComp.DestroyComponent();
			}
			ActiveAudioComponents.Remove(ContextTag);
		}
	}

	/**
	 * 停止所有音效
	 */
	UFUNCTION()
	void StopAllSounds()
	{
		for (auto& Pair : ActiveAudioComponents)
		{
			if (IsValid(Pair.Value))
			{
				Pair.Value.Stop();
				Pair.Value.DestroyComponent();
			}
		}
		ActiveAudioComponents.Empty();
	}

	/**
	 * 淡出停止音效
	 * @param ContextTag 上下文标签
	 * @param FadeOutDuration 淡出时长（秒）
	 */
	UFUNCTION()
	void FadeOutSound(FGameplayTag ContextTag, float FadeOutDuration = 0.5f)
	{
		if (!ContextTag.IsValid())
		{
			return;
		}

		UAudioComponent AudioComp;
		if (ActiveAudioComponents.Find(ContextTag, AudioComp))
		{
			if (IsValid(AudioComp))
			{
				AudioComp.FadeOut(FadeOutDuration, 0.0f);
				// 注意：淡出后组件会自动停止，但不会自动销毁
				// 我们立即从缓存中移除，避免重复使用
			}
			ActiveAudioComponents.Remove(ContextTag);
		}
	}

	// ========================================
	// 公共接口 - 查询
	// ========================================

	/**
	 * 检查指定音效是否正在播放
	 */
	UFUNCTION()
	bool IsSoundPlaying(FGameplayTag ContextTag) const
	{
		UAudioComponent AudioComp;
		if (ActiveAudioComponents.Find(ContextTag, AudioComp))
		{
			return IsValid(AudioComp) && AudioComp.IsPlaying();
		}
		return false;
	}

	// ========================================
	// 内部辅助函数
	// ========================================

	/**
	 * 从 WeaponVisualData 获取音效资产
	 */
	private USoundBase GetSoundAsset(FGameplayTag SoundTag)
	{
		if (CachedWeaponVisualData == nullptr)
		{
			// 尝试重新获取
			auto WeaponInst = CachedWeaponActor.GetWeaponInstance();
			if (IsValid(WeaponInst))
			{
				CachedWeaponVisualData = YcWeapon::GetWeaponVisualData(WeaponInst);
			}
		}

		if (CachedWeaponVisualData == nullptr)
		{
			return nullptr;
		}

		return CachedWeaponVisualData.GetSoundEffect(SoundTag).Get();
	}

	/**
	 * 将动作 Tag 转换为音效 Tag
	 * 例如: Asset.Weapon.Action.Reload -> Asset.Weapon.Sound.Reload
	 */
	private FGameplayTag ConvertActionTagToSoundTag(FGameplayTag ActionTag)
	{
		// 简单的字符串替换
		FString ActionStr = ActionTag.ToString();
		FString SoundStr = ActionStr.Replace("Action", "Sound");
		return FGameplayTag::RequestGameplayTag(FName(SoundStr));
	}
}
