// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "AsyncAction_ListenForRedDotCleared.h"

#include "YcRedDotManagerSubsystem.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_ListenForRedDotCleared)

UAsyncAction_ListenForRedDotCleared* UAsyncAction_ListenForRedDotCleared::ListenForRedDotCleared(
	UObject* WorldContextObject, FGameplayTag RedDotTag)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return nullptr;
	}

	UAsyncAction_ListenForRedDotCleared* Action = NewObject<UAsyncAction_ListenForRedDotCleared>();
	Action->WorldPtr = World;
	Action->RedDotTag = RedDotTag;
	Action->RegisterWithGameInstance(World);
	return Action;
}

void UAsyncAction_ListenForRedDotCleared::Activate()
{
	if (UWorld* World = WorldPtr.Get())
	{
		if (UYcRedDotManagerSubsystem::HasInstance(World))
		{
			UYcRedDotManagerSubsystem& RedDotManager = UYcRedDotManagerSubsystem::Get(World);

			TWeakObjectPtr<UAsyncAction_ListenForRedDotCleared> WeakThis(this);
			ListenerHandle = RedDotManager.RegisterRedDotDataProvider(RedDotTag,
				[WeakThis]()
				{
					if (UAsyncAction_ListenForRedDotCleared* StrongThis = WeakThis.Get())
					{
						StrongThis->HandleRedDotCleared();
					}
				}
			);

			return;
		}
	}

	SetReadyToDestroy();
}

void UAsyncAction_ListenForRedDotCleared::SetReadyToDestroy()
{
	ListenerHandle.Unregister();
	Super::SetReadyToDestroy();
}

void UAsyncAction_ListenForRedDotCleared::HandleRedDotCleared()
{
	OnRedDotCleared.Broadcast();
	
	// 如果委托没有绑定任何回调可以直接销毁
	if (!OnRedDotCleared.IsBound())
	{
		SetReadyToDestroy();
	}
}
