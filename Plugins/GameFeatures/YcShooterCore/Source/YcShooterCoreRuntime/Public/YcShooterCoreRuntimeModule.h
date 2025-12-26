// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

YCSHOOTERCORERUNTIME_API DECLARE_LOG_CATEGORY_EXTERN(LogYcShooterCore, Log, All);

class FYcShooterCoreRuntimeModule : public IModuleInterface
{
public:
	//~IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	//~End of IModuleInterface
};
