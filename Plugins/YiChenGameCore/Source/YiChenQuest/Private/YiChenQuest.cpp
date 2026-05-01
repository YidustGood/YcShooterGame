// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YiChenQuest.h"

#include "System/YcQuestSaveDomainProvider.h"
#include "System/YcSaveDomainRegistry.h"

DEFINE_LOG_CATEGORY(LogYcQuest);

void FYiChenQuestModule::StartupModule()
{
    YcSaveDomainRegistry::RegisterProviderClass(UYcQuestSaveDomainProvider::StaticClass());
}

void FYiChenQuestModule::ShutdownModule()
{
    YcSaveDomainRegistry::UnregisterProviderClass(UYcQuestSaveDomainProvider::StaticClass());
}

IMPLEMENT_MODULE(FYiChenQuestModule, YiChenQuest)

