// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "System/YcMetaInventoryTypes.h"

namespace YcMetaInventoryVersion
{
	/** 快照协议当前版本（单一来源）。 */
	inline constexpr int32 CurrentSnapshotVersion = 4;

	/** 判断版本是否受支持。当前策略：仅支持当前版本。 */
	YICHENINVENTORY_API bool IsSupportedVersion(int32 Version);

	/** 创建一个空白根快照并填充协议元数据。 */
	YICHENINVENTORY_API FYcMetaInventoryRootSnapshot MakeEmptySnapshot(const FYcProfileIdentity& ProfileIdentity);

	/** 保存前统一填充协议元数据。 */
	YICHENINVENTORY_API void PrepareSnapshotForSave(const FYcProfileIdentity& ProfileIdentity, FYcMetaInventoryRootSnapshot& InOutSnapshot);
}
