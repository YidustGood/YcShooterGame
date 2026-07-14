class AYcShooterCharacterBase : AYcCharacter
{
	UPROPERTY(DefaultComponent)
	UYcHeroComponent YcHeroComp;

	UPROPERTY(DefaultComponent)
	UYcShooterAnimComponent YcShooterAnimComp;

	/** 移动数值设置, 供动画蓝图使用驱动移动动画播放效果 */
	UPROPERTY()
	FVector2D MoveActionValue;

	// 武器随视角移动的比例
	UPROPERTY(EditDefaultsOnly, Category = "Weapon | Anim")
	float WeaponRotationRatio = 8;

	// 武器随视角移动的插值
	UPROPERTY(EditDefaultsOnly, Category = "Weapon | Anim")
	float WeaponRotationInterp = 3.5;

	// 武器Sway摇摆旋转值, 供动画为第一人称武器添加动态旋转偏移
	UPROPERTY(BlueprintReadOnly, Category = "Weapon | Anim")
	FRotator CurrentWeaponSawyRotation;

	// 缓存上一帧的移动状态，避免在Tick中重复添加/移除标签
	private bool bWasMovingForward = true;

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		SetActorHiddenInGame(true);
		FActorInitStateChangedBPDelegate InitStateChangedDelegate;
		InitStateChangedDelegate.BindUFunction(this, n"OnInitStateGameplayReady");
		PawnExtComponent.K2_RegisterAndCallForInitStateChange(GameplayTags::InitState_GameplayReady, InitStateChangedDelegate);
	}

	UFUNCTION(BlueprintEvent)
	void OnInitStateGameplayReady(const FActorInitStateChangedParams&in Params)
	{
		SetActorHiddenInGame(false);
	}

	UFUNCTION(BlueprintOverride)
	void Tick(float DeltaSeconds)
	{
		if (IsValid(YcAbilitySystemComponent))
		{
			UpdateMovementStateTags();
		}

		// 每帧回复一定的武器旋转值, 实现视角停止后武器Sway摇摆的回中
		UpdateWeaponSawyRotation(true, 0.f);
		UpdateWeaponSawyRotation(false, 0.f);
	}

	// 第一人称移动: 相对角色自身朝向计算前后左右, 并缓存输入值供动画使用
	UFUNCTION(BlueprintOverride)
	void HandleMoveInput(FVector2D MovementVector)
	{
		MoveActionValue = MovementVector;
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}

	// 第一人称视角: 施加视角增量, 并根据视角输入驱动武器Sway摇摆
	UFUNCTION(BlueprintOverride)
	void HandleLookInput(FVector2D LookDelta)
	{
		if (LookDelta.X != 0.f)
		{
			AddControllerYawInput(LookDelta.X);
		}
		if (LookDelta.Y != 0.f)
		{
			AddControllerPitchInput(LookDelta.Y);
		}

		UpdateWeaponSawyRotation(true, LookDelta.X);
		UpdateWeaponSawyRotation(false, LookDelta.Y);
	}

	// 根据视角输入更新武器Sway摇摆旋转值
	void UpdateWeaponSawyRotation(bool bIsYaw, float InValue)
	{
		const float Value = Math::Clamp(InValue, -1.f, 1.f);
		FRotator NewRotation;
		const float Offset = WeaponRotationRatio * Value;
		if (bIsYaw)
		{
			NewRotation = FRotator(CurrentWeaponSawyRotation.Pitch, Offset, CurrentWeaponSawyRotation.Roll);
		}
		else
		{
			NewRotation = FRotator(-Offset, CurrentWeaponSawyRotation.Yaw, CurrentWeaponSawyRotation.Roll);
		}
		CurrentWeaponSawyRotation = Math::RInterpTo(CurrentWeaponSawyRotation, NewRotation, Gameplay::GetWorldDeltaSeconds(), WeaponRotationInterp);
	}

	// 更新移动状态标签，让能力系统根据标签自动管理能力状态
	void UpdateMovementStateTags()
	{
		float VelocitySize = CharacterMovement.Velocity.Size2D();
		float MoveDir = YcShooterAnimComp.GetMoveDirection();

		// 判断是否在向前移动（前方或斜前方，角度在±70度内）
		bool bIsMovingForward = VelocitySize >= 10.0f && Math::Abs(MoveDir) <= 70.0f;

		// 只在状态发生变化时才添加/移除标签，避免Tick中重复操作导致计数累积
		if (bIsMovingForward != bWasMovingForward)
		{
			if (!bIsMovingForward)
			{
				// 状态切换：从向前移动 -> 非向前移动
				YcAbilitySystemComponent.AddLooseGameplayTag(GameplayTags::Character_State_Movement_NotForward);
			}
			else
			{
				// 状态切换：从非向前移动 -> 向前移动, 移除999是避免标签count残留问题
				YcAbilitySystemComponent.RemoveLooseGameplayTag(GameplayTags::Character_State_Movement_NotForward, 999);
			}
			bWasMovingForward = bIsMovingForward;
		}
	}

	/** 获取武器Sawy摇摆旋转值, 动画蓝图通过这个值为武器添加动态的旋转偏移效果, 提升游戏手感 */
	FRotator& GetCurrentWeaponSwayRotation()
	{
		return CurrentWeaponSawyRotation;
	}
}