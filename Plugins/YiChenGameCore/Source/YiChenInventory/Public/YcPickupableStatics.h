// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "YcPickupableStatics.generated.h"

class IYcPickupable;
class UYcInventoryManagerComponent;

/**
 * 可拾取系统工具函数库
 */
UCLASS()
class YICHENINVENTORY_API UYcPickupableStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UYcPickupableStatics();
	
	/**
	 * 从Actor对象获取首个拾取接口
	 * 内部会先从Actor对象进行查找, 如果没有则查找是否有实现了IYcPickupable的组件
	 * @param Actor 要搜索的Actor
	 * @return 拾取接口的脚本引用，如果Actor没有实现IYcPickupable接口则返回空引用
	 */
	UFUNCTION(BlueprintPure)
	static TScriptInterface<IYcPickupable> GetFirstPickupableFromActor(AActor* Actor);
	
	/**
	 * 拾取物品到库存中，目的拾取机制是拾取一个对象就会在库存中新增一个Item，并记录它的stack count，但是stack count不会自动寻找同类型然后叠加stack count
	 * @param InventoryComponent 目标库存组件
	 * @param Pickup ISCPickupable接口，通过这个接口获取到FSCInventoryPickup，并将其中定义好的物品，生成并添加到目标库存中
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta = (WorldContext = "Ability"))
	static void AddPickupToInventory(UYcInventoryManagerComponent* InventoryComponent, TScriptInterface<IYcPickupable> Pickup);
};