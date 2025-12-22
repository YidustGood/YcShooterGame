// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YiChenGameCore.h"

#include "GameplayTagsManager.h"

#define LOCTEXT_NAMESPACE "FYiChenGameCoreModule"

DEFINE_LOG_CATEGORY(LogYcGameCore);

void FYiChenGameCoreModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	// 扫描插件Config/Tags目录下的GameplayTag配置文件
	UGameplayTagsManager::Get().AddTagIniSearchPath(FPaths::ProjectPluginsDir() / TEXT("YiChenGameCore/Config/Tags"));
}

void FYiChenGameCoreModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FYiChenGameCoreModule, YiChenGameCore)