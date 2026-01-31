// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

YCANGELSCRIPTMIXIN_API DECLARE_LOG_CATEGORY_EXTERN(LogAngelscriptGFPLoader, Log, All);

class FYcAngelScriptMixinModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	// 在AngelScript加载之前先加载GameFeature的C++类，或许其中的GamePlayTags扫描路径也可以在这里进行操作不过需要考虑模块加载顺序，https://discord.com/channels/551756549962465299/1091937511435149322
	void LoadGameFeature();

	/** @YiChen: 在绑定前加载插件中的GameplayTags*/
	void LoadPluginTagConfigs();
};
