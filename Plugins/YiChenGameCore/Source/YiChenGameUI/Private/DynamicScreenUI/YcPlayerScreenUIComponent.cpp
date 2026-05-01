// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "DynamicScreenUI/YcPlayerScreenUIComponent.h"

#include "CommonActivatableWidget.h"
#include "DynamicScreenUI/YcDynamicScreenWidgetBase.h"
#include "DynamicScreenUI/YcManagedPlayerScreenSlot.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "NativeGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcPlayerScreenUIComponent)

namespace YcDynamicScreenUITags
{
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_DYNAMIC_REQUEST_SHOW, "UI.Dynamic.Request.Show");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_DYNAMIC_REQUEST_UPDATE, "UI.Dynamic.Request.Update");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_DYNAMIC_REQUEST_HIDE, "UI.Dynamic.Request.Hide");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_DYNAMIC_UPDATE, "UI.Dynamic.Update");
	UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_UI_DYNAMIC_HIDE, "UI.Dynamic.Hide");
}

UYcPlayerScreenUIComponent::UYcPlayerScreenUIComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UYcPlayerScreenUIComponent::ShowDynamicWidget(const FYcDynamicPlayerScreenUIShowRequest& Request)
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController() || Request.WidgetClass.IsNull() || Request.WidgetKey.IsNone())
	{
		return;
	}

	UYcManagedPlayerScreenSlot* ScreenSlot = ResolveScreenSlot(Request.WidgetKey);
	if (!ScreenSlot)
	{
		return;
	}

	ScreenSlot->SetCurrentPayload(Request.InitialPayload);
	ScreenSlot->ShowWidget(Request.WidgetClass, Request.WidgetLayer, Request.bSuspendInputUntilWidgetPushed);
}

void UYcPlayerScreenUIComponent::ClientShowDynamicWidget_Implementation(const FYcDynamicPlayerScreenUIShowRequest& Request)
{
	EnqueueShowDynamicWidgetRequest(Request);
}

void UYcPlayerScreenUIComponent::UpdateDynamicWidget(FName WidgetKey, const FInstancedStruct& Payload)
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController() || WidgetKey.IsNone())
	{
		return;
	}

	TObjectPtr<UYcManagedPlayerScreenSlot>* ScreenSlotPtr = ScreenSlots.Find(WidgetKey);
	if (!ScreenSlotPtr || !*ScreenSlotPtr)
	{
		return;
	}

	UYcManagedPlayerScreenSlot* ScreenSlot = *ScreenSlotPtr;
	ScreenSlot->SetCurrentPayload(Payload);
	if (ScreenSlot->HasActiveWidget())
	{
		BroadcastDynamicWidgetUpdate(WidgetKey, Payload);
	}
}

void UYcPlayerScreenUIComponent::ClientUpdateDynamicWidget_Implementation(FName WidgetKey, const FInstancedStruct& Payload)
{
	EnqueueUpdateDynamicWidgetRequest(WidgetKey, Payload);
}

void UYcPlayerScreenUIComponent::HideDynamicWidget(FName WidgetKey, const FInstancedStruct& Payload)
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController() || WidgetKey.IsNone())
	{
		return;
	}

	TObjectPtr<UYcManagedPlayerScreenSlot>* ScreenSlotPtr = ScreenSlots.Find(WidgetKey);
	if (!ScreenSlotPtr || !*ScreenSlotPtr)
	{
		return;
	}

	UYcManagedPlayerScreenSlot* ScreenSlot = *ScreenSlotPtr;
	if (ScreenSlot->HasActiveWidget())
	{
		BroadcastDynamicWidgetHide(WidgetKey, Payload);
	}

	ScreenSlot->HideWidget();
}

void UYcPlayerScreenUIComponent::ClientHideDynamicWidget_Implementation(FName WidgetKey, const FInstancedStruct& Payload)
{
	EnqueueHideDynamicWidgetRequest(WidgetKey, Payload);
}

