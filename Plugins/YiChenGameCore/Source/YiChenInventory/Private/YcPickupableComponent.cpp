// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcPickupableComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcPickupableComponent)

UYcPickupableComponent::UYcPickupableComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

FYcInventoryPickup UYcPickupableComponent::GetPickupInventory() const
{
	return StaticInventory;
}