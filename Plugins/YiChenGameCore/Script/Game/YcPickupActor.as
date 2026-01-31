/**
 * 可拾取Acotr
 * 通过配置YcPickupableComp的StaticInventory来指定拾取这个Acotr后要给玩家Inventory添加的Items
 */
class AYcPickupActor : AActor
{
	/** 提供可交互能力 */
	UPROPERTY(DefaultComponent)
	UYcInteractableComponent YcInteractableComp;

	/** 配置可供拾取的Item数据 */
	UPROPERTY(DefaultComponent)
	UYcPickupableComponent YcPickupableComp;

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		// 在所有端都请求加载Item资产
		RequestLoadItemsAssetAsync();
	}

	// 请求异步加载关联的Item资产
	void RequestLoadItemsAssetAsync()
	{
		for (auto& ItemTemp : YcPickupableComp.StaticInventory.Templates)
		{
			YcInventory::LoadItemDefDataAssetAsync(ItemTemp.ItemRegistryId);
		}
	}
}