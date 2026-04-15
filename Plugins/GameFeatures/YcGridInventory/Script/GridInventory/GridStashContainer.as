// 局外仓库容器：复用GridItemContainerBase的交互与库存能力，但不做静态模板初始化
class AGridStashContainer : AGridItemContainerBase
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MetaInventory")
	FString StashContainerId = "DefaultStash";

	default bEnableContainerSearch = false;

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		// 局外仓库不需要搜索玩法
		InventoryManager.bEnableContainerSearch = false;
		InventoryManager.bAllowDirectContainerInteraction = true;
		// 不调用父类BeginPlay，避免按静态模板向仓库灌默认物品
	}
}