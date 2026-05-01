class UYcCommonShooterAnimInstance : UYcShooterAnimationInstance
{
	// UPROPERTY()
	// TMap<FGameplayTag, UAnimSequenceBase> AnimSequenceMap;

	UPROPERTY(Category = "Animation")
	UAnimSequenceBase SequencePoseIdle;
	UPROPERTY(Category = "Animation")
	UAnimSequenceBase SequencePoseIdleLoop;
	UPROPERTY(Category = "Animation")
	UAnimSequenceBase SequencePoseAim;
	UPROPERTY(Category = "Animation")
	UAnimSequenceBase SequencePoseWalk;
	UPROPERTY(Category = "Animation")
	UAnimSequenceBase SequencePoseRun;
	UPROPERTY(Category = "Animation")
	UAnimSequenceBase SequencePoseCrouch;

	UPROPERTY(Category = "Animation")
	FGameplayTag SequencePoseIdleTag = GameplayTags::Asset_Weapon_Pose_Idle;
	UPROPERTY(Category = "Animation")
	FGameplayTag SequencePoseAimTag = GameplayTags::Asset_Weapon_Pose_Aim;
	UPROPERTY(Category = "Animation")
	FGameplayTag SequencePoseWalkTag = GameplayTags::Asset_Weapon_Pose_Walk;
	UPROPERTY(Category = "Animation")
	FGameplayTag SequencePoseRunTag = GameplayTags::Asset_Weapon_Pose_Run;
	UPROPERTY(Category = "Animation")
	FGameplayTag SequencePoseCrouchTag = GameplayTags::Asset_Weapon_Pose_Crouch;
	UPROPERTY(Category = "Animation")
	FGameplayTag SequencePoseIdleLoopTag = GameplayTags::Asset_Weapon_Pose_IdleLoop;

	// UFUNCTION(BlueprintOverride)
	// void BlueprintThreadSafeUpdateAnimation(float DeltaTime)
	// {
	// 	UpdateSequencePose();
	// }

	UFUNCTION(BlueprintOverride)
	void BlueprintUpdateAnimation(float DeltaTimeX)
	{
		Super::BlueprintUpdateAnimation(DeltaTimeX);
		UpdateSequencePose();
	}

	// 根据当前持有的武器更新基本姿势动画序列
	void UpdateSequencePose()
	{
		if (EquippedInst == nullptr)
			return;

		auto CurrentWeaponVisualData = YcWeapon::GetWeaponVisualData(EquippedInst);
		if (CurrentWeaponVisualData == nullptr)
		{
			Error("CurrentWeaponVisualData is nullptr for " +
				  EquippedInst.AssociatedItem.ItemRegistryId.RegistryType.Name.ToString() + "::" + EquippedInst.AssociatedItem.ItemRegistryId.ItemName.ToString());
		}
		else
		{
			SequencePoseIdle = CurrentWeaponVisualData.GetPoseAnim(SequencePoseIdleTag).Get();
			SequencePoseAim = CurrentWeaponVisualData.GetPoseAnim(SequencePoseAimTag).Get();
			SequencePoseRun = CurrentWeaponVisualData.GetPoseAnim(SequencePoseRunTag).Get();
			SequencePoseCrouch = CurrentWeaponVisualData.GetPoseAnim(SequencePoseCrouchTag).Get();
			SequencePoseWalk = CurrentWeaponVisualData.GetPoseAnim(SequencePoseWalkTag).Get();
			SequencePoseIdleLoop = CurrentWeaponVisualData.GetPoseAnim(SequencePoseIdleLoopTag).Get();
		}
	}
}