
/** 
 * 动画通知
 * 设置主弹夹可视性
 */
class UAnimNotify_MagazineSetVisiblity : UAnimNotify
{
	UPROPERTY(EditAnywhere, EditInstanceOnly)
	bool bVisibility;

	UFUNCTION(BlueprintOverride)
	FString GetNotifyName() const
	{
		return "Set Magazine Visibility (YcShooterCore)";
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

		WeaponComp.SetMagazineVisibility(bVisibility);

		return true;
	}
}