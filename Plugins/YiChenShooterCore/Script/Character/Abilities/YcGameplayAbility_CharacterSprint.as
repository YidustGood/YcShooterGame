/** 玩家角色奔跑技能 */
class UYcGameplayAbility_CharacterSprint : UYcGameplayAbility_HoldToggle
{
	default AbilityPressActiveMode = EYcAbilityPressActiveMode::Toggle;

	// 设置技能规则标签
	default AbilityTags.AddTag(GameplayTags::InputTag_Move_Sprint);
	default ActivationOwnedTags.AddTag(GameplayTags::Character_State_Movement_Sprint);
	default ActivationBlockedTags.AddLeafTag(GameplayTags::Character_State_Movement_NotForward);
	default ActivationBlockedTags.AddLeafTag(GameplayTags::Character_State_Movement_Crouching);
	default ActivationBlockedTags.AddLeafTag(GameplayTags::InputTag_Weapon_Fire);
	default ActivationBlockedTags.AddLeafTag(GameplayTags::InputTag_Weapon_ADS);
	default ActivationBlockedTags.AddLeafTag(GameplayTags::Ability_NoSprint);

	UFUNCTION(BlueprintOverride)
	void OnActivateAbilityEvent()
	{
		Super::OnActivateAbilityEvent();
		StopMontage();

		// 启动持续检查任务，监测是否应该取消奔跑
		StartSprintConditionCheck();
	}

	UFUNCTION(BlueprintOverride)
	void OnEndAbilityEvent()
	{
		Super::OnEndAbilityEvent();
		StopMontage();
	}

	// 持续检查奔跑条件，不满足时自动结束能力
	void StartSprintConditionCheck()
	{
		auto WaitDelay = AngelscriptAbilityTask::WaitDelay(this, 0.1f); // 每0.1秒检查一次
		WaitDelay.OnFinish.AddUFunction(this, n"CheckSprintCondition");
		WaitDelay.ReadyForActivation();
	}

	UFUNCTION()
	private void CheckSprintCondition()
	{
		auto ASC = GetAbilitySystemComponentFromActorInfo();

		// 检查是否有 NotForward 标签，如果有则取消奔跑
		if (ASC.HasMatchingGameplayTag(GameplayTags::Character_State_Movement_NotForward) ||
			ASC.HasMatchingGameplayTag(GameplayTags::InputTag_Weapon_ADS) ||
			ASC.HasMatchingGameplayTag(GameplayTags::Character_State_Movement_Crouching))
		{
			EndAbility();
			return;
		}

		// 条件满足，继续下一次检查
		StartSprintConditionCheck();
	}

	/** 停止蒙太奇动画 */
	void StopMontage()
	{
		USkeletalMeshComponent FPCharacter = GetAvatarActorFromActorInfo().FindComponentByTag(USkeletalMeshComponent, n"FPCharacter");
		if (IsValid(FPCharacter))
		{
			FPCharacter.GetAnimInstance().Montage_Stop(0.01f);
		}
		else
		{
			Error("UYcGameplayAbility_CharacterSprint::SetCharacterMoveSpeed: 获取FPCharacer骨骼网格体组件失败!");
		}
		// @TODO 目前只做了第一人称的蒙太奇动画停止, 还需要做第三人称的
	}
}