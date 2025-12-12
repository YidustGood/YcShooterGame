// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "AsyncAction_ListenForRedDotStateChanged.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_ListenForRedDotStateChanged)

UAsyncAction_ListenForRedDotStateChanged* UAsyncAction_ListenForRedDotStateChanged::ListenForRedDotStateChanged(
	UObject* WorldContextObject, FGameplayTag RedDotTag, EYcRedDotTagMatch MatchType, bool bUnregisterOnWorldDestroyed)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return nullptr;
	}

	UAsyncAction_ListenForRedDotStateChanged* Action = NewObject<UAsyncAction_ListenForRedDotStateChanged>();
	Action->WorldPtr = World;
	Action->RedDotTagRegister = RedDotTag;
	Action->RedDotTagMatchType = MatchType;
	Action->bUnregisterOnWorldDestroyed = bUnregisterOnWorldDestroyed;
	Action->RegisterWithGameInstance(World);

	return Action;
}

void UAsyncAction_ListenForRedDotStateChanged::Activate()
{
	if (UWorld* World = WorldPtr.Get())
	{
		if (UYcRedDotManagerSubsystem::HasInstance(World))
		{
			UYcRedDotManagerSubsystem& RedDotManager = UYcRedDotManagerSubsystem::Get(World);

			TWeakObjectPtr<UAsyncAction_ListenForRedDotStateChanged> WeakThis(this);
			ListenerHandle = RedDotManager.RegisterRedDotStateChangedListener(RedDotTagRegister,
				[WeakThis](FGameplayTag Channel, const FRedDotInfo* RedDotInfo)
				{
					if (UAsyncAction_ListenForRedDotStateChanged* StrongThis = WeakThis.Get())
					{
						StrongThis->HandleRedDotStateChanged(Channel, RedDotInfo);
					}
				},
	
				RedDotTagMatchType,
				bUnregisterOnWorldDestroyed
			);

			return;
		}
	}

	SetReadyToDestroy();
}

void UAsyncAction_ListenForRedDotStateChanged::SetReadyToDestroy()
{
	ListenerHandle.Unregister();
	Super::SetReadyToDestroy();
}

void UAsyncAction_ListenForRedDotStateChanged::HandleRedDotStateChanged(FGameplayTag Channel,
	const FRedDotInfo* RedDotInfo)
{
	// 如果改变后的RedDotInfo有效, 直接广播消息
	if (RedDotInfo != nullptr)
	{
		OnRedDotStateChanged.Broadcast(Channel, *RedDotInfo);
	}
	
	// 如果委托没有绑定任何回调可以直接销毁
	if (!OnRedDotStateChanged.IsBound())
	{
		SetReadyToDestroy();
	}
}
