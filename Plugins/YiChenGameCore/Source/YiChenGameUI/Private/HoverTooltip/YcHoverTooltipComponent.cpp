// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "HoverTooltip/YcHoverTooltipComponent.h"

#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "CommonInputSubsystem.h"
#include "DynamicScreenUI/YcDynamicScreenUITags.h"
#include "DynamicScreenUI/YcDynamicScreenUITypes.h"
#include "DynamicScreenUI/YcDynamicScreenWidgetBase.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcHoverTooltipComponent)

namespace
{
	const FName HoverTooltipWidgetKey(TEXT("UI.HoverTooltip"));
}

UYcHoverTooltipComponent::UYcHoverTooltipComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(false);
	PrimaryComponentTick.bCanEverTick = true;
}

void UYcHoverTooltipComponent::BeginHoverTooltip(UObject* SourceObject, const FYcHoverTooltipRequest& Request, FVector2D InitialScreenPosition)
{
	if (!SourceObject || !Request.WidgetClass || !CanUseHoverTooltip())
	{
		ClearHoverTooltipState(true);
		return;
	}

	const bool bSourceChanged = HoverTooltipState.ActiveSource.Get() != SourceObject;
	if (bSourceChanged)
	{
		ClearHoverTooltipState(true);
	}

	HoverTooltipState.ActiveSource = SourceObject;
	HoverTooltipState.ActiveRequest = Request;
	HoverTooltipState.ScreenPosition = InitialScreenPosition;

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(HoverTooltipState.DelayTimerHandle);
	}

	if (Request.DisplayConfig.DelaySeconds <= 0.0f)
	{
		ShowHoverTooltipNow();
		return;
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			HoverTooltipState.DelayTimerHandle,
			this,
			&ThisClass::ShowHoverTooltipNow,
			Request.DisplayConfig.DelaySeconds,
			false);
	}
}

void UYcHoverTooltipComponent::UpdateHoverTooltip(UObject* SourceObject, FVector2D ScreenPosition)
{
	if (!SourceObject || HoverTooltipState.ActiveSource.Get() != SourceObject)
	{
		return;
	}

	HoverTooltipState.ScreenPosition = ScreenPosition;
	if (HoverTooltipState.bVisible && HoverTooltipState.ActiveRequest.DisplayConfig.bFollowCursor)
	{
		BroadcastHoverTooltipUpdate(HoverTooltipWidgetKey, FInstancedStruct::Make(BuildHoverTooltipWidgetPayload()));
	}
}

void UYcHoverTooltipComponent::EndHoverTooltip(UObject* SourceObject)
{
	if (!SourceObject || HoverTooltipState.ActiveSource.Get() != SourceObject)
	{
		return;
	}

	ClearHoverTooltipState(true);
}

void UYcHoverTooltipComponent::CancelHoverTooltip()
{
	ClearHoverTooltipState(true);
}

void UYcHoverTooltipComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearHoverTooltipState(true);
	Super::EndPlay(EndPlayReason);
}

void UYcHoverTooltipComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	(void)DeltaTime;
	(void)TickType;
	(void)ThisTickFunction;

	if (!HoverTooltipState.ActiveSource.IsValid())
	{
		if (HoverTooltipState.bVisible || HoverTooltipState.DelayTimerHandle.IsValid())
		{
			ClearHoverTooltipState(true);
		}
		return;
	}

	if (!CanUseHoverTooltip() || UWidgetBlueprintLibrary::IsDragDropping())
	{
		ClearHoverTooltipState(true);
	}
}

APlayerController* UYcHoverTooltipComponent::GetOwningPlayerController() const
{
	return Cast<APlayerController>(GetOwner());
}

bool UYcHoverTooltipComponent::CanUseHoverTooltip() const
{
	APlayerController* PlayerController = GetOwningPlayerController();
	return PlayerController && PlayerController->IsLocalController() && IsMouseAndKeyboardInputActive();
}

bool UYcHoverTooltipComponent::IsMouseAndKeyboardInputActive() const
{
	const APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController)
	{
		return false;
	}

	const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return true;
	}

	const UCommonInputSubsystem* CommonInputSubsystem = UCommonInputSubsystem::Get(LocalPlayer);
	if (!CommonInputSubsystem)
	{
		return true;
	}

	return CommonInputSubsystem->GetCurrentInputType() == ECommonInputType::MouseAndKeyboard;
}

