// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UYcInventoryPersistenceExtensionProvider;

/** 库存持久化扩展 Provider 注册表。 */
namespace YcInventoryPersistenceExtensionRegistry
{
	/** 注册 Provider 类。 */
	YICHENINVENTORY_API void RegisterProviderClass(TSubclassOf<UYcInventoryPersistenceExtensionProvider> ProviderClass);

	/** 注销 Provider 类。 */
	YICHENINVENTORY_API void UnregisterProviderClass(TSubclassOf<UYcInventoryPersistenceExtensionProvider> ProviderClass);

	/** 获取当前已注册的 Provider 类列表。 */
	YICHENINVENTORY_API TArray<TSubclassOf<UYcInventoryPersistenceExtensionProvider>> GetRegisteredProviderClasses();
}
