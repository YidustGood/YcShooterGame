// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

class FYiChenGameCoreModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
