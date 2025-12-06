// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

YICHENGAMECORE_API DECLARE_LOG_CATEGORY_EXTERN(LogYcGameCore, Log, All);

class FYiChenGameCoreModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
