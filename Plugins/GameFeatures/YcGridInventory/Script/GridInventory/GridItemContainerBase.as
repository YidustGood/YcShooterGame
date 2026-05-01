// 地图物资容器基类：内含一个网格库存组件，可按配置启用搜索机制
class AGridItemContainerBase : AActor
{
	// 是否启用容器搜索玩法。开启后，玩家需要先搜索容器，才能逐步揭示其中的物品。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Search")
	bool bEnableContainerSearch = false;

	// 容器初始物品的刷新策略。默认保持旧版静态模板行为，避免影响已有关卡。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot")
	EYcContainerItemSpawnStrategy ItemSpawnStrategy = EYcContainerItemSpawnStrategy::LegacyStaticInventory;

	// 当刷新策略为随机掉落池时，这里配置要使用的掉落池 DataRegistryId。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (RegistryType = "LootPool"))
	FDataRegistryId LootPoolId;

	// 随机掉落池的抽取次数覆盖值。小于等于 0 时回退到掉落池自身配置的默认次数。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot", meta = (ClampMin = "0"))
	int32 SpawnCountOverride = 0;

	// 容器自身的网格库存管理组件，负责落格、搜索和物品存放。
	UPROPERTY(DefaultComponent)
	UGridInventoryManagerComponent InventoryManager;

	// 旧版静态物品配置所在的拾取组件，兼容策略下仍然复用这里的数据。
	UPROPERTY(DefaultComponent)
	UYcPickupableComponent PickupableComp;

	// 提供容器交互能力的组件。
	UPROPERTY(DefaultComponent)
	UYcInteractableComponent InteractableComp;

	// 容器在场景中的可视化网格体组件。
	UPROPERTY(DefaultComponent)
	UStaticMeshComponent DisplayMesh;

	default bReplicates = true;
	default NetCullDistanceSquared = 500000;

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		// 先把搜索开关同步给库存组件，再由服务端执行容器初始化。
		InventoryManager.bEnableContainerSearch = bEnableContainerSearch;

		if (HasAuthority())
		{
			InitContainer();
		}
	}

	// 按当前配置的刷新策略向容器中填充初始物品。
	// 这里只在服务端调用一次，避免重复初始化。
	void InitContainer()
	{
		// 统一从拾取配置中取出旧版静态数据，再交给容器初始化库按策略处理。
		FYcInventoryPickup PickupInventory = PickupableComp.StaticInventory;
		YcGridItemContainer::PopulateContainerInventory(
			InventoryManager,
			PickupInventory,
			ItemSpawnStrategy,
			LootPoolId,
			SpawnCountOverride);
	}
}
