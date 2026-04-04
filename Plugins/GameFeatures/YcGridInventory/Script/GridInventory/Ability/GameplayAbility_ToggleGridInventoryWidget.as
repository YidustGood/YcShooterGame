/**
 * 打开/关闭玩家网格库存UI的GA
 */
class UGameplayAbility_ToggleGridInventoryWidget : UYcGameplayAbility_ShowWidget
{
	private FGameplayMessageListenerHandle GridItemContainerMsgHandle;
	// 缓存容器开关消息，等待主Widget创建完成后再推送
	private TOptional<FGridItemContainerMessage> CachedContainerMessage;

	UFUNCTION(BlueprintOverride)
	void OnAbilityAddedEvent()
	{
		Super::OnAbilityAddedEvent();
		GridItemContainerMsgHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			GameplayTags::Yc_Inventory_Message_Grid_Container,
			this,
			n"OnReceivedGridItemContainerMsg",
			FGridItemContainerMessage(),
			EGameplayMessageMatch::ExactMatch);
	}

	UFUNCTION(BlueprintOverride)
	void OnAbilityRemovedEvent()
	{
		Super::OnAbilityRemovedEvent();
		GridItemContainerMsgHandle.Unregister();
	}

	UFUNCTION()
	void OnReceivedGridItemContainerMsg(FGameplayTag ActualTag, FGridItemContainerMessage Data)
	{
		// 只处理发给当前玩家的消息，避免监听服误开他人UI
		if (Data.Player != nullptr && Data.Player != GetAvatarActorFromActorInfo())
		{
			return;
		}

		// 如果当前Ability已结束，或收到关闭消息，则结束
		if (SpawnedWidget != nullptr || !Data.bIsOpen)
		{
			EndAbility();
			return;
		}

		// 缓存消息，在Widget创建完成后再推送
		CachedContainerMessage.Set(Data);
		if (Data.bIsOpen)
		{
			GetAbilitySystemComponentFromActorInfo().TryActivateAbilityByClass(this.GetClass());
		}
	}

	void AfterPush(UCommonActivatableWidget UserWidget) override
	{
		Super::AfterPush(UserWidget);
		// Widget异步加载完成后再推送容器消息，确保界面已可接收
		if (CachedContainerMessage.IsSet())
		{
			UGameplayMessageSubsystem::Get().BroadcastMessage(GameplayTags::Yc_Inventory_Message_Grid_Container_Push, CachedContainerMessage.GetValue());
			CachedContainerMessage.Reset();
		}
	}

	UFUNCTION(BlueprintOverride)
	void OnEndAbilityEvent()
	{
		Super::OnEndAbilityEvent();
		CachedContainerMessage.Reset();
	}
}



