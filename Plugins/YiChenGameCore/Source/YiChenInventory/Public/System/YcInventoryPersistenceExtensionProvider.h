// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"
#include "UObject/Object.h"
#include "YcInventoryPersistenceExtensionProvider.generated.h"

class UYcInventoryManagerComponent;
class UYcInventoryItemInstance;
struct FYcItemInstanceId;

/**
 * 库存持久化扩展 Provider 抽象基类。
 * 各功能模块（例如 GridInventory）实现该类，并在模块启动时注册到注册表。
 */
UCLASS(Abstract)
class YICHENINVENTORY_API UYcInventoryPersistenceExtensionProvider : public UObject
{
	GENERATED_BODY()

public:
	/** 扩展唯一键（同一个库存上用于路由到对应 Provider）。 */
	virtual FName GetExtensionKey() const PURE_VIRTUAL(UYcInventoryPersistenceExtensionProvider::GetExtensionKey, return NAME_None;);
	/** 扩展载荷版本号（用于后续升级兼容）。 */
	virtual int32 GetExtensionVersion() const { return 1; }
	/** 当前 Provider 是否处理该库存组件类型。 */
	virtual bool CanHandleInventory(const UYcInventoryManagerComponent* Inventory) const PURE_VIRTUAL(UYcInventoryPersistenceExtensionProvider::CanHandleInventory, return false;);

	/** 从运行时库存状态构建扩展载荷。 */
	virtual bool BuildInventoryExtensionPayload(const UYcInventoryManagerComponent* Inventory, FInstancedStruct& OutPayload, FString& OutReason) const PURE_VIRTUAL(UYcInventoryPersistenceExtensionProvider::BuildInventoryExtensionPayload, return false;);
	/** 将扩展载荷应用到运行时库存状态。 */
	virtual bool ApplyInventoryExtensionPayload(UYcInventoryManagerComponent* Inventory, const FInstancedStruct& Payload, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& ItemMap, FString& OutReason) const PURE_VIRTUAL(UYcInventoryPersistenceExtensionProvider::ApplyInventoryExtensionPayload, return false;);
};
