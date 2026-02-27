// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcShooter/Public/YcShooterGameInstance.h"

#include "YcLuaStateManager.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcShooterGameInstance)

UYcShooterGameInstance::UYcShooterGameInstance(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{ 
	// 确保不是CDO或Archetype对象时才初始化Lua
	if (!HasAnyFlags(RF_ClassDefaultObject | RF_ArchetypeObject))
	{
		UE_LOG(LogTemp, Warning, TEXT("Initiating Slua"));
		// 通过单例管理器初始化Lua状态
		FYcLuaStateManager::GetYiChenLuaStateManager().Init(this);
	}
}

void UYcShooterGameInstance::Init()
{
	Super::Init();
}

void UYcShooterGameInstance::Shutdown()
{
	Super::Shutdown();
	UE_LOG(LogTemp, Warning, TEXT("Closing Slua"));
	// 游戏实例关闭时，关闭Lua状态管理器
	FYcLuaStateManager::GetYiChenLuaStateManager().Close();
}