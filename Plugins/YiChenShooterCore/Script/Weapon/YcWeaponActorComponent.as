/**
 * 武器Actor辅助组件
 * 提供一些辅助性函数, 和处理一些辅助内容(弹夹可实现处理等), 主要是用于代替各种操作、数据获取的接口的, 因为AS不支持接口
 */
class UYcWeaponActorComponent : UActorComponent
{
	UPROPERTY()
	AYcWeaponActor WeaponActor;

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		WeaponActor = Cast<AYcWeaponActor>(GetOwner());
		if (WeaponActor == nullptr)
		{
			Error("请确保UYcWeaponComponent是挂载在AYcWeaponAcotr及其子类上的!");
			return;
		}
	}

	/**
	 * 设置主弹夹可视性, 一般是再动画通知中调用, 以协调换弹动画时两个弹夹的可视性
	 */
	UFUNCTION()
	void SetMagazineVisibility(bool bVisibility)
	{
		UStaticMeshComponent Magazine = WeaponActor.GetAttachmentMeshForSlot(GameplayTags::Attachment_Slot_Magazine);
		Magazine.SetVisibility(bVisibility);
	}

	/**
	 * 设置备用弹夹可视性, 一般是再动画通知中调用, 以协调换弹动画时两个弹夹的可视性
	 */
	UFUNCTION()
	void SetReserveMagazineVisibility(bool bVisibility)
	{
		UStaticMeshComponent MagazineReserve = WeaponActor.GetAttachmentMeshForSlot(GameplayTags::Attachment_Slot_MagazineReserve);
		MagazineReserve.SetVisibility(bVisibility);
	}

	/** 隐藏备用弹夹 */
	UFUNCTION()
	void HideReserveMagazine(UYcEquipmentInstance EquipmentInst)
	{
		UStaticMeshComponent MagazineReserve = WeaponActor.GetAttachmentMeshForSlot(GameplayTags::Attachment_Slot_MagazineReserve);
		if (MagazineReserve == nullptr)
			return;

		MagazineReserve.SetVisibility(false);
	}
}