// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameModes/YcGameState.h"

#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameModes/YcExperienceManagerComponent.h"
#include "GameplayCommon/YcGameVerbMessage.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameState)

AYcGameState::AYcGameState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	
	ExperienceManagerComponent = CreateDefaultSubobject<UYcExperienceManagerComponent>(TEXT("ExperienceManagerComponent"));
}

void AYcGameState::MulticastMessageToClients_Implementation(const FYcGameVerbMessage Message)
{
	if (GetNetMode() == NM_Client)
	{
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(Message.Verb, Message);
	}
}

void AYcGameState::MulticastReliableMessageToClients_Implementation(const FYcGameVerbMessage Message)
{
	MulticastMessageToClients_Implementation(Message);
}