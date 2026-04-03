#include "YiChenGameplay.h"
#include "System/YcAssetManager.h"
#include "YiChenInventory/Public/System/YcInventoryDataRegistryResolver.h"

#define LOCTEXT_NAMESPACE "FYiChenGameplayModule"

DEFINE_LOG_CATEGORY(LogYcGameplay);
DEFINE_LOG_CATEGORY(LogYcInput);

void FYiChenGameplayModule::StartupModule()
{
	// 注册 Inventory DataRegistry 解析器
	// 使用 OnPostEngineInit 确保资产管理器已初始化
	FCoreDelegates::OnPostEngineInit.AddLambda([]()
	{
		if (UYcAssetManager::IsInitialized())
		{
			UYcAssetManager& AssetManager = UYcAssetManager::Get();
			AssetManager.RegisterDataRegistryResolver(MakeShared<FYcInventoryDataRegistryResolver>());
			UE_LOG(LogYcGameplay, Log, TEXT("Inventory DataRegistry 解析器已注册"));
		}
	});
}

void FYiChenGameplayModule::ShutdownModule()
{
	// 模块卸载时清理解析器
	if (UYcAssetManager::IsInitialized())
	{
		UYcAssetManager& AssetManager = UYcAssetManager::Get();
		AssetManager.RegisterDataRegistryResolver(nullptr);
	}
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FYiChenGameplayModule, YiChenGameplay)