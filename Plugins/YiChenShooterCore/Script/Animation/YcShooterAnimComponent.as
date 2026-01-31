/** 射击角色动画组件, 用于提供一些函数接口快捷的获取动画所需的数据 */
class UYcShooterAnimComponent : UPawnComponent
{

	UPROPERTY(Category = "Animation")
	UAnimInstance AnimationInstance;

	UPROPERTY(Category = "Animation")
	AYcShooterCharacterBase OwnerCharacter;

	UPROPERTY(Category = "Animation")
	UCharacterMovementComponent CharacterMovement;

	UAbilitySystemComponent AbilityComp;

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		OwnerCharacter = Cast<AYcShooterCharacterBase>(GetOwner());
		CharacterMovement = OwnerCharacter.CharacterMovement;
		AbilityComp = UAbilitySystemComponent::Get(GetOwner());
	}

	UAbilitySystemComponent GetAbilityComp()
	{
		if (AbilityComp != nullptr)
			return AbilityComp;

		APawn Panw = Cast<APawn>(GetOwner());

		if (!IsValid(Panw) || !IsValid(Panw.PlayerState))
			return nullptr;

		AbilityComp = UAbilitySystemComponent::Get(Panw.PlayerState);
		return AbilityComp;
	}

	/**
	 * 根据运动组件的速度来设置速度和地面速度。
	 * 地面速度仅由速度的 X 轴和 Y 轴计算得出，因此向上或向下移动都不会影响它。
	 * @return 地面速度
	 */
	UFUNCTION()
	float GetGroundSpeed()
	{
		if (!IsValid(OwnerCharacter))
		{
			return 0.0f;
		}

		auto Velocity = OwnerCharacter.GetVelocity();
		return Velocity.Size2D();
	}

	/**
	 * Calculate direction using the delta between the velocity and the actor rotation. When the character is not strafing, clamp the value between -180 and +180 degrees so that backwards animations do not play when turning around, but running into wall looks better.
	 * @return 方向
	 */
	UFUNCTION()
	float GetMoveDirection()
	{
		if (!IsValid(OwnerCharacter) || !IsValid(AnimationInstance))
		{
			return 0.0f;
		}
		auto Velocity = OwnerCharacter.GetVelocity();
		float Direction = AnimationInstance.CalculateDirection(Velocity, OwnerCharacter.GetActorRotation());
		Direction = CharacterMovement.bOrientRotationToMovement ? Math::Clamp(Direction, -45.0f, 45.0f) : Direction;

		return Direction;
	}

	// Set Should Move to true only if ground speed is above a small threshold (to prevent incredibly small velocities from triggering animations) and if there is currently acceleration (input) applied.
	UFUNCTION()
	bool ShouldMove()
	{
		return GetGroundSpeed() > 0.01f && CharacterMovement.GetCurrentAcceleration().Size() != 0.0f;
	}

	UFUNCTION()
	bool IsJumping()
	{
		if (!IsValid(AbilityComp))
		{
			return false;
		}
		return AbilityComp.HasMatchingGameplayTag(GameplayTags::Character_State_Movement_Jumping);
	}

	// Set Is Falling from the movement components falling state.
	UFUNCTION()
	bool IsFalling()
	{
		if (!IsValid(CharacterMovement))
		{
			return false;
		}
		return CharacterMovement.IsFalling();
	}

	// Get Crouching
	UFUNCTION()
	bool IsCrouching()
	{
		if (!IsValid(CharacterMovement))
		{
			return false;
		}
		return CharacterMovement.IsCrouching();
	}

	UFUNCTION()
	bool IsAiming()
	{
		if (!IsValid(OwnerCharacter))
			return false;

		auto ASC = OwnerCharacter.GetYcAbilitySystemComponent();
		if (!IsValid(ASC))
			return false;

		return ASC.HasMatchingGameplayTag(GameplayTags::InputTag_Weapon_ADS);
	}

	UFUNCTION()
	bool IsRunning()
	{
		if (!IsValid(OwnerCharacter) || !IsValid(GetAbilityComp()))
		{
			return false;
		}
		FGameplayTagContainer Container;
		Container.AddTag(GameplayTags::Character_State_Movement_Sprint);
		return AbilityComp.HasMatchingGameplayTag(Container);
	}

	UFUNCTION()
	bool IsReloading()
	{
		if (!IsValid(OwnerCharacter) || !IsValid(GetAbilityComp()))
		{
			return false;
		}
		FGameplayTagContainer Container;
		Container.AddTag(GameplayTags::InputTag_Weapon_Reload);
		return AbilityComp.HasMatchingGameplayTag(Container);
	}

	UFUNCTION()
	bool IsLowered()
	{
		if (!IsValid(OwnerCharacter))
		{
			return false;
		}
		return false; // @TODO 待实现Lowered状态
	}

	UFUNCTION()
	FVector2D GetMoveActionValue()
	{
		if (!IsValid(OwnerCharacter))
		{
			return FVector2D();
		}
		return OwnerCharacter.MoveActionValue;
	}

	/** 获取武器Sawy摇摆旋转值, 动画蓝图通过这个值为武器添加动态的旋转偏移效果, 提升游戏手感 */
	FRotator GetCurrentWeaponSwayRotation()
	{
		if (!IsValid(OwnerCharacter))
		{
			return FRotator();
		}
		return OwnerCharacter.GetCurrentWeaponSwayRotation();
	}
}