// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YiChenInventory.h"
#include "System/YcInventoryDataRegistryResolver.h"
#include "System/YcAssetManager.h"

#define LOCTEXT_NAMESPACE "FYiChenInventoryModule"
DEFINE_LOG_CATEGORY(LogYcInventory);

void FYiChenInventoryModule::StartupModule()
{
	// 注册 Inventory DataRegistry 解析器
	// 使用 OnPostEngineInit 确保资产管理器已初始化
	FCoreDelegates::OnPostEngineInit.AddLambda([]()
	{
		if (UYcAssetManager::IsInitialized())
		{
			UYcAssetManager& AssetManager = UYcAssetManager::Get();
			AssetManager.RegisterDataRegistryResolver(MakeShared<FYcInventoryDataRegistryResolver>());
			UE_LOG(LogYcInventory, Log, TEXT("Inventory DataRegistry 解析器已注册"));
		}
	});
}

void FYiChenInventoryModule::ShutdownModule()
{
	// 模块卸载时清理解析器
	if (UYcAssetManager::IsInitialized())
	{
		UYcAssetManager& AssetManager = UYcAssetManager::Get();
		AssetManager.RegisterDataRegistryResolver(nullptr);
	}
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FYiChenInventoryModule, YiChenInventory)