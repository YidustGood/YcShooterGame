// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcEquipmentActorComponent.h"
#include "YcEquipmentInstance.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcEquipmentActorComponent)

UYcEquipmentActorComponent::UYcEquipmentActorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UYcEquipmentActorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	// 只在初始化时同步一次，后续不会修改
	DOREPLIFETIME(UYcEquipmentActorComponent, EquipmentInst);
	DOREPLIFETIME(UYcEquipmentActorComponent, EquipmentTags);
}

UYcEquipmentInstance* UYcEquipmentActorComponent::GetEquipmentFromActor(const AActor* Actor)
{
	if (!IsValid(Actor)) return nullptr;
	
	if (UYcEquipmentActorComponent* Comp = Actor->FindComponentByClass<UYcEquipmentActorComponent>())
	{
		return Comp->GetOwningEquipment();
	}
	return nullptr;
}

void UYcEquipmentActorComponent::OnRep_EquipmentInst()
{
	OnEquipmentRep.Broadcast(GetOwningEquipment());
}