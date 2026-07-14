enum EYcShotgunReloadStage
{
	None,
	Start,
	Loop,
	End
}

class UYcGameplayAbility_WeaponReloadShotgun : UYcGameplayAbility_WeaponBase
{
	default AbilityTags.AddTag(GameplayTags::InputTag_Weapon_Reload);
	default ActivationOwnedTags.AddTag(GameplayTags::InputTag_Weapon_Reload);

	// 换弹可以打断开火和检视
	default CancelAbilitiesWithTag.AddTag(GameplayTags::InputTag_Weapon_Fire);
	// 换弹可以打断检视
	default CancelAbilitiesWithTag.AddTag(GameplayTags::InputTag_Weapon_Inspect);

	// 添加 EventTrigger, 便于 AI 等系统触发换弹能力
	FAbilityTriggerData TriggerData;
	default TriggerData.TriggerTag = GameplayTags::InputTag_Weapon_Reload;
	default TriggerData.TriggerSource = EGameplayAbilityTriggerSource::GameplayEvent;
	default AbilityTriggers.Add(TriggerData);

	FYcWeaponActionVisual ReloadStartActionVisual;
	FYcWeaponActionVisual ReloadLoopActionVisual;
	FYcWeaponActionVisual ReloadEndActionVisual;

	UAbilityTask_WaitGameplayEvent WaitReloadMagInGameplayEvent;
	UAbilityTask_WaitGameplayEvent WaitFireInputGameplayEvent;

	EYcShotgunReloadStage CurrentReloadStage = EYcShotgunReloadStage::None;
	bool bPendingFireAfterReloadEnd = false;
	bool bTransitionToEndRequested = false;

	UFUNCTION(BlueprintOverride)
	void OnAbilityAdded()
	{
		Super::OnAbilityAdded();
		if (WeaponVisualDataCache != nullptr)
		{
			WeaponVisualDataCache.GetActionVisual(GameplayTags::Asset_Weapon_Action_Reload_Shotgun_Start, ReloadStartActionVisual);
			WeaponVisualDataCache.GetActionVisual(GameplayTags::Asset_Weapon_Action_Reload_Shotgun_Loop, ReloadLoopActionVisual);
			WeaponVisualDataCache.GetActionVisual(GameplayTags::Asset_Weapon_Action_Reload_Shotgun_End, ReloadEndActionVisual);
		}
	}

	/** 检查是否允许换弹 */
	UFUNCTION(BlueprintOverride)
	bool CanActivateAbility(FGameplayAbilityActorInfo InActorInfo, FGameplayAbilitySpecHandle Handle,
							FGameplayTagContainer& RelevantTags) const
	{
		auto ItemInst = GetAssociatedItem();
		int32 MagazineSize = ItemInst.GetStatTagStackCount(GameplayTags::Weapon_Stat_MagazineSize);
		int32 RemainingMagazineAmmo = ItemInst.GetStatTagStackCount(GameplayTags::Weapon_Stat_MagazineAmmo);
		int32 SpareAmmo = ItemInst.GetStatTagStackCount(GameplayTags::Weapon_Stat_SpareAmmo);
		bool bHasSpareAmmo = SpareAmmo > 0;
		bool bNeedReload = RemainingMagazineAmmo < MagazineSize;
		return bHasSpareAmmo && bNeedReload;
	}

	UFUNCTION(BlueprintOverride)
	void ActivateAbility()
	{
		bPendingFireAfterReloadEnd = false;
		bTransitionToEndRequested = false;
		CurrentReloadStage = EYcShotgunReloadStage::None;

		// 换弹期间统一阻止常规开火能力，允许 ShotgunReload 自己接管 Fire 输入并在合适时机补发
		GetYcAbilitySystemComponentFromActorInfo().AddLooseGameplayTag(GameplayTags::Weapon_State_NoFireAction);

		WaitAnimNotifyReloadMagIn();
		WaitFireInputDuringReload();
		StartReloadStage(EYcShotgunReloadStage::Start);
	}

	UFUNCTION(BlueprintOverride)
	void OnEndAbility(bool bWasCancelled)
	{
		CurrentReloadStage = EYcShotgunReloadStage::None;
		bTransitionToEndRequested = false;

		// 无论如何换弹结束都取消不能开火状态
		GetYcAbilitySystemComponentFromActorInfo().RemoveLooseGameplayTag(GameplayTags::Weapon_State_NoFireAction, 9999);
		StopShotgunReloadSounds();

		if (bWasCancelled)
		{
			StopCharacterReloadMontage();
		}
	}

