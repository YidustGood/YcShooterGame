// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "DynamicScreenUI/YcDynamicScreenWidgetBase.h"

#include "DynamicScreenUI/YcDynamicScreenUITags.h"
#include "GameFramework/GameplayMessageSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDynamicScreenWidgetBase)

void UYcDynamicScreenWidgetBase::InitializeDynamicScreenWidget(const FName InWidgetKey)
{
	DynamicWidgetKey = InWidgetKey;
	RegisterDynamicMessageListeners();
	OnDynamicScreenWidgetInitialized();
}

FName UYcDynamicScreenWidgetBase::GetDynamicScreenWidgetKey() const
{
	return DynamicWidgetKey;
}

void UYcDynamicScreenWidgetBase::GetDynamicScreenWidgetPayload(FInstancedStruct& OutPayload) const
{
	OutPayload = CurrentPayload;
}

const FInstancedStruct& UYcDynamicScreenWidgetBase::GetDynamicScreenWidgetPayloadRef() const
{
	return CurrentPayload;
}

void UYcDynamicScreenWidgetBase::NativeDestruct()
{
	UnregisterDynamicMessageListeners();
	Super::NativeDestruct();
}

void UYcDynamicScreenWidgetBase::RegisterDynamicMessageListeners()
{
	if (bDynamicListenersRegistered || DynamicWidgetKey.IsNone() || !GetWorld())
	{
		return;
	}

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(GetWorld());
	DynamicUpdateHandle = MessageSubsystem.RegisterListener<FYcDynamicPlayerScreenUIMessage>(
		YcDynamicScreenUITags::UI_Dynamic_Update,
		this,
		&ThisClass::OnDynamicScreenWidgetUpdated);
	DynamicHideHandle = MessageSubsystem.RegisterListener<FYcDynamicPlayerScreenUIMessage>(
		YcDynamicScreenUITags::UI_Dynamic_Hide,
		this,
		&ThisClass::OnDynamicScreenWidgetHidden);
	bDynamicListenersRegistered = true;
}

void UYcDynamicScreenWidgetBase::UnregisterDynamicMessageListeners()
{
	if (!bDynamicListenersRegistered)
	{
		return;
	}

	if (DynamicUpdateHandle.IsValid())
	{
		DynamicUpdateHandle.Unregister();
	}

	if (DynamicHideHandle.IsValid())
	{
		DynamicHideHandle.Unregister();
	}

	bDynamicListenersRegistered = false;
}

void UYcDynamicScreenWidgetBase::OnDynamicScreenWidgetUpdated(FGameplayTag Channel, const FYcDynamicPlayerScreenUIMessage& Data)
{
	(void)Channel;

	if (Data.WidgetKey != DynamicWidgetKey)
	{
		return;
	}

	CurrentPayload = Data.Payload;
	OnDynamicScreenWidgetPayloadUpdated();
}

void UYcDynamicScreenWidgetBase::OnDynamicScreenWidgetHidden(FGameplayTag Channel, const FYcDynamicPlayerScreenUIMessage& Data)
{
	(void)Channel;

	if (Data.WidgetKey != DynamicWidgetKey)
	{
		return;
	}

	CurrentPayload = Data.Payload;
	OnDynamicScreenWidgetPayloadHidden();
}
