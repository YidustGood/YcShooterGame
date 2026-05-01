// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "DynamicScreenUI/YcManagedPlayerScreenSlot.h"

#include "Actions/AsyncAction_PushContentToLayerForPlayer.h"
#include "CommonActivatableWidget.h"
#include "DynamicScreenUI/YcPlayerScreenUIComponent.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcManagedPlayerScreenSlot)

void UYcManagedPlayerScreenSlot::InitializeSlot(UYcPlayerScreenUIComponent* InOwningScreenUIComponent, APlayerController* InOwningPlayerController, FName InWidgetKey)
{
	OwningScreenUIComponent = InOwningScreenUIComponent;
	OwningPlayerController = InOwningPlayerController;
	WidgetKey = InWidgetKey;
}

void UYcManagedPlayerScreenSlot::ShowWidget(TSoftClassPtr<UCommonActivatableWidget> InWidgetClass, FGameplayTag InWidgetLayer, bool bInSuspendInputUntilWidgetPushed)
{
	if (!OwningPlayerController || InWidgetClass.IsNull())
	{
		return;
	}

	if (IsValid(ActiveWidget))
	{
		if (OwningScreenUIComponent)
		{
			OwningScreenUIComponent->HandleScreenSlotWidgetPushed(WidgetKey, ActiveWidget);
		}
		return;
	}

	if (IsValid(PendingPushAction))
	{
		return;
	}

	PendingPushAction = UAsyncAction_PushContentToLayerForPlayer::PushContentToLayerForPlayer(
		OwningPlayerController,
		InWidgetClass,
		InWidgetLayer,
		bInSuspendInputUntilWidgetPushed);

	if (!PendingPushAction)
	{
		return;
	}

	PendingPushAction->AfterPush.AddDynamic(this, &ThisClass::HandleWidgetPushed);
	PendingPushAction->Activate();
}

void UYcManagedPlayerScreenSlot::HideWidget()
{
	if (IsValid(PendingPushAction))
	{
		PendingPushAction->Cancel();
		PendingPushAction = nullptr;
	}

	if (IsValid(ActiveWidget))
	{
		ActiveWidget->DeactivateWidget();
		ActiveWidget = nullptr;
	}
}

bool UYcManagedPlayerScreenSlot::HasActiveWidget() const
{
	return IsValid(ActiveWidget);
}

void UYcManagedPlayerScreenSlot::SetCurrentPayload(const FInstancedStruct& InPayload)
{
	CurrentPayload = InPayload;
}

const FInstancedStruct& UYcManagedPlayerScreenSlot::GetCurrentPayload() const
{
	return CurrentPayload;
}

void UYcManagedPlayerScreenSlot::HandleWidgetPushed(UCommonActivatableWidget* UserWidget)
{
	PendingPushAction = nullptr;
	ActiveWidget = UserWidget;

	if (OwningScreenUIComponent)
	{
		OwningScreenUIComponent->HandleScreenSlotWidgetPushed(WidgetKey, UserWidget);
	}
}
