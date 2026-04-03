// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

YICHENCOMBATCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogCombatCore, Log, All);

class FYiChenCombatCoreModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