	UFUNCTION()
	void StartReloadStage(EYcShotgunReloadStage NewStage)
	{
		CurrentReloadStage = NewStage;
		if (CurrentReloadStage == EYcShotgunReloadStage::End)
		{
			bTransitionToEndRequested = false;
		}

		UAnimMontage CharacterMontage = nullptr;
		UAnimMontage WeaponMontage = nullptr;

		switch (CurrentReloadStage)
		{
			case EYcShotgunReloadStage::Start:
				ActionTag = GameplayTags::Asset_Weapon_Action_Reload_Shotgun_Start;
				CharacterMontage = ReloadStartActionVisual.FPCharacterAnimMontage.Get();
				WeaponMontage = ReloadStartActionVisual.FPWeaponAnimMontage.Get();
				break;
			case EYcShotgunReloadStage::Loop:
				ActionTag = GameplayTags::Asset_Weapon_Action_Reload_Shotgun_Loop;
				CharacterMontage = ReloadLoopActionVisual.FPCharacterAnimMontage.Get();
				WeaponMontage = ReloadLoopActionVisual.FPWeaponAnimMontage.Get();
				break;
			case EYcShotgunReloadStage::End:
				ActionTag = GameplayTags::Asset_Weapon_Action_Reload_Shotgun_End;
				CharacterMontage = ReloadEndActionVisual.FPCharacterAnimMontage.Get();
				WeaponMontage = ReloadEndActionVisual.FPWeaponAnimMontage.Get();
				break;
			default:
				return;
		}

		if (CharacterMontage == nullptr)
		{
			EndAbility();
			return;
		}

		StopShotgunReloadSounds();
		PlayAbilitySound();
		PlayCharacterReloadMontage(CharacterMontage);
		NotifyWeaponPlayReloadMontage(WeaponMontage);
	}

	UFUNCTION()
	void PlayCharacterReloadMontage(UAnimMontage MontageToPlay)
	{
		auto PlayMontageTask =
			AsYcGameCoreGameTask::PlayMontageOnSkeletalMeshAndWait(
				this,
				n"",
				nullptr,
				n"FPCharacter",
				MontageToPlay,
				FGameplayTagContainer());

		PlayMontageTask.OnCompleted.AddUFunction(this, n"OnStageMontageCompleted");
		PlayMontageTask.OnInterrupted.AddUFunction(this, n"OnStageMontageInterrupted");
		PlayMontageTask.OnCancelled.AddUFunction(this, n"OnStageMontageInterrupted");
		PlayMontageTask.ReadyForActivation();
	}

	UFUNCTION()
	void NotifyWeaponPlayReloadMontage(UAnimMontage WeaponMontage)
	{
		FGameplayEventData Payload;
		Payload.EventTag = GameplayTags::InputTag_Weapon_Reload;
		Payload.OptionalObject = WeaponMontage;
		AbilitySystem::SendGameplayEventToActor(GetAvatarActorFromActorInfo(), GameplayTags::InputTag_Weapon_Reload, Payload);
	}

	UFUNCTION()
	void OnStageMontageCompleted(FGameplayTag EventTag, FGameplayEventData EventData)
	{
		switch (CurrentReloadStage)
		{
			case EYcShotgunReloadStage::Start:
				if (ShouldEnterReloadEnd())
				{
					StartReloadStage(EYcShotgunReloadStage::End);
				}
				else
				{
					StartReloadStage(EYcShotgunReloadStage::Loop);
				}
				break;
			case EYcShotgunReloadStage::Loop:
				if (ShouldEnterReloadEnd())
				{
					StartReloadStage(EYcShotgunReloadStage::End);
				}
				else
				{
					StartReloadStage(EYcShotgunReloadStage::Loop);
				}
				break;
			case EYcShotgunReloadStage::End:
				FinishShotgunReload();
				break;
			default:
				break;
		}
	}

	UFUNCTION()
	void OnStageMontageInterrupted(FGameplayTag EventTag, FGameplayEventData EventData)
	{
		if (CurrentReloadStage == EYcShotgunReloadStage::Loop && bTransitionToEndRequested)
		{
			StartReloadStage(EYcShotgunReloadStage::End);
		}
	}

	UFUNCTION()
	bool ShouldEnterReloadEnd()
	{
		return bTransitionToEndRequested || !CanLoadOneShell();
	}

