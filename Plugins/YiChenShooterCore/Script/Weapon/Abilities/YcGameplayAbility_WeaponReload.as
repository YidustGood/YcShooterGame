class UYcGameplayAbility_WeaponReload : UYcGameplayAbility_WeaponBase
{
	// 配置音效自动停止
	default bAutoStopSound = true;

	default AbilityTags.AddTag(GameplayTags::InputTag_Weapon_Reload);
	default ActivationOwnedTags.AddTag(GameplayTags::InputTag_Weapon_Reload);

	// 换弹可以打断开火和检视
	default CancelAbilitiesWithTag.AddTag(GameplayTags::InputTag_Weapon_Fire);
	default CancelAbilitiesWithTag.AddTag(GameplayTags::InputTag_Weapon_Inspect);

	// 弹夹为空时的换弹动作
	FYcWeaponActionVisual ReloadEmptyActionVisual;
	// 弹夹不为空时的战术换弹动作
	FYcWeaponActionVisual ReloadTacicalActionVisual;

	UAbilityTask_WaitGameplayEvent WaitReloadMagInGameplayEvent;

	UFUNCTION(BlueprintOverride)
	void OnAbilityAdded()
	{
		Super::OnAbilityAdded();
		if (WeaponVisualDataCache != nullptr)
		{
			WeaponVisualDataCache.GetActionVisual(GameplayTags::Asset_Weapon_Action_Reload_Empty, ReloadEmptyActionVisual);
			WeaponVisualDataCache.GetActionVisual(GameplayTags::Asset_Weapon_Action_Reload_Tactical, ReloadTacicalActionVisual);
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
		// 根据剩余子弹数量选择动作标签, 分为两种换弹动作
		if (GetRemainingMagazineAmmo() > 0)
		{
			// 配置动作 Tag（用于音效播放）
			ActionTag = GameplayTags::Asset_Weapon_Action_Reload_Tactical;
		}
		else
		{
			ActionTag = GameplayTags::Asset_Weapon_Action_Reload_Empty;
		}

		// 设置武器不可开火状态, 由换弹动画的动画通知处理恢复可开火状态
		GetYcAbilitySystemComponentFromActorInfo().AddLooseGameplayTag(GameplayTags::Weapon_State_NoFireAction);

		// 监听弹夹插入
		WaitAnimNotifyReloadMagIn();

		auto PlayMontageTask =
			AsYcGameCoreGameTask::PlayMontageOnSkeletalMeshAndWait(
				this,
				n"",
				nullptr,
				n"FPCharacter", // 第一人称手臂模型上要添加"FPCharacter"这个Tag才可以找到,
				GetCharacterReloadAnim(),
				FGameplayTagContainer());

		PlayMontageTask.OnCompleted.AddUFunction(this, n"OnReloadDone");
		PlayMontageTask.OnBlendOut.AddUFunction(this, n"OnReloadDone");
		PlayMontageTask.OnInterrupted.AddUFunction(this, n"OnReloadDone");
		PlayMontageTask.OnCancelled.AddUFunction(this, n"OnReloadDone");
		PlayMontageTask.ReadyForActivation();

		auto Char = GetAvatarActorFromActorInfo();
		//@TODO 具体动画干脆由具体Actor选择, FP和TP是不一样的
		// 通知FP武器Actor执行换弹
		FGameplayEventData Payload;
		Payload.EventTag = GameplayTags::InputTag_Weapon_Reload;
		Payload.OptionalObject = GetWeaponReloadAnim();
		AbilitySystem::SendGameplayEventToActor(GetAvatarActorFromActorInfo(), GameplayTags::InputTag_Weapon_Reload, Payload);

		// 播放换弹音效
		PlayAbilitySound();
	}

	UFUNCTION(BlueprintOverride)
	void OnEndAbility(bool bWasCancelled)
	{
		// 无论如何换弹结束都取消不能开火状态
		GetYcAbilitySystemComponentFromActorInfo().RemoveLooseGameplayTag(GameplayTags::Weapon_State_NoFireAction);
		StopAbilitySound();
	}

	UAnimMontage GetCharacterReloadAnim()
	{
		if (GetRemainingMagazineAmmo() > 0)
		{
			return ReloadTacicalActionVisual.FPCharacterAnimMontage.Get();
		}
		else
		{
			return ReloadEmptyActionVisual.FPCharacterAnimMontage.Get();
		}
	}

	UAnimMontage GetWeaponReloadAnim()
	{
		if (GetRemainingMagazineAmmo() > 0)
		{
			return ReloadTacicalActionVisual.FPWeaponAnimMontage.Get();
		}
		else
		{
			return ReloadEmptyActionVisual.FPWeaponAnimMontage.Get();
		}
	}

	int32 GetRemainingMagazineAmmo()
	{
		auto ItemInst = GetAssociatedItem();
		return ItemInst.GetStatTagStackCount(GameplayTags::Weapon_Stat_MagazineAmmo);
	}

	UFUNCTION()
	void OnReloadDone(FGameplayTag EventTag, FGameplayEventData EventData)
	{
		EndAbility();
	}

	// 换弹在子弹数值层面的实际操作函数
	UFUNCTION()
	void ReloadAmmoIntoMagazine()
	{
		// 数值修改操作只在权威端进行
		if (!HasAuthority())
			return;

		auto ItemInst = GetAssociatedItem();
		int32 MagazineSize = ItemInst.GetStatTagStackCount(GameplayTags::Weapon_Stat_MagazineSize);
		int32 RemainingMagazineAmmo = ItemInst.GetStatTagStackCount(GameplayTags::Weapon_Stat_MagazineAmmo);
		int32 SpareAmmo = ItemInst.GetStatTagStackCount(GameplayTags::Weapon_Stat_SpareAmmo);
		int32 CurrentTotalAmmo = SpareAmmo + RemainingMagazineAmmo;
		int32 NumToAddToMagazine = Math::Min(MagazineSize, CurrentTotalAmmo) - RemainingMagazineAmmo;
		ItemInst.AddStatTagStack(GameplayTags::Weapon_Stat_MagazineAmmo, NumToAddToMagazine);
		ItemInst.RemoveStatTagStack(GameplayTags::Weapon_Stat_SpareAmmo, NumToAddToMagazine);
	}

	// 服务端监听弹夹插入事件
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

	// 处理换弹
	UFUNCTION()
	void OnAnimNotifyReloadMagIn(FGameplayEventData Payload)
	{
		ReloadAmmoIntoMagazine();
	}
}