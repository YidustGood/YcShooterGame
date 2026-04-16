// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcGridInventoryRuntimeModule.h"

#include "System/YcInventoryPersistenceExtensionRegistry.h"
#include "YcGridInventoryPersistenceExtensionProvider.h"

#define LOCTEXT_NAMESPACE "FYcGridInventoryRuntimeModule"

void FYcGridInventoryRuntimeModule::StartupModule()
{
	YcInventoryPersistenceExtensionRegistry::RegisterProviderClass(UYcGridInventoryPersistenceExtensionProvider::StaticClass());
}

void FYcGridInventoryRuntimeModule::ShutdownModule()
{
	YcInventoryPersistenceExtensionRegistry::UnregisterProviderClass(UYcGridInventoryPersistenceExtensionProvider::StaticClass());
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FYcGridInventoryRuntimeModule, YcGridInventoryRuntime)
