// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "HoverTooltip/YcHoverTooltip.h"

#include "HoverTooltip/YcHoverTooltipComponent.h"
#include "HoverTooltip/YcHoverTooltipProvider.h"
#include "HoverTooltip/YcHoverTooltipProviderWidget.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcHoverTooltip)

namespace
{
	UYcHoverTooltipComponent* ResolveHoverTooltipComponent(APlayerController* PlayerController)
	{
		if (!PlayerController)
		{
			return nullptr;
		}

		if (UYcHoverTooltipComponent* ExistingComponent = PlayerController->FindComponentByClass<UYcHoverTooltipComponent>())
		{
			return ExistingComponent;
		}

		UYcHoverTooltipComponent* HoverTooltipComponent = NewObject<UYcHoverTooltipComponent>(PlayerController);
		if (!HoverTooltipComponent)
		{
			return nullptr;
		}

		PlayerController->AddInstanceComponent(HoverTooltipComponent);
		HoverTooltipComponent->RegisterComponent();
		return HoverTooltipComponent;
	}
}

void UYcHoverTooltipLibrary::NotifyHoverEnter(UUserWidget* SourceWidget, FVector2D ScreenPosition)
{
	if (!SourceWidget)
	{
		return;
	}

	FYcHoverTooltipRequest Request;
	if (!BuildHoverTooltipRequest(SourceWidget, Request))
	{
		return;
	}

	if (UYcHoverTooltipComponent* HoverTooltipComponent = ResolveHoverTooltipComponent(SourceWidget->GetOwningPlayer()))
	{
		HoverTooltipComponent->BeginHoverTooltip(SourceWidget, Request, ScreenPosition);
	}
}

void UYcHoverTooltipLibrary::NotifyHoverMove(UUserWidget* SourceWidget, FVector2D ScreenPosition)
{
	if (!SourceWidget)
	{
		return;
	}

	if (UYcHoverTooltipComponent* HoverTooltipComponent = ResolveHoverTooltipComponent(SourceWidget->GetOwningPlayer()))
	{
		HoverTooltipComponent->UpdateHoverTooltip(SourceWidget, ScreenPosition);
	}
}

void UYcHoverTooltipLibrary::NotifyHoverLeave(UUserWidget* SourceWidget)
{
	CancelHoverTooltipForWidget(SourceWidget);
}

void UYcHoverTooltipLibrary::CancelHoverTooltipForWidget(UUserWidget* SourceWidget)
{
	if (!SourceWidget)
	{
		return;
	}

	if (UYcHoverTooltipComponent* HoverTooltipComponent = ResolveHoverTooltipComponent(SourceWidget->GetOwningPlayer()))
	{
		HoverTooltipComponent->EndHoverTooltip(SourceWidget);
	}
}

void UYcHoverTooltipLibrary::CancelHoverTooltipForPlayer(APlayerController* PlayerController)
{
	if (UYcHoverTooltipComponent* HoverTooltipComponent = ResolveHoverTooltipComponent(PlayerController))
	{
		HoverTooltipComponent->CancelHoverTooltip();
	}
}

bool UYcHoverTooltipLibrary::BuildHoverTooltipRequest(UObject* SourceObject, FYcHoverTooltipRequest& OutRequest)
{
	if (!SourceObject)
	{
		return false;
	}

	bool bCanShow = false;
	TSubclassOf<UCommonActivatableWidget> WidgetClass;
	FInstancedStruct Payload;
	FYcHoverTooltipDisplayConfig DisplayConfig;

	if (SourceObject->GetClass()->ImplementsInterface(UYcHoverTooltipProvider::StaticClass()))
	{
		bCanShow = IYcHoverTooltipProvider::Execute_CanShowHoverTooltip(SourceObject);
		WidgetClass = IYcHoverTooltipProvider::Execute_GetHoverTooltipWidgetClass(SourceObject);
		Payload = IYcHoverTooltipProvider::Execute_BuildHoverTooltipPayload(SourceObject);
		DisplayConfig = IYcHoverTooltipProvider::Execute_GetHoverTooltipDisplayConfig(SourceObject);
	}
	else if (const UYcHoverTooltipProviderWidget* ProviderWidget = Cast<UYcHoverTooltipProviderWidget>(SourceObject))
	{
		bCanShow = ProviderWidget->CanShowHoverTooltip();
		WidgetClass = ProviderWidget->GetHoverTooltipWidgetClass();
		Payload = ProviderWidget->BuildHoverTooltipPayload();
		DisplayConfig = ProviderWidget->GetHoverTooltipDisplayConfig();
	}
	else
	{
		return false;
	}

	if (!bCanShow || !WidgetClass)
	{
		return false;
	}

	OutRequest.WidgetClass = WidgetClass;
	OutRequest.Payload = Payload;
	OutRequest.DisplayConfig = DisplayConfig;
	return true;
}
