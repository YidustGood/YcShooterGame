/** 射击角色动画实例基类 */
class UYcShooterAnimationInstance : UAnimInstance
{

	UPROPERTY()
	UYcShooterAnimComponent AnimComponent;

	UPROPERTY()
	UYcQuickBarComponent QuickBarComponent;

	ACharacter Character;

	UPROPERTY(Category = "Animation")
	float GroundSpeed;
	UPROPERTY(Category = "Animation")
	float MoveDirection;
	UPROPERTY(Category = "Animation")
	bool bShouldMove;
	UPROPERTY(Category = "Animation")
	bool bIsFalling;
	UPROPERTY(Category = "Animation")
	bool bCrouching;
	UPROPERTY(Category = "Animation")
	bool bAiming;
	UPROPERTY(Category = "Animation")
	bool bRunning;
	UPROPERTY(Category = "Animation")
	bool bLowered;
	UPROPERTY(Category = "Animation")
	bool bReloading;

	/////////// Weapon Values ///////////
	UPROPERTY()
	UYcEquipmentInstance EquippedInst;

	UFUNCTION(BlueprintOverride)
	void BlueprintInitializeAnimation()
	{
		Character = Cast<ACharacter>(GetOwningActor());
	}

	UFUNCTION(BlueprintOverride)
	void BlueprintUpdateAnimation(float DeltaTimeX)
	{
		if (IsValid(Character))
		{
			AnimComponent = Character.GetComponentByClass(UYcShooterAnimComponent);
			QuickBarComponent = Character.GetComponentByClass(UYcQuickBarComponent);
		}
		if (IsValid(AnimComponent))
		{
			GetPawnValues();
		}
		if (IsValid(QuickBarComponent))
		{
			GetWeaponValues();
		}
	}

	UFUNCTION()
	void GetPawnValues()
	{
		GroundSpeed = AnimComponent.GetGroundSpeed();
		MoveDirection = AnimComponent.GetMoveDirection();
		bShouldMove = AnimComponent.ShouldMove();
		bIsFalling = AnimComponent.IsFalling();
		bCrouching = AnimComponent.IsCrouching();
		bAiming = AnimComponent.IsAiming();
		bRunning = AnimComponent.IsRunning();
		bReloading = AnimComponent.IsReloading();
		bLowered = AnimComponent.IsLowered();
	}

	UFUNCTION()
	void GetWeaponValues()
	{
		EquippedInst = QuickBarComponent.GetActiveEquipmentInstance();
	}
}