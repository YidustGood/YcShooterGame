/**
 * 动画通知
 * 用于控制武器第二弹夹可视性的通知
 */
class UAnimNotify_MagazineReserveSetVisiblity : UAnimNotify
{
	UPROPERTY(EditAnywhere, EditInstanceOnly)
	bool bVisibility;

	UFUNCTION(BlueprintOverride)
	FString GetNotifyName() const
	{
		return "Set Reserve Magazine Visibility (YcShooterCore)";
	}

	UFUNCTION(BlueprintOverride)
	bool Notify(USkeletalMeshComponent MeshComp, UAnimSequenceBase Animation,
				FAnimNotifyEventReference EventReference) const
	{
		AYcWeaponActor WeaponActor = YcWeapon::GetPlayerFirstPersonWeaponActor(MeshComp.GetOwner());
		if (WeaponActor == nullptr)
			return true;

		auto WeaponComp = WeaponActor.GetComponentByClass(UYcWeaponActorComponent);
		if (WeaponComp == nullptr)
			return true;

		WeaponComp.SetReserveMagazineVisibility(bVisibility);

		return true;
	}
}