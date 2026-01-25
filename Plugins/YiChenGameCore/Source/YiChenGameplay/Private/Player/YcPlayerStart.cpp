// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Player/YcPlayerStart.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcPlayerStart)

// Sets default values
AYcPlayerStart::AYcPlayerStart()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AYcPlayerStart::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AYcPlayerStart::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

