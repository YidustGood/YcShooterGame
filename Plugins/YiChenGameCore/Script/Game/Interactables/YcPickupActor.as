/**
 * 可拾取Actor
 * 通过配置 YcPickupableComp.StaticInventory 指定拾取后添加到玩家背包中的物品。
 */
class AYcPickupActor : AActor
{
	/** 提供可交互能力 */
	UPROPERTY(DefaultComponent)
	UYcInteractableComponent YcInteractableComp;

	/** 配置可被拾取的物品数据 */
	UPROPERTY(DefaultComponent)
	UYcPickupableComponent YcPickupableComp;

	default bReplicates = true;
	default bReplicateUsingRegisteredSubObjectList = true;
	default YcInteractableComp.Option.InteractionAbilityToGrant = UYcGameplayAbility_PickupItem;

	/** 是否在拾取成功后自动销毁Actor（默认开启） */
	UPROPERTY(EditAnywhere, Category = "Pickup")
	bool bAutoDestroyOnPickedUp = true;

	/** 拾取成功后自动销毁延迟秒数（默认0.1秒） */
	UPROPERTY(EditAnywhere, Category = "Pickup", meta = (ClampMin = "0.0"))
	float AutoDestroyLifeSpan = 0.1f;

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		// 在所有端预加载对应物品资产
		RequestLoadItemsAssetAsync();
	}

	// 请求异步加载关联的Item资产
	void RequestLoadItemsAssetAsync()
	{
		for (auto& ItemTemp : YcPickupableComp.StaticInventory.Templates)
		{
			YcInventory::LoadItemDefDataAssetAsync(ItemTemp.ItemRegistryId);
		}

		for (auto& PickupInstance : YcPickupableComp.StaticInventory.Instances)
		{
			if (PickupInstance.Item != nullptr)
			{
				YcInventory::LoadItemDefDataAssetAsync(PickupInstance.Item.ItemRegistryId);
			}
		}
	}

	/**
	 * 拾取成功后的默认行为。
	 * - bAutoDestroyOnPickedUp=true 时，设置生命周期并自动销毁。
	 * - bAutoDestroyOnPickedUp=false 时，不做销毁。
	 */
	UFUNCTION(BlueprintCallable)
	void HandlePickedUp(AActor Instigator)
	{
		if (!HasAuthority())
		{
			return;
		}

		if (!bAutoDestroyOnPickedUp)
		{
			return;
		}

		SetLifeSpan(Math::Max(0.0f, AutoDestroyLifeSpan));
	}
}
