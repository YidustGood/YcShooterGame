// Copyright Epic Games, Inc. All Rights Reserved.

#include "YcAngelScriptMixin.h"

#include "GameFeaturesSubsystemSettings.h"
#include "GameplayTagsManager.h"
#include "Interfaces/IPluginManager.h"

#define LOCTEXT_NAMESPACE "FYcAngelScriptMixinModule"

DEFINE_LOG_CATEGORY(LogAngelscriptGFPLoader);


void FYcAngelScriptMixinModule::StartupModule()
{
	// 插件模块加载时加载GFPs的C++
	LoadGameFeature();
	// 在AngelScript加载完成前加载插件中的GamePlayTags
	LoadPluginTagConfigs();
}

void FYcAngelScriptMixinModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

void FYcAngelScriptMixinModule::LoadGameFeature()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	TSharedPtr<IPlugin> AngelscriptPlugin = IPluginManager::Get().FindPlugin("Angelscript");
	if (!AngelscriptPlugin || !AngelscriptPlugin->IsEnabled())
	{
		UE_LOG(LogAngelscriptGFPLoader, Log, TEXT("Angelscript Plugin not enabled. Doing nothing."));
		return;
	}

	UE_LOG(LogAngelscriptGFPLoader, Log, TEXT("Loading Game Feature Plugin C++ modules to allow Angelscript bindings"));
    
	UGameplayTagsManager::Get().PushDeferOnGameplayTagTreeChangedBroadcast();
    
	// load the C++ modules for GameFeaturePlugins before Angelscript loads to properly bind the C++ classes to script
	TArray<TSharedRef<IPlugin>> EnabledPlugins = IPluginManager::Get().GetEnabledPlugins();
	for (const TSharedRef<IPlugin>& Plugin : EnabledPlugins)
	{
		const FString& PluginDescriptorFilename = Plugin->GetDescriptorFileName();
        
		if (!PluginDescriptorFilename.IsEmpty() 
			&& GetDefault<UGameFeaturesSubsystemSettings>()->IsValidGameFeaturePlugin(FPaths::ConvertRelativePathToFull(PluginDescriptorFilename))
			&& FPaths::FileExists(PluginDescriptorFilename))
		{
			const FPluginDescriptor& PluginDesc = Plugin->GetDescriptor();
			if (PluginDesc.bExplicitlyLoaded && PluginDesc.Modules.Num() > 0)
			{
				const FString& PluginName = Plugin->GetName();

				UE_LOG(LogAngelscriptGFPLoader, Log, TEXT("Trying to load Game Feature Plugin: %s"), *PluginName);

				const bool bResult = IPluginManager::Get().MountExplicitlyLoadedPlugin(PluginName);
				if (!bResult)
				{
					UE_LOG(LogAngelscriptGFPLoader, Error, TEXT("Fail to load modules for Plugin %s"), *PluginName);
				}
			}
		}
	}
    
	UGameplayTagsManager::Get().PopDeferOnGameplayTagTreeChangedBroadcast();
}

/** @YiChen: 在绑定前加载插件中的GameplayTags*/
void FYcAngelScriptMixinModule::LoadPluginTagConfigs()
{
	TRACE_CPUPROFILER_EVENT_SCOPE(ASM_LoadPuginTagConfigs);
	// const FString PluginFolder = FPaths::GetPath(StateProperties.PluginInstalledFilename);
	// UGameplayTagsManager::Get().AddTagIniSearchPath(PluginFolder / TEXT("Config") / TEXT("Tags"));
	// Load the tags from configs of all enabled plugins
	IPluginManager& PluginManager = IPluginManager::Get();
	for (auto& Plugin : PluginManager.GetEnabledPluginsWithContent())
	{
		UGameplayTagsManager::Get().AddTagIniSearchPath(Plugin->GetBaseDir() / TEXT("Config") / TEXT("Tags"));
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FYcAngelScriptMixinModule, YcAngelScriptMixin)