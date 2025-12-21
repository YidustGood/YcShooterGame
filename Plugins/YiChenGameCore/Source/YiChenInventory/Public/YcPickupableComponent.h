// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcPickupable.h"
#include "Components/ActorComponent.h"
#include "YcPickupableComponent.generated.h"

/**
 * 为Actor提供可拾取功能的组件
 * 本质上就是提供一个拾取物配置数据, 然后通过IYcPickupable提供获取拾取物配置数据的接口, 然后在其它地方可以通过接口拿到数据往库存中进行添加
 * 在插件中默认是实现一个YcGameplayAbility_Interact的技能来获取可拾取物数据并操作玩家库存组件进行物品添加
 */
UCLASS(ClassGroup=(YiChenInventory), meta=(BlueprintSpawnableComponent))
class YICHENINVENTORY_API UYcPickupableComponent : public UActorComponent, public IYcPickupable
{
	GENERATED_BODY()
public:
	UYcPickupableComponent();
	
	// IYcPickupable interface.
	virtual FYcInventoryPickup GetPickupInventory() const override;
	// ~IYcPickupable interface.
	
private:
	/** 这个拾取物被拾取后向库存添加的内容 -- 对应IYcPickupable接口的 GetPickupInventory() 函数*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Interactable, meta = (AllowPrivateAccess = true))
	FYcInventoryPickup StaticInventory;
};