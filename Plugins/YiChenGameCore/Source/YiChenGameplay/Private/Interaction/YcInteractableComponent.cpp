// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Interaction/YcInteractableComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcInteractableComponent)

UYcInteractableComponent::UYcInteractableComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UYcInteractableComponent::GatherInteractionOptions(const FYcInteractionQuery& InteractQuery,
	FYcInteractionOptionBuilder& InteractionBuilder)
{
	InteractionBuilder.AddInteractionOption(Option);
}

void UYcInteractableComponent::OnPlayerFocusBegin(const FYcInteractionQuery& InteractQuery)
{
	OnPlayerFocusBeginEvent.Broadcast(InteractQuery);
}

void UYcInteractableComponent::OnPlayerFocusEnd(const FYcInteractionQuery& InteractQuery)
{
	OnPlayerFocusEndEvent.Broadcast(InteractQuery);
}