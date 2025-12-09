// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Character/AsyncAction_PawnExtensionReady.h"

#include "Character/YcPawnExtensionComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_PawnExtensionReady)

UAsyncAction_PawnExtensionReady* UAsyncAction_PawnExtensionReady::WaitForPawnExtensionReady(UObject* WorldContextObject)
{
	UAsyncAction_PawnExtensionReady* Action = nullptr;

	if (const auto PawnExtComp = UYcPawnExtensionComponent::FindPawnExtensionComponent(Cast<AActor>(WorldContextObject));
		const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		Action = NewObject<UAsyncAction_PawnExtensionReady>();
		Action->PawnExtCompPtr = PawnExtComp;
		// 将异步操作注册到游戏实例，防止对象在异步操作完成前被垃圾回收
		Action->RegisterWithGameInstance(World);
	}

	return Action;
}

void UAsyncAction_PawnExtensionReady::Activate()
{
	if (auto PawnExt = PawnExtCompPtr.Get())
	{
		if (PawnExt->IsExtensionReady())
		{
			OnReady.Broadcast();
		}else
		{
			PawnExt->OnExtensionReady.AddDynamic(this, &ThisClass::BroadcastReady);
		}
	}
	else
	{
		SetReadyToDestroy();
	}
}

void UAsyncAction_PawnExtensionReady::BroadcastReady()
{
	OnReady.Broadcast();
	SetReadyToDestroy();
}