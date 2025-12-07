// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameModes/YcGameState.h"
#include "GameModes/YcExperienceManagerComponent.h"
#include "GameModes/YcGameMode.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameState)

AYcGameState::AYcGameState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	
	ExperienceManagerComponent = CreateDefaultSubobject<UYcExperienceManagerComponent>(TEXT("ExperienceManagerComponent"));
}