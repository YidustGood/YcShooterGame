// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "System/YcGameInstance.h"

#include "YcGameplayTags.h"
#include "Components/GameFrameworkComponentManager.h"
#include "Player/YcPlayerController.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameInstance)

AYcPlayerController* UYcGameInstance::GetPrimaryPlayerController() const
{
	return Cast<AYcPlayerController>(Super::GetPrimaryPlayerController(false));
}

void UYcGameInstance::Init()
{
	Super::Init();
	
	// 注册我们自定义的初始状态链条
	UGameFrameworkComponentManager* ComponentManager = GetSubsystem<UGameFrameworkComponentManager>(this);

	if (ensure(ComponentManager))
	{
		ComponentManager->RegisterInitState(YcGameplayTags::InitState_Spawned, false, FGameplayTag());
		ComponentManager->RegisterInitState(YcGameplayTags::InitState_DataAvailable, false, YcGameplayTags::InitState_Spawned);
		ComponentManager->RegisterInitState(YcGameplayTags::InitState_DataInitialized, false, YcGameplayTags::InitState_DataAvailable);
		ComponentManager->RegisterInitState(YcGameplayTags::InitState_GameplayReady, false, YcGameplayTags::InitState_DataInitialized);
	}
}

void UYcGameInstance::Shutdown()
{
	Super::Shutdown();
	
}
