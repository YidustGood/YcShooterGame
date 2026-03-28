// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcPickupableStatics.h"

#include "YcInventoryManagerComponent.h"
#include "YcPickupable.h"
#include "YcPickupableComponent.h"


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

	// 3. 触发拾取事件委托
	// 尝试获取拾取组件并广播事件
	if (UObject* PickupObject = Pickup.GetObject())
	{
		if (UYcPickupableComponent* PickupComp = Cast<UYcPickupableComponent>(PickupObject))
		{
			// 获取拾取者Pawn
			APawn* InstigatorPawn = Cast<APawn>(InventoryComponent->GetOwner());
			if (!InstigatorPawn && InventoryComponent->GetOwner())
			{
				// 如果Owner不是Pawn，尝试从Controller获取
				if (AController* Controller = Cast<AController>(InventoryComponent->GetOwner()))
				{
					InstigatorPawn = Controller->GetPawn();
				}
			}
			
			PickupComp->BroadcastPickedUp(InstigatorPawn);
		}
	}
}

bool UYcPickupableStatics::PickupFromActor(AActor* Actor, UYcInventoryManagerComponent* InventoryComponent, TArray<UYcInventoryItemInstance*>& OutAddedInstances)
{
	OutAddedInstances.Empty();
	
	// 1. 从Actor查找IYcPickupable接口
	TScriptInterface<IYcPickupable> Pickup = GetFirstPickupableFromActor(Actor);
	if (!Pickup)
	{
		return false;
	}
	
	if (!InventoryComponent)
	{
		return false;
	}
	
	const FYcInventoryPickup& PickupInventory = Pickup->GetPickupInventory();
	
	// 2. 通过ItemRegistryId添加（从DataRegistry获取物品定义）
	for (const FYcPickupTemplate& Template : PickupInventory.Templates)
	{
		if (UYcInventoryItemInstance* NewInstance = InventoryComponent->AddItem(Template.ItemRegistryId, Template.StackCount))
		{
			OutAddedInstances.Add(NewInstance);
		}
	}
	
	// 3. 通过ItemInstance添加
	for (const FYcPickupInstance& Instance : PickupInventory.Instances)
	{
		if (InventoryComponent->AddItemInstance(Instance.Item, 1))
		{
			OutAddedInstances.Add(Instance.Item);
		}
	}
	
	// 4. 触发拾取事件委托
	if (UObject* PickupObject = Pickup.GetObject())
	{
		if (UYcPickupableComponent* PickupComp = Cast<UYcPickupableComponent>(PickupObject))
		{
			// 获取拾取者Pawn
			APawn* InstigatorPawn = Cast<APawn>(InventoryComponent->GetOwner());
			if (!InstigatorPawn && InventoryComponent->GetOwner())
			{
				// 如果Owner不是Pawn，尝试从Controller获取
				if (AController* Controller = Cast<AController>(InventoryComponent->GetOwner()))
				{
					InstigatorPawn = Controller->GetPawn();
				}
			}
			
			PickupComp->BroadcastPickedUp(InstigatorPawn);
		}
	}
	
	return OutAddedInstances.Num() > 0;
}