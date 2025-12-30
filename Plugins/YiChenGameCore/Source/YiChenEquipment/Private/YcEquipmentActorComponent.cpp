// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcEquipmentActorComponent.h"
#include "YcEquipmentInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcEquipmentActorComponent)

UYcEquipmentActorComponent::UYcEquipmentActorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	bWantsInitializeComponent = false;
}

UYcEquipmentInstance* UYcEquipmentActorComponent::GetEquipmentFromActor(AActor* Actor)
{
	if (!IsValid(Actor)) return nullptr;
	
	if (UYcEquipmentActorComponent* Comp = Actor->FindComponentByClass<UYcEquipmentActorComponent>())
	{
		return Comp->GetOwningEquipment();
	}
	return nullptr;
}