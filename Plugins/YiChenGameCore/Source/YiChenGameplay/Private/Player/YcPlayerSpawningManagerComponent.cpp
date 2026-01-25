// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Player/YcPlayerSpawningManagerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcPlayerSpawningManagerComponent)

// Sets default values for this component's properties
UYcPlayerSpawningManagerComponent::UYcPlayerSpawningManagerComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UYcPlayerSpawningManagerComponent::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}


// Called every frame
void UYcPlayerSpawningManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType,
                                                      FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

