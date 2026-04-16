// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcInventoryPersistenceExtensionRegistry.h"

#include "System/YcInventoryPersistenceExtensionProvider.h"

namespace
{
	// 进程级静态注册表：存放已注册的 Provider 类型。
	TArray<TSubclassOf<UYcInventoryPersistenceExtensionProvider>>& GetProviderClasses()
	{
		static TArray<TSubclassOf<UYcInventoryPersistenceExtensionProvider>> ProviderClasses;
		return ProviderClasses;
	}
}

void YcInventoryPersistenceExtensionRegistry::RegisterProviderClass(const TSubclassOf<UYcInventoryPersistenceExtensionProvider> ProviderClass)
{
	if (!ProviderClass)
	{
		return;
	}

	TArray<TSubclassOf<UYcInventoryPersistenceExtensionProvider>>& ProviderClasses = GetProviderClasses();
	// 去重后追加，避免重复注册同一类型。
	ProviderClasses.RemoveSingleSwap(ProviderClass);
	ProviderClasses.Add(ProviderClass);
}

void YcInventoryPersistenceExtensionRegistry::UnregisterProviderClass(const TSubclassOf<UYcInventoryPersistenceExtensionProvider> ProviderClass)
{
	if (!ProviderClass)
	{
		return;
	}

	GetProviderClasses().RemoveSingleSwap(ProviderClass);
}

TArray<TSubclassOf<UYcInventoryPersistenceExtensionProvider>> YcInventoryPersistenceExtensionRegistry::GetRegisteredProviderClasses()
{
	return GetProviderClasses();
}
