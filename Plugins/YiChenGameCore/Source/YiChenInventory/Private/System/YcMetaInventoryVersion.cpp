// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcMetaInventoryVersion.h"

namespace
{
	int64 GetNowUnixTime()
	{
		const FDateTime UtcNow = FDateTime::UtcNow();
		const FDateTime UnixEpoch(1970, 1, 1);
		return (UtcNow - UnixEpoch).GetTotalSeconds();
	}
}

bool YcMetaInventoryVersion::IsSupportedVersion(const int32 Version)
{
	return Version == CurrentSnapshotVersion;
}

FYcMetaInventoryRootSnapshot YcMetaInventoryVersion::MakeEmptySnapshot(const FString& AccountId)
{
	FYcMetaInventoryRootSnapshot Snapshot;
	Snapshot.AccountId = AccountId;
	Snapshot.SnapshotVersion = CurrentSnapshotVersion;
	Snapshot.LastSavedUnixTime = GetNowUnixTime();
	return Snapshot;
}

void YcMetaInventoryVersion::PrepareSnapshotForSave(const FString& AccountId, FYcMetaInventoryRootSnapshot& InOutSnapshot)
{
	InOutSnapshot.AccountId = AccountId;
	InOutSnapshot.SnapshotVersion = CurrentSnapshotVersion;
	InOutSnapshot.LastSavedUnixTime = GetNowUnixTime();
}
