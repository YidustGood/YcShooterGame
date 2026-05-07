// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcPickupableComponent.h"

#include "Net/UnrealNetwork.h"
#include "YcInventoryItemInstance.h"
#include "YcInventoryLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcPickupableComponent)

UYcPickupableComponent::UYcPickupableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	bReplicateUsingRegisteredSubObjectList = true;
}

void UYcPickupableComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UYcPickupableComponent, StaticInventory);
}

FYcInventoryPickup UYcPickupableComponent::GetPickupInventory() const
{
	return StaticInventory;
}

void UYcPickupableComponent::BroadcastPickedUp(AActor* Instigator)
{
	OnPickedUp.Broadcast(Instigator);
}

void UYcPickupableComponent::SetStaticInventory(const FYcInventoryPickup& NewStaticInventory)
{
	StaticInventory = NewStaticInventory;
	OnRep_StaticInventory();
}

void UYcPickupableComponent::OnRep_StaticInventory()
{
	AActor* OwnerActor = GetOwner();
	if (OwnerActor && OwnerActor->HasAuthority())
	{
		for (const FYcPickupInstance& PickupInstance : StaticInventory.Instances)
		{
			if (IsValid(PickupInstance.Item))
			{
				OwnerActor->AddReplicatedSubObject(PickupInstance.Item);
			}
		}
	}

	for (const FYcPickupInstance& PickupInstance : StaticInventory.Instances)
	{
		if (IsValid(PickupInstance.Item))
		{
			UYcInventoryLibrary::LoadItemDefDataAssetAsync(this, PickupInstance.Item->GetItemRegistryId());
		}
	}

	for (const FYcPickupTemplate& ItemTemplate : StaticInventory.Templates)
	{
		UYcInventoryLibrary::LoadItemDefDataAssetAsync(this, ItemTemplate.ItemRegistryId);
	}

	OnPickupInventoryChanged.Broadcast();
}
