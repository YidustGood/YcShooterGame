// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcSaveDomainRegistry.h"

#include "System/YcSaveDomainProvider.h"

namespace
{
    // 进程级 Provider 类型注册表。
    TArray<TSubclassOf<UYcSaveDomainProvider>>& GetProviderClasses()
    {
        static TArray<TSubclassOf<UYcSaveDomainProvider>> ProviderClasses;
        return ProviderClasses;
    }
}

void YcSaveDomainRegistry::RegisterProviderClass(const TSubclassOf<UYcSaveDomainProvider> ProviderClass)
{
    if (!ProviderClass)
    {
        return;
    }

    TArray<TSubclassOf<UYcSaveDomainProvider>>& ProviderClasses = GetProviderClasses();
    // 去重后再添加，避免重复注册。
    ProviderClasses.RemoveSingleSwap(ProviderClass);
    ProviderClasses.Add(ProviderClass);
}

void YcSaveDomainRegistry::UnregisterProviderClass(const TSubclassOf<UYcSaveDomainProvider> ProviderClass)
{
    if (!ProviderClass)
    {
        return;
    }

    GetProviderClasses().RemoveSingleSwap(ProviderClass);
}

TArray<TSubclassOf<UYcSaveDomainProvider>> YcSaveDomainRegistry::GetRegisteredProviderClasses()
{
    return GetProviderClasses();
}
