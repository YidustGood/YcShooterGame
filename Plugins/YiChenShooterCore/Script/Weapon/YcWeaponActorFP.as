class AYcWeaponActorFP : AYcWeaponActorScript
{
	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		/**
		 * 当武器装备时EquipmentInst会把Actor的所有模型都设置为可见
		 * 这个委托是在设置为可见后执行的, 我们特殊处理武器第二弹夹的可见性, 避免意外的显示出第二弹夹
		 */
		EquipmentActorComponent.OnEquipped.AddUFunction(YcWeaponComponent, n"HideReserveMagazine");
	}
}