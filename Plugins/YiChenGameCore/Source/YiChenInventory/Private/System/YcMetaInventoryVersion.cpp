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

FYcMetaInventoryRootSnapshot YcMetaInventoryVersion::MakeEmptySnapshot(const FYcProfileIdentity& ProfileIdentity)
{
	FYcMetaInventoryRootSnapshot Snapshot;
	Snapshot.Environment = ProfileIdentity.AccountIdentity.Environment;
	Snapshot.AccountId = ProfileIdentity.AccountIdentity.AccountId;
	Snapshot.ProfileId = ProfileIdentity.ProfileId;
	Snapshot.SnapshotVersion = CurrentSnapshotVersion;
	Snapshot.LastSavedUnixTime = GetNowUnixTime();
	return Snapshot;
}

void YcMetaInventoryVersion::PrepareSnapshotForSave(const FYcProfileIdentity& ProfileIdentity, FYcMetaInventoryRootSnapshot& InOutSnapshot)
{
	InOutSnapshot.Environment = ProfileIdentity.AccountIdentity.Environment;
	InOutSnapshot.AccountId = ProfileIdentity.AccountIdentity.AccountId;
	InOutSnapshot.ProfileId = ProfileIdentity.ProfileId;
	InOutSnapshot.SnapshotVersion = CurrentSnapshotVersion;
	InOutSnapshot.LastSavedUnixTime = GetNowUnixTime();
}
