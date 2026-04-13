// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "System/YcInventoryPersistenceProvider.h"
#include "YcInventoryPersistenceProvider_LocalSave.generated.h"

/**
 * 本地存档实现（SaveGame）持久化提供者。
 * 当前用于局外仓库/背包快照的本地读写。
 */
UCLASS(BlueprintType)
class YICHENINVENTORY_API UYcInventoryPersistenceProvider_LocalSave : public UYcInventoryPersistenceProvider
{
	GENERATED_BODY()

public:
	/** 从本地 SaveGame 读取快照。 */
	virtual bool LoadSnapshot(const FString& AccountId, FYcMetaInventoryRootSnapshot& OutSnapshot) override;
	/** 将快照写入本地 SaveGame。 */
	virtual bool SaveSnapshot(const FString& AccountId, const FYcMetaInventoryRootSnapshot& Snapshot) override;

private:
	/** 根据账号生成唯一的 SaveGame Slot 名称。 */
	static FString BuildSlotName(const FString& AccountId);
};
