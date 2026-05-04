#include "YcGameCoreEditor.h"

#include "AssetToolsModule.h"
#include "Feedback/ContextEffects/AssetTypeActions_YcContextEffectsLibrary.h"
#include "Feedback/FootstepNotifyGenerator/YcFootstepNotifyGeneratorMenuExtensions.h"
#include "IAssetTools.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FYcGameCoreEditorModule"

DEFINE_LOG_CATEGORY(LogYcGameCoreEditor);

void FYcGameCoreEditorModule::StartupModule()
{
	// 注册反馈资源库资产类型，方便在内容浏览器中直接创建该资产。
	{
		IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		TSharedRef<FAssetTypeActions_YcContextEffectsLibrary> AssetAction = MakeShared<FAssetTypeActions_YcContextEffectsLibrary>();
		YcContextEffectsLibraryAssetAction = AssetAction;
		AssetTools.RegisterAssetTypeActions(AssetAction);
	}

	// ToolMenus 启动后再挂接菜单，避免模块加载时序导致菜单扩展丢失。
	ToolMenusStartupHandle = UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FYcGameCoreEditorModule::RegisterMenus));
}

void FYcGameCoreEditorModule::ShutdownModule()
{
	UToolMenus::UnRegisterStartupCallback(ToolMenusStartupHandle);
	FYcFootstepNotifyGeneratorMenuExtensions::UnregisterMenus();

	if (FModuleManager::Get().IsModuleLoaded("AssetTools"))
	{
		IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();
		if (TSharedPtr<IAssetTypeActions> AssetAction = YcContextEffectsLibraryAssetAction.Pin())
		{
			AssetTools.UnregisterAssetTypeActions(AssetAction.ToSharedRef());
		}
	}
}

void FYcGameCoreEditorModule::RegisterMenus()
{
	FYcFootstepNotifyGeneratorMenuExtensions::RegisterMenus();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FYcGameCoreEditorModule, YiChenGameCoreEditor)
