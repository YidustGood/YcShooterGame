// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "DynamicScreenUI/YcDynamicScreenUIRouting.h"

#include "DynamicScreenUI/YcPlayerScreenUIComponent.h"
#include "Engine/Engine.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDynamicScreenUIRouting)

namespace
{
	UYcPlayerScreenUIComponent* ResolvePlayerScreenUIComponent(APlayerController* TargetPlayerController)
	{
		if (!TargetPlayerController)
		{
			return nullptr;
		}

		return TargetPlayerController->FindComponentByClass<UYcPlayerScreenUIComponent>();
	}

	APlayerController* ResolvePlayerControllerFromState(APlayerState* PlayerState)
	{
		if (!PlayerState)
		{
			return nullptr;
		}

		if (APlayerController* PlayerController = Cast<APlayerController>(PlayerState->GetPlayerController()))
		{
			return PlayerController;
		}

		if (APawn* Pawn = Cast<APawn>(PlayerState->GetPawn()))
		{
			if (APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController()))
			{
				return PlayerController;
			}
		}

		return Cast<APlayerController>(PlayerState->GetOwner());
	}

	UWorld* ResolveRoutingWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}

		for (const FWorldContext& WorldContext : GEngine->GetWorldContexts())
		{
			UWorld* World = WorldContext.World();
			if (!World)
			{
				continue;
			}

			if (World->IsGameWorld())
			{
				return World;
			}
		}

		return nullptr;
	}
}

void UYcDynamicScreenUIRoutingLibrary::RouteShowToPlayer(APlayerController* TargetPlayerController, const FYcDynamicPlayerScreenUIShowRequest& Request)
{
	if (!TargetPlayerController || Request.WidgetKey.IsNone() || Request.WidgetClass.IsNull())
	{
		return;
	}

	UYcPlayerScreenUIComponent* ScreenUIComponent = ResolvePlayerScreenUIComponent(TargetPlayerController);
	if (!ScreenUIComponent)
	{
		return;
	}

	FYcDynamicPlayerScreenUIShowRequest RoutedRequest = Request;
	RoutedRequest.TargetPlayerController = nullptr;

	if (TargetPlayerController->IsLocalController())
	{
		ScreenUIComponent->EnqueueShowDynamicWidgetRequest(RoutedRequest);
	}
	else if (TargetPlayerController->HasAuthority())
	{
		ScreenUIComponent->ClientShowDynamicWidget(RoutedRequest);
	}
}

void UYcDynamicScreenUIRoutingLibrary::RouteUpdateToPlayer(APlayerController* TargetPlayerController, FName WidgetKey, const FInstancedStruct& Payload)
{
	if (!TargetPlayerController || WidgetKey.IsNone())
	{
		return;
	}

	UYcPlayerScreenUIComponent* ScreenUIComponent = ResolvePlayerScreenUIComponent(TargetPlayerController);
	if (!ScreenUIComponent)
	{
		return;
	}

	if (TargetPlayerController->IsLocalController())
	{
		ScreenUIComponent->EnqueueUpdateDynamicWidgetRequest(WidgetKey, Payload);
	}
	else if (TargetPlayerController->HasAuthority())
	{
		ScreenUIComponent->ClientUpdateDynamicWidget(WidgetKey, Payload);
	}
}

void UYcDynamicScreenUIRoutingLibrary::RouteHideToPlayer(APlayerController* TargetPlayerController, FName WidgetKey, const FInstancedStruct& Payload)
{
	if (!TargetPlayerController || WidgetKey.IsNone())
	{
		return;
	}

	UYcPlayerScreenUIComponent* ScreenUIComponent = ResolvePlayerScreenUIComponent(TargetPlayerController);
	if (!ScreenUIComponent)
	{
		return;
	}

	if (TargetPlayerController->IsLocalController())
	{
		ScreenUIComponent->EnqueueHideDynamicWidgetRequest(WidgetKey, Payload);
	}
	else if (TargetPlayerController->HasAuthority())
	{
		ScreenUIComponent->ClientHideDynamicWidget(WidgetKey, Payload);
	}
}

void UYcDynamicScreenUIRoutingLibrary::RouteShowToPlayers(const TArray<APlayerController*>& InTargetPlayerControllers, const FYcDynamicPlayerScreenUIShowRequest& Request)
{
	for (APlayerController* TargetPlayerController : InTargetPlayerControllers)
	{
		RouteShowToPlayer(TargetPlayerController, Request);
	}
}

void UYcDynamicScreenUIRoutingLibrary::RouteUpdateToPlayers(const TArray<APlayerController*>& InTargetPlayerControllers, FName WidgetKey, const FInstancedStruct& Payload)
{
	for (APlayerController* TargetPlayerController : InTargetPlayerControllers)
	{
		RouteUpdateToPlayer(TargetPlayerController, WidgetKey, Payload);
	}
}

void UYcDynamicScreenUIRoutingLibrary::RouteHideToPlayers(const TArray<APlayerController*>& InTargetPlayerControllers, FName WidgetKey, const FInstancedStruct& Payload)
{
	for (APlayerController* TargetPlayerController : InTargetPlayerControllers)
	{
		RouteHideToPlayer(TargetPlayerController, WidgetKey, Payload);
	}
}

void UYcDynamicScreenUIRoutingLibrary::RouteShowToAllPlayers(const FYcDynamicPlayerScreenUIShowRequest& Request, bool bIncludeBots)
{
	TArray<APlayerController*> TargetPlayerControllers;
	ResolveMatchPlayerControllers(TargetPlayerControllers, bIncludeBots);
	RouteShowToPlayers(TargetPlayerControllers, Request);
}

void UYcDynamicScreenUIRoutingLibrary::RouteUpdateToAllPlayers(FName WidgetKey, const FInstancedStruct& Payload, bool bIncludeBots)
{
	TArray<APlayerController*> TargetPlayerControllers;
	ResolveMatchPlayerControllers(TargetPlayerControllers, bIncludeBots);
	RouteUpdateToPlayers(TargetPlayerControllers, WidgetKey, Payload);
}

void UYcDynamicScreenUIRoutingLibrary::RouteHideToAllPlayers(FName WidgetKey, const FInstancedStruct& Payload, bool bIncludeBots)
{
	TArray<APlayerController*> TargetPlayerControllers;
	ResolveMatchPlayerControllers(TargetPlayerControllers, bIncludeBots);
	RouteHideToPlayers(TargetPlayerControllers, WidgetKey, Payload);
}

void UYcDynamicScreenUIRoutingLibrary::ResolveMatchPlayerControllers(TArray<APlayerController*>& OutPlayerControllers, bool bIncludeBots)
{
	OutPlayerControllers.Reset();

	UWorld* World = ResolveRoutingWorld();
	if (!World)
	{
		return;
	}

	AGameStateBase* GameState = World->GetGameState();
	if (!GameState)
	{
		return;
	}

	for (APlayerState* PlayerState : GameState->PlayerArray)
	{
		if (!PlayerState)
		{
			continue;
		}

		if (!bIncludeBots && PlayerState->IsABot())
		{
			continue;
		}

		APlayerController* PlayerController = ResolvePlayerControllerFromState(PlayerState);
		if (!PlayerController || OutPlayerControllers.Contains(PlayerController))
		{
			continue;
		}

		OutPlayerControllers.Add(PlayerController);
	}
}
