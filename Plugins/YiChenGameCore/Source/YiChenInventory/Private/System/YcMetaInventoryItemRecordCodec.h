// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "System/YcMetaInventoryTypes.h"

class UYcInventoryItemInstance;

namespace YcMetaInventoryItemRecordCodec
{
	/** 从运行时物品状态构建持久化记录。 */
	void ExportFromItem(const UYcInventoryItemInstance& Item, const TArray<FYcMetaItemExtensionPayload>* UnknownPayloads, FYcMetaInventoryItemRecord& OutRecord);

	/** 将持久化记录恢复到运行时物品状态，并返回未识别扩展载荷。 */
	void ImportToItem(UYcInventoryItemInstance& Item, const FYcMetaInventoryItemRecord& InRecord, TArray<FYcMetaItemExtensionPayload>& OutUnknownPayloads);
}

