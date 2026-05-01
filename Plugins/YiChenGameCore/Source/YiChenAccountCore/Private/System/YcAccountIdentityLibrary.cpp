// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcAccountIdentityLibrary.h"

bool UYcAccountIdentityLibrary::IsPlatformIdentityValid(const FYcPlatformIdentity& PlatformIdentity)
{
    return PlatformIdentity.IsValid();
}

bool UYcAccountIdentityLibrary::IsAccountIdentityValid(const FYcAccountIdentity& AccountIdentity)
{
    return AccountIdentity.IsValid();
}

bool UYcAccountIdentityLibrary::IsProfileIdentityValid(const FYcProfileIdentity& ProfileIdentity)
{
    return ProfileIdentity.IsValid();
}

bool UYcAccountIdentityLibrary::IsPlayerIdentityAuthenticated(const FYcPlayerIdentitySnapshot& PlayerIdentity)
{
    return PlayerIdentity.IsAuthenticated();
}

bool UYcAccountIdentityLibrary::HasActiveProfileIdentity(const FYcPlayerIdentitySnapshot& PlayerIdentity)
{
    return PlayerIdentity.HasActiveProfile();
}

bool UYcAccountIdentityLibrary::IsPlayerIdentityReady(const FYcPlayerIdentitySnapshot& PlayerIdentity)
{
    return PlayerIdentity.IsReady();
}

bool UYcAccountIdentityLibrary::IsPersistentOwnerKeyValid(const FYcPersistentOwnerKey& PersistentOwnerKey)
{
    return PersistentOwnerKey.IsValid();
}

FYcAccountIdentity UYcAccountIdentityLibrary::MakeAccountIdentity(const EYcAccountEnvironment Environment, const FString& AccountId)
{
    FYcAccountIdentity AccountIdentity;
    AccountIdentity.Environment = Environment;
    AccountIdentity.AccountId = AccountId;
    return AccountIdentity;
}

FYcProfileIdentity UYcAccountIdentityLibrary::MakeProfileIdentity(const EYcAccountEnvironment Environment, const FString& AccountId, const FString& ProfileId, const FString& DisplayName)
{
    FYcProfileIdentity ProfileIdentity;
    ProfileIdentity.AccountIdentity = MakeAccountIdentity(Environment, AccountId);
    ProfileIdentity.ProfileId = ProfileId;
    ProfileIdentity.DisplayName = DisplayName.IsEmpty() ? ProfileId : DisplayName;
    return ProfileIdentity;
}

FYcProfileIdentity UYcAccountIdentityLibrary::GetActiveProfileIdentity(const FYcPlayerIdentitySnapshot& PlayerIdentity)
{
    return PlayerIdentity.ActiveProfileIdentity;
}

FYcPersistentOwnerKey UYcAccountIdentityLibrary::MakePersistentOwnerKey(const EYcPersistentOwnerType OwnerType, const FString& OwnerId)
{
    FYcPersistentOwnerKey PersistentOwnerKey;
    PersistentOwnerKey.OwnerType = OwnerType;
    PersistentOwnerKey.OwnerId = OwnerId;
    return PersistentOwnerKey;
}

FYcPersistentOwnerKey UYcAccountIdentityLibrary::MakeAccountOwnerKey(const FYcAccountIdentity& AccountIdentity)
{
    return MakePersistentOwnerKey(EYcPersistentOwnerType::Account, AccountIdentity.ToOwnerId());
}

FYcPersistentOwnerKey UYcAccountIdentityLibrary::MakeProfileOwnerKey(const FYcProfileIdentity& ProfileIdentity)
{
    return MakePersistentOwnerKey(EYcPersistentOwnerType::Profile, ProfileIdentity.ToOwnerId());
}

FYcPersistentOwnerKey UYcAccountIdentityLibrary::GetPlayerPersistentOwnerKey(const FYcPlayerIdentitySnapshot& PlayerIdentity)
{
    return PlayerIdentity.GetPersistentOwnerKey();
}

FString UYcAccountIdentityLibrary::GetAccountOwnerId(const FYcAccountIdentity& AccountIdentity)
{
    return AccountIdentity.ToOwnerId();
}

FString UYcAccountIdentityLibrary::GetProfileOwnerId(const FYcProfileIdentity& ProfileIdentity)
{
    return ProfileIdentity.ToOwnerId();
}

FString UYcAccountIdentityLibrary::GetPlayerPersistentOwnerId(const FYcPlayerIdentitySnapshot& PlayerIdentity)
{
    return PlayerIdentity.GetPersistentOwnerId();
}

FString UYcAccountIdentityLibrary::GetPersistentOwnerKeyId(const FYcPersistentOwnerKey& PersistentOwnerKey)
{
    return PersistentOwnerKey.OwnerId;
}

FString UYcAccountIdentityLibrary::DescribePlatformIdentity(const FYcPlatformIdentity& PlatformIdentity)
{
    return PlatformIdentity.ToDebugString();
}

FString UYcAccountIdentityLibrary::DescribeAccountIdentity(const FYcAccountIdentity& AccountIdentity)
{
    return AccountIdentity.ToDebugString();
}

FString UYcAccountIdentityLibrary::DescribeProfileIdentity(const FYcProfileIdentity& ProfileIdentity)
{
    return ProfileIdentity.ToDebugString();
}

FString UYcAccountIdentityLibrary::DescribePlayerIdentity(const FYcPlayerIdentitySnapshot& PlayerIdentity)
{
    return PlayerIdentity.ToDebugString();
}

FString UYcAccountIdentityLibrary::DescribePersistentOwnerKey(const FYcPersistentOwnerKey& PersistentOwnerKey)
{
    return PersistentOwnerKey.ToDebugString();
}
