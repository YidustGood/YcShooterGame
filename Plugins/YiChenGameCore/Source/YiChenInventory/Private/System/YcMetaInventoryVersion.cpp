// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcMetaInventoryVersion.h"
#include "Utils/CommonSimpleUtil.h"

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
	Snapshot.LastSavedUnixTime = YcTimeUtils::GetUtcNowUnixTimestampSeconds();
	return Snapshot;
}

void YcMetaInventoryVersion::PrepareSnapshotForSave(const FYcProfileIdentity& ProfileIdentity, FYcMetaInventoryRootSnapshot& InOutSnapshot)
{
	InOutSnapshot.Environment = ProfileIdentity.AccountIdentity.Environment;
	InOutSnapshot.AccountId = ProfileIdentity.AccountIdentity.AccountId;
	InOutSnapshot.ProfileId = ProfileIdentity.ProfileId;
	InOutSnapshot.SnapshotVersion = CurrentSnapshotVersion;
	InOutSnapshot.LastSavedUnixTime = YcTimeUtils::GetUtcNowUnixTimestampSeconds();
}
