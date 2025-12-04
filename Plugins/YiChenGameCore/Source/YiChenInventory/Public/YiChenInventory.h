// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"
#include "Logging/LogMacros.h"

YICHENINVENTORY_API DECLARE_LOG_CATEGORY_EXTERN(LogYcInventory, Log, All);

class FYiChenInventoryModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
