#pragma once

#include "CoreMinimal.h"
#include "IAssetTypeActions.h"
#include "Modules/ModuleManager.h"

#include "Logging/LogMacros.h"

DECLARE_LOG_CATEGORY_EXTERN(LogYcGameCoreEditor, Log, All);

class FYcGameCoreEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();

	FDelegateHandle ToolMenusStartupHandle;
	TWeakPtr<IAssetTypeActions> YcContextEffectsLibraryAssetAction;
};
