// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "YcInventoryPersistenceProvider.generated.h"

struct FYcMetaInventoryRootSnapshot;

/**
 * 存档持久化提供者抽象接口。
 * 用于解耦“快照构建/恢复逻辑”与“底层存储介质（本地存档/网络服务）”。
 */
UCLASS(Abstract, BlueprintType)
class YICHENINVENTORY_API UYcInventoryPersistenceProvider : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * 加载指定账号的库存快照。
	 * @param AccountId 账号标识。
	 * @param OutSnapshot 输出的库存根快照。
	 * @return 是否加载成功。
	 */
	virtual bool LoadSnapshot(const FString& AccountId, FYcMetaInventoryRootSnapshot& OutSnapshot) PURE_VIRTUAL(UYcInventoryPersistenceProvider::LoadSnapshot, return false;);

	/**
	 * 保存指定账号的库存快照。
	 * @param AccountId 账号标识。
	 * @param Snapshot 待保存的库存根快照。
	 * @return 是否保存成功。
	 */
	virtual bool SaveSnapshot(const FString& AccountId, const FYcMetaInventoryRootSnapshot& Snapshot) PURE_VIRTUAL(UYcInventoryPersistenceProvider::SaveSnapshot, return false;);
};
