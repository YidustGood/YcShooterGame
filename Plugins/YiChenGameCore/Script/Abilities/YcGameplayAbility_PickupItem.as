/**
 * 将物品拾取到Avatar背包中的技能
 * 注意：仅负责“拾取到背包”，不处理其它使用逻辑。
 */
class UYcGameplayAbility_PickupItem : UYcGameplayAbility
{
	UFUNCTION(BlueprintOverride)
	void ActivateAbility()
	{
		PickupItem();
		EndAbility();
	}

	UFUNCTION(BlueprintOverride)
	void ActivateAbilityFromEvent(FGameplayEventData EventData)
	{
		PickupItem();
		EndAbility();
	}

	/**
	 * 将物品拾取至Avatar背包
	 */
	TArray<UYcInventoryItemInstance> PickupItem()
	{
		TArray<UYcInventoryItemInstance> AddedItemInstances;

		// 仅在服务端执行拾取逻辑
		if (!HasAuthority())
		{
			return AddedItemInstances;
		}

		auto InventoryManager = YcInventory::GetInventoryManagerComponent(GetAvatarActorFromActorInfo());
		if (InventoryManager == nullptr)
		{
			Warning("UYcGameplayAbility_PickupItem: InventoryManager is nullptr");
			return AddedItemInstances;
		}

		auto IndicatorComp = UInteractionIndicatorComponent::FindInteractionIndicatorComponent(GetControllerFromActorInfo());
		if (IndicatorComp == nullptr)
		{
			Warning("UYcGameplayAbility_PickupItem: IndicatorComp is nullptr");
			return AddedItemInstances;
		}

		FIndicatorDescriptor IndicatorDescriptor;
		if (!IndicatorComp.GetFirstIndicator(IndicatorDescriptor) || IndicatorDescriptor.TargetActor == nullptr)
		{
			return AddedItemInstances;
		}

		// 执行拾取并将物品加入库存
		YcPickupable::PickupFromActor(IndicatorDescriptor.TargetActor, InventoryManager, AddedItemInstances);

		// 拾取成功后，尝试调用AYcPickupActor的拾取后处理（可配置是否自动销毁）
		if (AddedItemInstances.Num() > 0)
		{
			auto PickupActor = Cast<AYcPickupActor>(IndicatorDescriptor.TargetActor);
			if (PickupActor != nullptr)
			{
				PickupActor.HandlePickedUp(GetAvatarActorFromActorInfo());
			}
		}

		return AddedItemInstances;
	}
}
