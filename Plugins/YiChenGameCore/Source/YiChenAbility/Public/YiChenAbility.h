// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

YICHENABILITY_API DECLARE_LOG_CATEGORY_EXTERN(LogYcAbilitySystem, Log, All);

class FYiChenAbilityModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};
