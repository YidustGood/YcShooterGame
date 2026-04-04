/**
 * 将物品拾取至Avator背包中的技能
 * 注意只会拾取到背包中，不会执行其他任何操作
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
	 * 将物品拾取至Avatar背包中的技能
	 */
	TArray<UYcInventoryItemInstance> PickupItem()
	{
		TArray<UYcInventoryItemInstance> AddedItemInstances;
		// 仅在权威端执行拾取逻辑
		if (!HasAuthority())
			return AddedItemInstances;

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

		// 有有效的交互对象才继续
		FIndicatorDescriptor IndicatorDescriptor;
		if (!IndicatorComp.GetFirstIndicator(IndicatorDescriptor) || IndicatorDescriptor.TargetActor == nullptr)
			return AddedItemInstances;

		// 添加到库存组件
		YcPickupable::PickupFromActor(IndicatorDescriptor.TargetActor, InventoryManager, AddedItemInstances);
		return AddedItemInstances;
	}
}