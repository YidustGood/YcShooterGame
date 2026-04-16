// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YiChenInventory.h"
#include "System/YcInventorySaveDomainProvider.h"
#include "System/YcSaveDomainRegistry.h"

#define LOCTEXT_NAMESPACE "FYiChenInventoryModule"
DEFINE_LOG_CATEGORY(LogYcInventory);

void FYiChenInventoryModule::StartupModule()
{
	YcSaveDomainRegistry::RegisterProviderClass(UYcInventorySaveDomainProvider::StaticClass());
}

void FYiChenInventoryModule::ShutdownModule()
{
	YcSaveDomainRegistry::UnregisterProviderClass(UYcInventorySaveDomainProvider::StaticClass());
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FYiChenInventoryModule, YiChenInventory)
