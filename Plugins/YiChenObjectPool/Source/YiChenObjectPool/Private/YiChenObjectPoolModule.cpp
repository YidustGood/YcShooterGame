// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Modules/ModuleManager.h"

class FYiChenObjectPoolModule : public IModuleInterface
{
public:
	virtual void StartupModule() override
	{
	}

	virtual void ShutdownModule() override
	{
	}
};

IMPLEMENT_MODULE(FYiChenObjectPoolModule, YiChenObjectPool)
