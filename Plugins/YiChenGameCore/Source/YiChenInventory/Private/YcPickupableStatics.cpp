// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcPickupableStatics.h"

#include "YcInventoryManagerComponent.h"
#include "YcPickupable.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcPickupableStatics)

UYcPickupableStatics::UYcPickupableStatics()
	: Super(FObjectInitializer::Get())
{
}

TScriptInterface<IYcPickupable> UYcPickupableStatics::GetFirstPickupableFromActor(AActor* Actor)
{
	// 如果Actor直接实现了IYcPickupable, 那么直接返回
	TScriptInterface<IYcPickupable> PickupableActor(Actor);
	if (PickupableActor)
	{
		return PickupableActor;
	}
	
	// 如果Actor没有直接实现IYcPickupable, 它可能有一个实现了IYcPickupable的组件
	TArray<UActorComponent*> PickupableComponents = Actor ? Actor->GetComponentsByInterface(UYcPickupable::StaticClass()) : TArray<UActorComponent*>();
	if (PickupableComponents.Num() > 0)
	{
		// 这里只简单的返回第一个接口引用, 如果用户需要更复杂的拾取区分, 需要在其他地方解决
		return TScriptInterface<IYcPickupable>(PickupableComponents[0]);
	}

	return TScriptInterface<IYcPickupable>();
}

void UYcPickupableStatics::AddPickupToInventory(UYcInventoryManagerComponent* InventoryComponent,
	const TScriptInterface<IYcPickupable> Pickup)
{
	if (!InventoryComponent || !Pickup) return;
	
	const FYcInventoryPickup& PickupInventory = Pickup->GetPickupInventory();

	// 1. 通过ItemRegistryId添加（从DataRegistry获取物品定义）
	for (const FYcPickupTemplate& Template : PickupInventory.Templates)
	{
		InventoryComponent->AddItem(Template.ItemRegistryId, Template.StackCount);
	}

	// 2. 通过ItemInstance添加
	for (const FYcPickupInstance& Instance : PickupInventory.Instances)
	{
		// @TODO 这里StackCount传入1符合业务需求吗
		InventoryComponent->AddItemInstance(Instance.Item, 1);
	}
}