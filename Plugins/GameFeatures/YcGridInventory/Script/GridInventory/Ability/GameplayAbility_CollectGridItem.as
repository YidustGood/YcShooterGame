class UGameplayAbility_CollectGridItem : UYcGameplayAbility
{
	bool bDestroyActor;
	default bDestroyActor = true;

	UFUNCTION(BlueprintOverride)
	void ActivateAbilityFromEvent(FGameplayEventData EventData)
	{
		// 确保无论收集是否成功，都能正常结束能力，避免后续无法再次触发
		auto WaitInputRelease = AngelscriptAbilityTask::WaitInputRelease(this);
		WaitInputRelease.OnRelease.AddUFunction(this, n"OnRelease");
		WaitInputRelease.ReadyForActivation();

		auto PC = GetControllerFromActorInfo();
		// 收集逻辑仅在服务端执行
		if (!PC.HasAuthority())
			return;
		auto Inventory = Cast<UGridInventoryManagerComponent>(PC.GetComponentByClass(UGridInventoryManagerComponent));
		check(Inventory != nullptr, "UGridInventoryManagerComponent not found");
		auto Indicator = UInteractionIndicatorComponent::Get(PC);

		if (!Indicator.Indicators.IsValidIndex(0))
		{
			// 当前没有有效交互目标
			EndAbility();
			return;
		}

		auto IndicatorObj = Indicator.Indicators[0].TargetActor;
		FYcInventoryPickup Pickup;
		if (!YcInventory::GetPickupInventory(IndicatorObj, Pickup))
		{
			Warning("交互物体无法收集：未获取到有效的 FYcInventoryPickup 数据");
			EndAbility();
			return;
		}

		// TODO: 后续可扩展对 ItemInstance 直接收集的支持
		for (auto ItemTemp : Pickup.Templates)
		{
			FIntPoint Tile;
			bool bRotated;
			if (Inventory.FindFirstFitPosition(ItemTemp.ItemRegistryId, Tile, bRotated))
			{
				if (Inventory.TryAddGridItemByDefinition(ItemTemp.ItemRegistryId, ItemTemp.StackCount, Tile, bRotated))
				{
					Log(f"位置{Tile}添加物品成功:{ItemTemp.ItemRegistryId.ItemName}");
				}
				else
				{
					AddFailed();
				}
			}
			else
			{
				AddFailed();
			}
		}
		auto IndicatorActor = Cast<AActor>(IndicatorObj);
		IndicatorActor.SetLifeSpan(0.01); // 下一帧销毁拾取物
		EndAbility();
	}

	UFUNCTION()
	private void AddFailed()
	{
		/**
		 * TODO:
		 * 一个可收集 Actor 可能包含多个掉落项。
		 * 目前任一项添加失败会保留 Actor（不销毁），后续可以细化为：
		 * 仅移除已被成功收集的部分，让其他玩家继续收集剩余物品。
		 */
		bDestroyActor = false;
		Log("未能添加物品：没有有效可放置位置");
		EndAbility();
	}

	UFUNCTION()
	private void OnRelease(float32 TimeHeld)
	{
		EndAbility();
	}
}
