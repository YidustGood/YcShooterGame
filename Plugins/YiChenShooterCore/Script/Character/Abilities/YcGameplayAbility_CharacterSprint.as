/** 玩家角色奔跑技能 */
class UYcGameplayAbility_CharacterSprint : UYcGameplayAbility_HoldToggle
{
	default AbilityPressActiveMode = EYcAbilityPressActiveMode::Toggle;

	default AbilityTags.AddTag(GameplayTags::InputTag_Move_Sprint);
	default ActivationOwnedTags.AddTag(GameplayTags::Character_State_Movement_Sprint);
	default ActivationBlockedTags.AddLeafTag(GameplayTags::InputTag_Weapon_Fire);
	default ActivationBlockedTags.AddLeafTag(GameplayTags::Ability_NoSprint);

	/** 奔跑速度 */
	UPROPERTY(EditDefaultsOnly)
	float SprintSpeed = 680;

	/** 普通走路速度 */
	UPROPERTY(EditDefaultsOnly)
	float NormalWalkSpeed = 500;

	UFUNCTION(BlueprintOverride)
	void OnActivateAbilityEvent()
	{
		Super::OnActivateAbilityEvent();
		SetCharacterMoveSpeed(SprintSpeed);

		// 启动持续检查任务，监测是否应该取消奔跑
		StartSprintConditionCheck();
	}

	UFUNCTION(BlueprintOverride)
	void OnEndAbilityEvent()
	{
		Super::OnEndAbilityEvent();
		SetCharacterMoveSpeed(NormalWalkSpeed);
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
		if (ASC.HasMatchingGameplayTag(GameplayTags::Character_State_Movement_NotForward))
		{
			EndAbility();
			return;
		}

		// 条件满足，继续下一次检查
		StartSprintConditionCheck();
	}

	void SetCharacterMoveSpeed(float NewMaxWalkSpeed)
	{
		ACharacter Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
		if (Character == nullptr)
		{
			Error("请确保UYcGameplayAbility_CharacterSprint激活时的AvatarActor是玩家控制的角色");
			EndAbility();
			return;
		}

		Print(f"UYcGameplayAbility_CharacterSprint调整移速:{NewMaxWalkSpeed}");

		auto CharacterMovement = Character.CharacterMovement;

		auto ASC = GetAbilitySystemComponentFromActorInfo();

		CharacterMovement.MaxWalkSpeed = NewMaxWalkSpeed;

		USkeletalMeshComponent FPCharacter = GetAvatarActorFromActorInfo().FindComponentByTag(USkeletalMeshComponent, n"FPCharacter");
		if (IsValid(FPCharacter))
		{
			FPCharacter.GetAnimInstance().Montage_Stop(0.01f);
		}
		else
		{
			Error("UYcGameplayAbility_CharacterSprint::SetCharacterMoveSpeed: 获取FPCharacer骨骼网格体组件失败!");
		}

		// if (NewMaxWalkSpeed == SprintSpeed)
		// {
		// 	ASC.RemoveLooseGameplayTag(GameplayTags::Character_State_Movement_Walk, 999);
		// 	ASC.AddLooseGameplayTag(GameplayTags::Character_State_Movement_Sprint);
		// }
		// else
		// {
		// 	ASC.RemoveLooseGameplayTag(GameplayTags::Character_State_Movement_Sprint, 999);
		// 	ASC.AddLooseGameplayTag(GameplayTags::Character_State_Movement_Walk);
		// }
	}
}