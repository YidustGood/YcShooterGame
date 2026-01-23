#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#define DEBUG_INIT_STATE 0
#define DEBUG_INPUT_STATE 0

YICHENGAMEPLAY_API DECLARE_LOG_CATEGORY_EXTERN(LogYcGameplay, Log, All);
YICHENGAMEPLAY_API DECLARE_LOG_CATEGORY_EXTERN(LogYcInput, Log, All);

class FYiChenGameplayModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
