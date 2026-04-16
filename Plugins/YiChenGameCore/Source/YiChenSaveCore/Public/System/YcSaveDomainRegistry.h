// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UYcSaveDomainProvider;

/** SaveCore 域 Provider 注册表。 */
namespace YcSaveDomainRegistry
{
    /** 注册一个域 Provider 类型。 */
    YICHENSAVECORE_API void RegisterProviderClass(TSubclassOf<UYcSaveDomainProvider> ProviderClass);
    /** 注销一个域 Provider 类型。 */
    YICHENSAVECORE_API void UnregisterProviderClass(TSubclassOf<UYcSaveDomainProvider> ProviderClass);
    /** 获取当前全部已注册 Provider 类型。 */
    YICHENSAVECORE_API TArray<TSubclassOf<UYcSaveDomainProvider>> GetRegisteredProviderClasses();
}
