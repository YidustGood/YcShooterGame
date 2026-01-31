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
	bool bIsJumping;
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

	UPROPERTY(Category = "Animation")
	FVector2D MoveActionValue;

	UPROPERTY(Category = "Animation")
	FVector WeaponScopeOffset;

	UPROPERTY(Category = "Animation")
	FVector WorldSpaceLocationScope;

	UPROPERTY(Category = "Animation")
	bool bReplaceAimingPose;

	UPROPERTY(Category = "Animation")
	FRotator WeaponSawyRotation;

	/////////// Weapon Values ///////////
	UPROPERTY()
	UYcEquipmentInstance EquippedInst;

	AYcWeaponActorFP WeaponActorFP;

	UFUNCTION(BlueprintOverride)
	void BlueprintInitializeAnimation()
	{
		Character = Cast<ACharacter>(GetOwningActor());
		if (IsValid(Character))
		{
			AnimComponent = Character.GetComponentByClass(UYcShooterAnimComponent);
			QuickBarComponent = Character.GetComponentByClass(UYcQuickBarComponent);
		}
	}

	UFUNCTION(BlueprintOverride)
	void BlueprintUpdateAnimation(float DeltaTimeX)
	{
		if (IsValid(AnimComponent))
		{
			GetPawnValues();
		}
		if (IsValid(QuickBarComponent))
		{
			GetWeaponValues();
			CalculateWorldOffsetScope();
		}
	}

	UFUNCTION()
	void GetPawnValues()
	{
		GroundSpeed = AnimComponent.GetGroundSpeed();
		MoveDirection = AnimComponent.GetMoveDirection();
		bShouldMove = AnimComponent.ShouldMove();
		bIsJumping = AnimComponent.IsJumping();
		bIsFalling = AnimComponent.IsFalling();
		bCrouching = AnimComponent.IsCrouching();
		bAiming = AnimComponent.IsAiming();
		bRunning = AnimComponent.IsRunning();
		bReloading = AnimComponent.IsReloading();
		bLowered = AnimComponent.IsLowered();
		WeaponSawyRotation = AnimComponent.GetCurrentWeaponSwayRotation();
		MoveActionValue = AnimComponent.GetMoveActionValue();
	}

	UFUNCTION()
	void GetWeaponValues()
	{
		EquippedInst = QuickBarComponent.GetActiveEquipmentInstance();
		WeaponActorFP = Cast<AYcWeaponActorFP>(YcWeapon::GetPlayerFirstPersonWeaponActor(GetOwningActor()));
	}

	UFUNCTION()
	void CalculateWorldOffsetScope()
	{
		if (WeaponActorFP == nullptr)
			return;

		auto WeaponComp = WeaponActorFP.GetComponentByClass(UYcWeaponActorComponent);
		if (WeaponComp == nullptr)
			return;

		auto WeaponMesh = WeaponActorFP.WeaponMesh;
		auto ScopeMesh = WeaponActorFP.GetAttachmentMeshForSlot(GameplayTags::Attachment_Slot_Scope);
		if (ScopeMesh == nullptr)
		{
			WeaponScopeOffset = FVector();
			WorldSpaceLocationScope = FVector();
			return;
		}
		WeaponScopeOffset = WeaponComp.GetScopeOffset();

		if (!ScopeMesh.DoesSocketExist(n"SOCKET_Aim"))
		{
			bReplaceAimingPose = false;
			return;
		}

		bReplaceAimingPose = true;
		// 我们还需要一个摄像头组件，因为我们要将武器的位置向后移动，以便瞄准镜正好位于摄像头前方。所以，如果没有摄像头的话，我们就无法实现所有这些操作了
		auto FPCamera = GetOwningActor().FindComponentByTag(UCameraComponent, n"FPCamera");

		if (FPCamera == nullptr)
		{
			bReplaceAimingPose = false;
			return;
		}
		WorldSpaceLocationScope = TransScopeOffset(WeaponMesh, ScopeMesh, FPCamera);
	}

	UFUNCTION(BlueprintEvent)
	FVector TransScopeOffset(USkeletalMeshComponent WeaponMesh, UStaticMeshComponent ScopeMesh, UCameraComponent FPCamera)
	{
		auto FPCameraTrans = FPCamera.GetWorldTransform();

		auto ScopeAimTrans = ScopeMesh.GetSocketTransform(n"SOCKET_Aim", ERelativeTransformSpace::RTS_Component);
		float InverseScopeAimLocZ = ScopeAimTrans.Location.Z * -1;
		auto Loc1 = FPCameraTrans.TransformDirection(FVector(0, 0, InverseScopeAimLocZ)) + FPCameraTrans.Location;

		auto SocpeSocketTrans = WeaponMesh.GetSocketTransform(n"SOCKET_Scope", ERelativeTransformSpace::RTS_Component);
		auto InverseScopeSocketLoc = SocpeSocketTrans.Location * -1;
		auto Loc2 = FPCameraTrans.TransformDirection(FVector(InverseScopeSocketLoc.Y, InverseScopeSocketLoc.X, InverseScopeSocketLoc.Z));

		WorldSpaceLocationScope = FPCameraTrans.TransformDirection(FVector()) + (Loc1 + Loc2);

		return WorldSpaceLocationScope;
	}
}