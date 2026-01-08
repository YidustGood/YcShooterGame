// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

YICHENSHOOTERCORE_API DECLARE_LOG_CATEGORY_EXTERN(LogYcShooterCore, Log, All);

class FYiChenShooterCoreModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
