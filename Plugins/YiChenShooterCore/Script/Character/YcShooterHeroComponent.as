/**
 * 第一人称射击游戏专用的HeroComp组件
 * 因为一些输入是在Hero组件处理的, 所以我在这里做了与输入相关联的武器Sway摇摆值的计算
 */
class UYcShooterHeroComponent : UYcHeroComponent
{
	// 武器随视角移动的比例
	UPROPERTY(EditDefaultsOnly, Category = "Weapon | Anim")
	float WeaponRotationRatio = 8;

	// 武器随视角移动的插值
	UPROPERTY(EditDefaultsOnly, Category = "Weapon | Anim")
	float WeaponRotationInterp = 3.5;

	UPROPERTY(BlueprintReadOnly, Category = "Weapon | Anim")
	FRotator CurrentWeaponSawyRotation;

	// 根据移动输入更新武器旋转值
	UFUNCTION(BlueprintOverride)
	void OnLookMouse(FInputActionValue InputActionValue)
	{
		UpdateWeaponSawyRotation(true, InputActionValue.Axis2D.X);
		UpdateWeaponSawyRotation(false, InputActionValue.Axis2D.Y);
	}

	// 每帧都回复一定的武器旋转值, 以实现视角移动时武器的Sway摇摆数值变化
	UFUNCTION(BlueprintOverride)
	void Tick(float DeltaSeconds)
	{
		UpdateWeaponSawyRotation(true, 0.f);
		UpdateWeaponSawyRotation(false, 0.f);
	}

	void UpdateWeaponSawyRotation(bool bIsYaw, float InValue)
	{
		const float Value = Math::Clamp(InValue, -1.f, 1.f);
		FRotator NewRotation;
		const float Offset = WeaponRotationRatio * Value;
		if (bIsYaw)
		{
			// NewRotation = FRotator(Offset, Offset, CurrentWeaponRotation.Roll);
			NewRotation = FRotator(CurrentWeaponSawyRotation.Pitch, Offset, CurrentWeaponSawyRotation.Roll);
		}
		else
		{
			// NewRotation = FRotator(CurrentWeaponRotation.Pitch, CurrentWeaponRotation.Yaw, Offset);
			NewRotation = FRotator(-Offset, CurrentWeaponSawyRotation.Yaw, CurrentWeaponSawyRotation.Roll);
		}
		CurrentWeaponSawyRotation = Math::RInterpTo(CurrentWeaponSawyRotation, NewRotation, Gameplay::GetWorldDeltaSeconds(), WeaponRotationInterp);
	}
}