void UYcHoverTooltipComponent::ShowHoverTooltipNow()
{
	if (!HoverTooltipState.ActiveSource.IsValid() || !HoverTooltipState.ActiveRequest.WidgetClass || !CanUseHoverTooltip())
	{
		ClearHoverTooltipState(true);
		return;
	}

	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(HoverTooltipState.DelayTimerHandle);
	}

	ShowOrUpdateHoverTooltipWidget(FInstancedStruct::Make(BuildHoverTooltipWidgetPayload()));
}

void UYcHoverTooltipComponent::ShowOrUpdateHoverTooltipWidget(const FInstancedStruct& Payload)
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController() || !HoverTooltipState.ActiveRequest.WidgetClass)
	{
		return;
	}

	UClass* DesiredWidgetClass = HoverTooltipState.ActiveRequest.WidgetClass.Get();
	if (!DesiredWidgetClass)
	{
		return;
	}

	if (IsValid(HoverTooltipState.ActiveWidget) && HoverTooltipState.ActiveWidget->GetClass() != DesiredWidgetClass)
	{
		DestroyHoverTooltipWidget();
	}

	if (!IsValid(HoverTooltipState.ActiveWidget))
	{
		UYcDynamicScreenWidgetBase* TooltipWidget = CreateWidget<UYcDynamicScreenWidgetBase>(PlayerController, DesiredWidgetClass);
		if (!TooltipWidget)
		{
			return;
		}

		HoverTooltipState.ActiveWidget = TooltipWidget;
		TooltipWidget->AddToPlayerScreen(10000);
		TooltipWidget->InitializeDynamicScreenWidget(HoverTooltipWidgetKey);
	}

	HoverTooltipState.bVisible = true;
	BroadcastHoverTooltipUpdate(HoverTooltipWidgetKey, Payload);
}

void UYcHoverTooltipComponent::DestroyHoverTooltipWidget()
{
	if (!IsValid(HoverTooltipState.ActiveWidget))
	{
		return;
	}

	HoverTooltipState.ActiveWidget->RemoveFromParent();
	HoverTooltipState.ActiveWidget = nullptr;
}

void UYcHoverTooltipComponent::ClearHoverTooltipState(bool bHideWidget)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(HoverTooltipState.DelayTimerHandle);
	}

	if (bHideWidget && (HoverTooltipState.bVisible || IsValid(HoverTooltipState.ActiveWidget)))
	{
		BroadcastHoverTooltipHide(HoverTooltipWidgetKey, FInstancedStruct());
		DestroyHoverTooltipWidget();
	}

	HoverTooltipState.ResetSessionData();
}

void UYcHoverTooltipComponent::BroadcastHoverTooltipUpdate(FName WidgetKey, const FInstancedStruct& Payload) const
{
	if (!GetWorld())
	{
		return;
	}

	FYcDynamicPlayerScreenUIMessage Message;
	Message.WidgetKey = WidgetKey;
	Message.Payload = Payload;
	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(YcDynamicScreenUITags::UI_Dynamic_Update, Message);
}

void UYcHoverTooltipComponent::BroadcastHoverTooltipHide(FName WidgetKey, const FInstancedStruct& Payload) const
{
	if (!GetWorld())
	{
		return;
	}

	FYcDynamicPlayerScreenUIMessage Message;
	Message.WidgetKey = WidgetKey;
	Message.Payload = Payload;
	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(YcDynamicScreenUITags::UI_Dynamic_Hide, Message);
}

FYcHoverTooltipWidgetPayload UYcHoverTooltipComponent::BuildHoverTooltipWidgetPayload() const
{
	FYcHoverTooltipWidgetPayload Payload;
	Payload.ScreenPosition = HoverTooltipState.ScreenPosition;
	Payload.ScreenOffset = HoverTooltipState.ActiveRequest.DisplayConfig.ScreenOffset;
	Payload.bFollowCursor = HoverTooltipState.ActiveRequest.DisplayConfig.bFollowCursor;
	Payload.bClampToViewport = HoverTooltipState.ActiveRequest.DisplayConfig.bClampToViewport;
	Payload.ContentPayload = HoverTooltipState.ActiveRequest.Payload;
	return Payload;
}
