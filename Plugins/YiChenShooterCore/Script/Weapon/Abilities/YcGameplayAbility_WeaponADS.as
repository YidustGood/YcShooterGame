/** 武器瞄准技能 */
class UYcGameplayAbility_WeaponADS : UYcGameplayAbility_WeaponHoldToggle
{
	//@TODO 这个值可以与输入按键设置系统做集成关联, 以实现用户自由设置按住/切换的输入模式
	default AbilityPressActiveMode = EYcWeaponAbilityPressActiveMode::Hold;

	// 设置ADS Tag
	default AbilityTags.AddTag(GameplayTags::InputTag_Weapon_ADS);
	default ActivationOwnedTags.AddTag(GameplayTags::InputTag_Weapon_ADS);

	UFUNCTION(BlueprintOverride)
	void OnActivateAbilityEvent()
	{
		// 获取武器实例以验证技能可以激活
		UYcHitScanWeaponInstance WeaponInstance = GetWeaponInstance();
		if (WeaponInstance == nullptr)
		{
			Print("ADS Ability: Failed to get weapon instance", 3.0f, FLinearColor::Red);
			EndAbility();
			return;
		}

		// 添加 Status.Weapon.ADS 标签到 ASC
		auto ASC = GetYcAbilitySystemComponentFromActorInfo();
		if (ASC != nullptr)
		{
			ASC.AddLooseGameplayTag(GameplayTags::Status_Weapon_ADS, 1);
		}

		// 广播 ADS 状态变化消息（进入瞄准）
		BroadcastADSStateChanged(true);
	}

	UFUNCTION(BlueprintOverride)
	void OnEndAbilityEvent()
	{
		// 从 ASC 移除 Status.Weapon.ADS 标签
		auto ASC = GetYcAbilitySystemComponentFromActorInfo();
		if (ASC != nullptr)
		{
			// 使用 999 来确保标签被完全移除，避免计数残留问题
			ASC.RemoveLooseGameplayTag(GameplayTags::Status_Weapon_ADS, 999);
		}

		// 广播 ADS 状态变化消息（退出瞄准）
		BroadcastADSStateChanged(false);
	}

	/**
	 * 获取当前武器实例
	 * @return 当前装备的武器实例，如果没有则返回 nullptr
	 */
	UFUNCTION()
	UYcHitScanWeaponInstance GetWeaponInstance()
	{
		// 从关联的装备实例中获取并转换为 HitScan 武器实例
		return Cast<UYcHitScanWeaponInstance>(GetAssociatedEquipment());
	}

	/**
	 * 广播 ADS 状态变化消息
	 * @param bIsAiming 是否正在瞄准
	 */
	UFUNCTION()
	void BroadcastADSStateChanged(bool bIsAiming)
	{
		// 获取武器实例
		UYcHitScanWeaponInstance WeaponInstance = GetWeaponInstance();
		if (WeaponInstance == nullptr)
		{
			Print("ADS Ability: Cannot broadcast message - no weapon instance", 3.0f, FLinearColor::Yellow);
			return;
		}

		// 获取玩家控制器
		APlayerController PlayerController = Cast<APlayerController>(GetControllerFromActorInfo());
		if (PlayerController == nullptr)
		{
			Print("ADS Ability: Cannot broadcast message - no player controller", 3.0f, FLinearColor::Yellow);
			return;
		}

		// 创建消息
		FYcWeaponADSMessage Message;
		Message.bIsAiming = bIsAiming;
		Message.WeaponInstance = WeaponInstance;
		Message.PlayerController = PlayerController;

		// 广播消息
		auto MessageSubsystem = UGameplayMessageSubsystem::Get();
		if (MessageSubsystem != nullptr)
		{
			MessageSubsystem.BroadcastMessage(GameplayTags::Message_Weapon_ADS_StateChanged, Message);
		}
	}
}