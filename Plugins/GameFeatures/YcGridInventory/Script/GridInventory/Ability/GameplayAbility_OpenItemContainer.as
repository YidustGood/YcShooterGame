/** 仅本地执行：为当前交互玩家打开容器网格界面，并触发服务端搜索会话 */
class UGameplayAbility_OpenItemContainer : UYcGameplayAbility
{
	// 容器库存界面WidgetClass
	UPROPERTY(EditAnywhere)
	TSubclassOf<UGridInventoryWidget> ContainerWidgetClass;

	UFUNCTION(BlueprintOverride)
	void ActivateAbilityFromEvent(FGameplayEventData EventData)
	{
		// 该能力仅用于本地显示容器Widget
		// 监听服与远端代理不应打开本地UI
		if (!GetControllerFromActorInfo().IsLocalController())
		{
			return;
		}
		if (ContainerWidgetClass == nullptr)
		{
			EndAbility();
			return;
		}

		auto Inventory = Cast<UGridInventoryManagerComponent>(EventData.Target.GetComponentByClass(UGridInventoryManagerComponent));
		if (Inventory == nullptr)
		{
			EndAbility();
			return;
		}

		// 通知服务端为当前玩家开启容器搜索会话（容器未启用搜索时会自动忽略）
		auto PlayerInventory = Cast<UGridInventoryManagerComponent>(YcInventory::GetInventoryManagerComponent(GetAvatarActorFromActorInfo()));
		if (PlayerInventory != nullptr)
		{
			PlayerInventory.ServerStartContainerSearchSession(Inventory);
		}

		FGridItemContainerMessage ContainerMessage;
		ContainerMessage.Player = GetAvatarActorFromActorInfo();
		ContainerMessage.ContainerInventory = Inventory;
		ContainerMessage.ContainerWidgetClass = ContainerWidgetClass;
		ContainerMessage.bIsOpen = true;
		UGameplayMessageSubsystem::Get().BroadcastMessage(GameplayTags::Yc_Inventory_Message_Grid_Container, ContainerMessage);

		EndAbility();
	}
}



