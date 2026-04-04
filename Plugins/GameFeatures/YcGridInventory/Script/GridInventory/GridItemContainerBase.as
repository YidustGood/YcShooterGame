// 地图物资容器基类：内含一个网格库存组件，可按配置启用搜索机制
class AGridItemContainerBase : AActor
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Search")
	bool bEnableContainerSearch = false;

	UPROPERTY(DefaultComponent)
	UGridInventoryManagerComponent InventoryManager;

	UPROPERTY(DefaultComponent)
	UYcPickupableComponent PickupableComp;

	UPROPERTY(DefaultComponent)
	UYcInteractableComponent InteractableComp;

	UPROPERTY(DefaultComponent)
	UStaticMeshComponent DisplayMesh;

	default bReplicates = true;
	default NetCullDistanceSquared = 500000;

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		InventoryManager.bEnableContainerSearch = bEnableContainerSearch;

		if (HasAuthority())
		{
			InitContainer();
		}
	}
	// 将静态配置中的初始物品按可放置位置装入容器
	void InitContainer()
	{
		for (auto ItemDef : PickupableComp.StaticInventory.Templates)
		{
			if (ItemDef.ItemRegistryId.ItemName.IsNone() || ItemDef.StackCount <= 0)
				continue;

			FIntPoint Tile;
			bool bRotated;
			if (InventoryManager.FindFirstFitPosition(ItemDef.ItemRegistryId, Tile, bRotated))
			{
				InventoryManager.TryAddGridItemByDefinition(ItemDef.ItemRegistryId, ItemDef.StackCount, Tile, bRotated);
			}
		}
	}
}