	UFUNCTION()
	void FinishShotgunReload()
	{
		bool bShouldFire = bPendingFireAfterReloadEnd && GetRemainingMagazineAmmo() > 0;
		bPendingFireAfterReloadEnd = false;
		EndAbility();

		if (bShouldFire)
		{
			AbilitySystem::SendGameplayEventToActor(GetAvatarActorFromActorInfo(), GameplayTags::InputTag_Weapon_Fire, FGameplayEventData());
		}
	}

	UFUNCTION()
	int32 GetRemainingMagazineAmmo()
	{
		auto ItemInst = GetAssociatedItem();
		return ItemInst.GetStatTagStackCount(GameplayTags::Weapon_Stat_MagazineAmmo);
	}

	UFUNCTION()
	bool CanLoadOneShell()
	{
		auto ItemInst = GetAssociatedItem();
		int32 MagazineSize = ItemInst.GetStatTagStackCount(GameplayTags::Weapon_Stat_MagazineSize);
		int32 RemainingMagazineAmmo = ItemInst.GetStatTagStackCount(GameplayTags::Weapon_Stat_MagazineAmmo);
		int32 SpareAmmo = ItemInst.GetStatTagStackCount(GameplayTags::Weapon_Stat_SpareAmmo);
		return SpareAmmo > 0 && RemainingMagazineAmmo < MagazineSize;
	}

	// 单发装填
	UFUNCTION()
	void LoadOneShellIntoMagazine()
	{
		if (!HasAuthority() || !CanLoadOneShell())
			return;

		auto ItemInst = GetAssociatedItem();
		ItemInst.AddStatTagStack(GameplayTags::Weapon_Stat_MagazineAmmo, 1);
		ItemInst.RemoveStatTagStack(GameplayTags::Weapon_Stat_SpareAmmo, 1);
	}

	// 服务端监听单发装弹事件
	UFUNCTION()
	void WaitAnimNotifyReloadMagIn()
	{
		if (!HasAuthority())
			return;

		WaitReloadMagInGameplayEvent =
			AngelscriptAbilityTask::WaitGameplayEvent(this, GameplayTags::Yc_Weapon_AnimNotify_ReloadMagIn, nullptr, false, true);

		WaitReloadMagInGameplayEvent.EventReceived.AddUFunction(this, n"OnAnimNotifyReloadMagIn");
		WaitReloadMagInGameplayEvent.ReadyForActivation();
	}

	UFUNCTION()
	void OnAnimNotifyReloadMagIn(FGameplayEventData Payload)
	{
		LoadOneShellIntoMagazine();
	}

	// 监听 Fire 输入，用于在循环装弹期间请求中断换弹并在收尾后开火
	UFUNCTION()
	void WaitFireInputDuringReload()
	{
		WaitFireInputGameplayEvent =
			AngelscriptAbilityTask::WaitGameplayEvent(this, GameplayTags::InputTag_Weapon_Fire, nullptr, false, true);

		WaitFireInputGameplayEvent.EventReceived.AddUFunction(this, n"OnFireInputDuringReload");
		WaitFireInputGameplayEvent.ReadyForActivation();
	}

	UFUNCTION()
	void OnFireInputDuringReload(FGameplayEventData Payload)
	{
		if (CurrentReloadStage != EYcShotgunReloadStage::Loop)
		{
			return;
		}

		if (GetRemainingMagazineAmmo() <= 0)
		{
			return;
		}

		RequestEnterReloadEndAndFire();
	}

	UFUNCTION()
	void RequestEnterReloadEndAndFire()
	{
		if (CurrentReloadStage != EYcShotgunReloadStage::Loop || bTransitionToEndRequested)
		{
			return;
		}

		bPendingFireAfterReloadEnd = true;
		bTransitionToEndRequested = true;
		StopCharacterReloadMontage();
	}

	UFUNCTION()
	void StopCharacterReloadMontage()
	{
		USkeletalMeshComponent FPCharacter = GetAvatarActorFromActorInfo().FindComponentByTag(USkeletalMeshComponent, n"FPCharacter");
		if (IsValid(FPCharacter) && IsValid(FPCharacter.GetAnimInstance()))
		{
			FPCharacter.GetAnimInstance().Montage_Stop(0.05f);
		}
	}

	UFUNCTION()
	void StopShotgunReloadSounds()
	{
		StopSound(GameplayTags::Asset_Weapon_Action_Reload_Shotgun_Start);
		StopSound(GameplayTags::Asset_Weapon_Action_Reload_Shotgun_Loop);
		StopSound(GameplayTags::Asset_Weapon_Action_Reload_Shotgun_End);
	}
}