void UYcPlayerScreenUIComponent::EnqueueShowDynamicWidgetRequest(const FYcDynamicPlayerScreenUIShowRequest& Request)
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController() || !GetWorld())
	{
		return;
	}

	FYcDynamicPlayerScreenUIShowRequest RoutedRequest = Request;
	RoutedRequest.TargetPlayerController = PlayerController;
	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(YcDynamicScreenUITags::TAG_UI_DYNAMIC_REQUEST_SHOW, RoutedRequest);
}

void UYcPlayerScreenUIComponent::EnqueueUpdateDynamicWidgetRequest(FName WidgetKey, const FInstancedStruct& Payload)
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController() || WidgetKey.IsNone() || !GetWorld())
	{
		return;
	}

	FYcDynamicPlayerScreenUIRouteMessage Request;
	Request.TargetPlayerController = PlayerController;
	Request.WidgetKey = WidgetKey;
	Request.Payload = Payload;
	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(YcDynamicScreenUITags::TAG_UI_DYNAMIC_REQUEST_UPDATE, Request);
}

void UYcPlayerScreenUIComponent::EnqueueHideDynamicWidgetRequest(FName WidgetKey, const FInstancedStruct& Payload)
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController() || WidgetKey.IsNone() || !GetWorld())
	{
		return;
	}

	FYcDynamicPlayerScreenUIRouteMessage Request;
	Request.TargetPlayerController = PlayerController;
	Request.WidgetKey = WidgetKey;
	Request.Payload = Payload;
	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(YcDynamicScreenUITags::TAG_UI_DYNAMIC_REQUEST_HIDE, Request);
}

void UYcPlayerScreenUIComponent::HandleScreenSlotWidgetPushed(FName WidgetKey, UCommonActivatableWidget* UserWidget)
{
	if (UYcDynamicScreenWidgetBase* DynamicWidget = Cast<UYcDynamicScreenWidgetBase>(UserWidget))
	{
		DynamicWidget->InitializeDynamicScreenWidget(WidgetKey);
	}

	TObjectPtr<UYcManagedPlayerScreenSlot>* ScreenSlotPtr = ScreenSlots.Find(WidgetKey);
	if (!ScreenSlotPtr || !*ScreenSlotPtr)
	{
		return;
	}

	BroadcastDynamicWidgetUpdate(WidgetKey, (*ScreenSlotPtr)->GetCurrentPayload());
}

void UYcPlayerScreenUIComponent::BeginPlay()
{
	Super::BeginPlay();
	RegisterRouteListeners();
}

void UYcPlayerScreenUIComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterRouteListeners();

	TArray<FName> WidgetKeys;
	ScreenSlots.GetKeys(WidgetKeys);
	for (const FName& WidgetKey : WidgetKeys)
	{
		HideDynamicWidget(WidgetKey);
	}

	Super::EndPlay(EndPlayReason);
}

void UYcPlayerScreenUIComponent::OnDynamicShowRequestReceived(FGameplayTag Channel, const FYcDynamicPlayerScreenUIShowRequest& Request)
{
	(void)Channel;

	if (!ShouldProcessRouteRequest(Request.TargetPlayerController, Request.WidgetKey))
	{
		return;
	}

	ShowDynamicWidget(Request);
}

void UYcPlayerScreenUIComponent::OnDynamicUpdateRequestReceived(FGameplayTag Channel, const FYcDynamicPlayerScreenUIRouteMessage& Request)
{
	(void)Channel;

	if (!ShouldProcessRouteRequest(Request.TargetPlayerController, Request.WidgetKey))
	{
		return;
	}

	UpdateDynamicWidget(Request.WidgetKey, Request.Payload);
}

void UYcPlayerScreenUIComponent::OnDynamicHideRequestReceived(FGameplayTag Channel, const FYcDynamicPlayerScreenUIRouteMessage& Request)
{
	(void)Channel;

	if (!ShouldProcessRouteRequest(Request.TargetPlayerController, Request.WidgetKey))
	{
		return;
	}

	HideDynamicWidget(Request.WidgetKey, Request.Payload);
}

