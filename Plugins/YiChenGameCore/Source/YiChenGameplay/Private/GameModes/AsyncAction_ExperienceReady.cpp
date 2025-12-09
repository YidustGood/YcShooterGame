// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameModes/AsyncAction_ExperienceReady.h"

#include "GameModes/YcExperienceManagerComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_ExperienceReady)

UAsyncAction_ExperienceReady::UAsyncAction_ExperienceReady(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UAsyncAction_ExperienceReady* UAsyncAction_ExperienceReady::WaitForExperienceReady(UObject* InWorldContextObject)
{
	UAsyncAction_ExperienceReady* Action = nullptr;

	if (UWorld* World = GEngine->GetWorldFromContextObject(InWorldContextObject, EGetWorldErrorMode::LogAndReturnNull))
	{
		Action = NewObject<UAsyncAction_ExperienceReady>();
		Action->WorldPtr = World;
		// 将异步操作注册到游戏实例，防止对象在异步操作完成前被垃圾回收
		Action->RegisterWithGameInstance(World);
	}

	return Action;
}

void UAsyncAction_ExperienceReady::Activate()
{
	UWorld* World = WorldPtr.Get();
	// 没有世界对象, 永远不会自然完成, 直接结束
	if (!World) SetReadyToDestroy();
	
	// GameState有效就直接通过GameState监听游戏体验加载完成事件, 无效就监听GameState设置事件以获取GameState后再监听
	if (AGameStateBase* GameState = World->GetGameState())
	{
		Step2_ListenToExperienceLoading(GameState);
	}
	else
	{
		World->GameStateSetEvent.AddUObject(this, &ThisClass::Step1_HandleGameStateSet);
	}
}

void UAsyncAction_ExperienceReady::Step1_HandleGameStateSet(AGameStateBase* GameState)
{
	if (UWorld* World = WorldPtr.Get())
	{
		World->GameStateSetEvent.RemoveAll(this);
	}

	Step2_ListenToExperienceLoading(GameState);
}

void UAsyncAction_ExperienceReady::Step2_ListenToExperienceLoading(const AGameStateBase* GameState)
{
	check(GameState);
	UYcExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UYcExperienceManagerComponent>();
	check(ExperienceComponent);

	// 已加载就绑定到下一帧调用, 未加载就注册监听加载完成事件
	if (ExperienceComponent->IsExperienceLoaded())
	{
		UWorld* World = GameState->GetWorld();
		check(World);

		// 体验恰巧已经加载完成，但仍延迟一帧执行，
		// 以确保开发者不会编写依赖于"体验总是已加载"这种假设的代码
		// @TODO: 考虑是否为动态生成的内容/加载界面消失后的任何时机取消延迟？
		// @TODO: 也许在体验加载本身中注入0-1秒的随机延迟？
		World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(this, &ThisClass::Step4_BroadcastReady));
	}
	else
	{
		ExperienceComponent->CallOrRegister_OnExperienceLoaded(FOnYcExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::Step3_HandleExperienceLoaded));
	}
}

void UAsyncAction_ExperienceReady::Step3_HandleExperienceLoaded(const UYcExperienceDefinition* CurrentExperience)
{
	Step4_BroadcastReady();
}

void UAsyncAction_ExperienceReady::Step4_BroadcastReady()
{
	OnReady.Broadcast();

	SetReadyToDestroy();
}
