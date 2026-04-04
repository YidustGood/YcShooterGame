/**
 * 给物品添加数据数据的技能
 * ItemInstance有一个TagsStack容器可以基于GameplayTag存储整数型的数值
 * 还有一个FloatTagsStack是浮点型的版本, 本技能可以同时添加整数型和浮点型的数值
 * 例如武器物品实例的子弹数量可以添加到TagsStack中
 */
class UAbility_ItemDataSupply : UYcGameplayAbility
{

	// 要添加的物品数据标签和数量
	UPROPERTY()
	TMap<FGameplayTag, int32> ItemDataSupplyMap;

	// 要添加的物品数据标签和数值
	UPROPERTY()
	TMap<FGameplayTag, float> ItemDataSupplyFloatMap;

	// 要添加的目标物品所需的标签, 用于匹配物品是否符合添加条件, 例如只给XX类型子弹的武器物品添加XX子弹
	UPROPERTY()
	FGameplayTagContainer ItemNeedTag;

	// 用于控制可以给多少个Item添加ItemDataSupplyCount中的数据
	UPROPERTY(meta = (ClampMin = "1"))
	int32 MaxItemSupplyCount = 1;

	UFUNCTION(BlueprintOverride)
	void ActivateAbility()
	{
		if (ItemDataSupplyMap.Num() == 0)
		{
			Warning("ItemDataSupplyMap is empty");
			EndAbility();
			return;
		}

		// 打印交互物体的名称, 如果用的UYcInteractableComponent那么CurrentSourceObject是组件, Outer通常就是交互物体Actor
		// 因为这个可交互物体一般并不会是某个玩家持有的, 也可以通过转换成组件再通过GetOwner来获取交互物体的Owner
		// Print("交互物体: " + CurrentSourceObject.GetOuter());

		auto AvatarActor = GetAvatarActorFromActorInfo();
		auto QuickBarComponent = UYcQuickBarComponent::FindQuickBarComponent(AvatarActor);
		auto SlotItms = QuickBarComponent.GetSlots();
		for (auto Item : SlotItms)
		{
			if (!Item.HasMatchingGameplayTag(ItemNeedTag))
				continue;

			for (auto ItemDataTag : ItemDataSupplyMap)
			{
				Item.AddStatTagStack(ItemDataTag.Key, ItemDataTag.Value);
				Print("Add Stat Tag Stack: " + ItemDataTag.Key.ToString() + " x " + ItemDataTag.Value);
			}

			for (auto ItemDataTag : ItemDataSupplyFloatMap)
			{
				Item.AddFloatTagStack(ItemDataTag.Key, ItemDataTag.Value);
				Print("Add Float Tag Stack: " + ItemDataTag.Key.ToString() + " x " + ItemDataTag.Value);
			}

			if (--MaxItemSupplyCount == 0)
			{
				break;
			}
		}

		auto Actor = Cast<AActor>(CurrentSourceObject.GetOuter());
		Actor.SetLifeSpan(0.1);
		// @TOOD 销毁Actor, 例如是弹药箱Actor赋予的玩家这个GA，那么这个GA执行完成代表弹药箱被拾取，那么就销毁这个弹药箱Actor
		EndAbility();
	}

	UFUNCTION(BlueprintOverride)
	void OnAbilityAdded()
	{
		// 打印添加的技能名称
		Print("添加技能: " + GetName());
	}

	UFUNCTION(BlueprintOverride)
	void OnAbilityRemoved()
	{
		// 打印移除的技能名称
		Print("移除技能: " + GetName());
	}
}