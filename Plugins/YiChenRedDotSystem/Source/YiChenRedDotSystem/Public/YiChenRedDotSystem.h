// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

YICHENREDDOTSYSTEM_API DECLARE_LOG_CATEGORY_EXTERN(LogYcRedDot, Log, All);

class FYiChenRedDotSystemModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
