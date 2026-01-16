// Copyright Epic Games, Inc. All Rights Reserved.

#include "YiChenShooterCore.h"

#include "GameplayTagsManager.h"

#define LOCTEXT_NAMESPACE "FYiChenShooterCoreModule"

DEFINE_LOG_CATEGORY(LogYcShooterCore);

void FYiChenShooterCoreModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	// 扫描插件Config/Tags目录下的GameplayTag配置文件
	UGameplayTagsManager::Get().AddTagIniSearchPath(FPaths::ProjectPluginsDir() / TEXT("YiChenShooterCore/Config/Tags"));
}

void FYiChenShooterCoreModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FYiChenShooterCoreModule, YiChenShooterCore)