void UYcPlayerScreenUIComponent::BroadcastDynamicWidgetUpdate(FName WidgetKey, const FInstancedStruct& Payload)
{
	if (!GetWorld())
	{
		return;
	}

	FYcDynamicPlayerScreenUIMessage Message;
	Message.WidgetKey = WidgetKey;
	Message.Payload = Payload;
	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(YcDynamicScreenUITags::TAG_UI_DYNAMIC_UPDATE, Message);
}

void UYcPlayerScreenUIComponent::BroadcastDynamicWidgetHide(FName WidgetKey, const FInstancedStruct& Payload)
{
	if (!GetWorld())
	{
		return;
	}

	FYcDynamicPlayerScreenUIMessage Message;
	Message.WidgetKey = WidgetKey;
	Message.Payload = Payload;
	UGameplayMessageSubsystem::Get(GetWorld()).BroadcastMessage(YcDynamicScreenUITags::TAG_UI_DYNAMIC_HIDE, Message);
}

UYcManagedPlayerScreenSlot* UYcPlayerScreenUIComponent::ResolveScreenSlot(FName WidgetKey)
{
	if (TObjectPtr<UYcManagedPlayerScreenSlot>* ExistingSlot = ScreenSlots.Find(WidgetKey))
	{
		return ExistingSlot->Get();
	}

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController)
	{
		return nullptr;
	}

	UYcManagedPlayerScreenSlot* ScreenSlot = NewObject<UYcManagedPlayerScreenSlot>(this);
	if (!ScreenSlot)
	{
		return nullptr;
	}

	ScreenSlot->InitializeSlot(this, PlayerController, WidgetKey);
	ScreenSlots.Add(WidgetKey, ScreenSlot);
	return ScreenSlot;
}

void UYcPlayerScreenUIComponent::RegisterRouteListeners()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if (bRouteListenersRegistered || !PlayerController || !PlayerController->IsLocalController() || !GetWorld())
	{
		return;
	}

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(GetWorld());
	DynamicShowRequestHandle = MessageSubsystem.RegisterListener<FYcDynamicPlayerScreenUIShowRequest>(
		YcDynamicScreenUITags::TAG_UI_DYNAMIC_REQUEST_SHOW,
		this,
		&ThisClass::OnDynamicShowRequestReceived);
	DynamicUpdateRequestHandle = MessageSubsystem.RegisterListener<FYcDynamicPlayerScreenUIRouteMessage>(
		YcDynamicScreenUITags::TAG_UI_DYNAMIC_REQUEST_UPDATE,
		this,
		&ThisClass::OnDynamicUpdateRequestReceived);
	DynamicHideRequestHandle = MessageSubsystem.RegisterListener<FYcDynamicPlayerScreenUIRouteMessage>(
		YcDynamicScreenUITags::TAG_UI_DYNAMIC_REQUEST_HIDE,
		this,
		&ThisClass::OnDynamicHideRequestReceived);
	bRouteListenersRegistered = true;
}

void UYcPlayerScreenUIComponent::UnregisterRouteListeners()
{
	if (!bRouteListenersRegistered)
	{
		return;
	}

	if (DynamicShowRequestHandle.IsValid())
	{
		DynamicShowRequestHandle.Unregister();
	}
	if (DynamicUpdateRequestHandle.IsValid())
	{
		DynamicUpdateRequestHandle.Unregister();
	}
	if (DynamicHideRequestHandle.IsValid())
	{
		DynamicHideRequestHandle.Unregister();
	}

	bRouteListenersRegistered = false;
}

bool UYcPlayerScreenUIComponent::ShouldProcessRouteRequest(const APlayerController* TargetPlayerController, FName WidgetKey) const
{
	const APlayerController* PlayerController = GetOwningPlayerController();
	if (!PlayerController || !PlayerController->IsLocalController() || WidgetKey.IsNone())
	{
		return false;
	}

	return TargetPlayerController == PlayerController;
}

APlayerController* UYcPlayerScreenUIComponent::GetOwningPlayerController() const
{
	return Cast<APlayerController>(GetOwner());
}
