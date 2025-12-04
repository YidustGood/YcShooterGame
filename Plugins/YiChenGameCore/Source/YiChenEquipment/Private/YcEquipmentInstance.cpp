// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcEquipmentInstance.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcEquipmentInstance)

UYcEquipmentInstance::UYcEquipmentInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UYcEquipmentInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UYcEquipmentInstance, Instigator);
	DOREPLIFETIME(UYcEquipmentInstance, SpawnedActors);
}

void UYcEquipmentInstance::OnRep_Instigator(const UYcInventoryItemInstance* LastInstigator)
{
	// @TODO OnRep_Instigator
}