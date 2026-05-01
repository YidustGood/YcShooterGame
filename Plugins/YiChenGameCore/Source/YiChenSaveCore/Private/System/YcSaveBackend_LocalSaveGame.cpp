// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcSaveBackend_LocalSaveGame.h"

#include "System/YcProfileSaveGame.h"
#include "YiChenSaveCore.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcSaveBackend_LocalSaveGame)

namespace
{
    // 本地 SaveGame 用户索引（单用户）。
    static constexpr int32 SaveUserIndex = 0;
}

FString UYcSaveBackend_LocalSaveGame::BuildSlotName(const FYcProfileSaveKey& ProfileKey)
{
    // 统一槽位命名：YcProfile_<Env>_<Account>_<Profile>
    if (!ProfileKey.IsValid())
    {
        UE_LOG(LogYcSaveCore, Error, TEXT("BuildSlotName failed: invalid profile key."));
        return FString();
    }
    return FString::Printf(TEXT("YcProfile_%d_%s_%s"), static_cast<int32>(ProfileKey.Environment), *ProfileKey.AccountId, *ProfileKey.ProfileId);
}

EYcSaveBackendResult UYcSaveBackend_LocalSaveGame::LoadProfileRootSync(const FYcProfileSaveKey& ProfileKey, FYcProfileSaveRoot& Root, FString& OutReason)
{
    const FString SlotName = BuildSlotName(ProfileKey);
    if (SlotName.IsEmpty())
    {
        OutReason = TEXT("Load failed: invalid profile key.");
        Root = FYcProfileSaveRoot();
        return EYcSaveBackendResult::Failed;
    }

    if (!UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex))
    {
        Root = FYcProfileSaveRoot();
        OutReason.Reset();
        return EYcSaveBackendResult::NotFound;
    }

    USaveGame* SaveGameObject = UGameplayStatics::LoadGameFromSlot(SlotName, SaveUserIndex);
    UYcProfileSaveGame* ProfileSave = Cast<UYcProfileSaveGame>(SaveGameObject);
    if (!ProfileSave)
    {
        OutReason = FString::Printf(TEXT("Load failed: slot '%s' exists but save type mismatched."), *SlotName);
        UE_LOG(LogYcSaveCore, Warning, TEXT("%s"), *OutReason);
        Root = FYcProfileSaveRoot();
        return EYcSaveBackendResult::Failed;
    }

    Root = ProfileSave->Root;
    OutReason.Reset();
    return EYcSaveBackendResult::Success;
}

bool UYcSaveBackend_LocalSaveGame::SaveProfileRootSync(const FYcProfileSaveKey& ProfileKey, const FYcProfileSaveRoot& Root, FString& OutReason)
{
    const FString SlotName = BuildSlotName(ProfileKey);
    if (SlotName.IsEmpty())
    {
        OutReason = TEXT("Save failed: invalid profile key.");
        return false;
    }

    UYcProfileSaveGame* SaveObject = Cast<UYcProfileSaveGame>(UGameplayStatics::CreateSaveGameObject(UYcProfileSaveGame::StaticClass()));
    if (!SaveObject)
    {
        OutReason = TEXT("Save failed: CreateSaveGameObject returned null.");
        UE_LOG(LogYcSaveCore, Error, TEXT("%s"), *OutReason);
        return false;
    }

    SaveObject->Root = Root;
    // 以传入 ProfileKey 为准覆盖主键信息。
    SaveObject->Root.Environment = ProfileKey.Environment;
    SaveObject->Root.AccountId = ProfileKey.AccountId;
    SaveObject->Root.ProfileId = ProfileKey.ProfileId;

    const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveObject, SlotName, SaveUserIndex);
    if (!bSaved)
    {
        OutReason = FString::Printf(TEXT("Save failed: slot '%s' write failed."), *SlotName);
        UE_LOG(LogYcSaveCore, Error, TEXT("%s"), *OutReason);
        return false;
    }

    OutReason.Reset();
    return true;
}

void UYcSaveBackend_LocalSaveGame::LoadProfileRootAsync(const FYcProfileSaveKey& ProfileKey, const FYcOnLoadProfileRoot& Completion)
{
    FYcProfileSaveRoot Root;
    const FString SlotName = BuildSlotName(ProfileKey);
    if (SlotName.IsEmpty())
    {
        Completion.ExecuteIfBound(EYcSaveBackendResult::Failed, Root, TEXT("Load failed: invalid profile key."));
        return;
    }

    if (!UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex))
    {
        Completion.ExecuteIfBound(EYcSaveBackendResult::NotFound, Root, FString());
        return;
    }

    UGameplayStatics::AsyncLoadGameFromSlot(SlotName, SaveUserIndex,
        FAsyncLoadGameFromSlotDelegate::CreateWeakLambda(this, [Completion, SlotName](const FString& LoadedSlotName, const int32 UserIndex, USaveGame* SaveGameObject)
        {
            (void)LoadedSlotName;
            (void)UserIndex;

            FYcProfileSaveRoot LoadedRoot;
            UYcProfileSaveGame* ProfileSave = Cast<UYcProfileSaveGame>(SaveGameObject);
            if (!ProfileSave)
            {
                const FString Reason = FString::Printf(TEXT("Load failed: slot '%s' exists but save type mismatched."), *SlotName);
                UE_LOG(LogYcSaveCore, Warning, TEXT("%s"), *Reason);
                Completion.ExecuteIfBound(EYcSaveBackendResult::Failed, LoadedRoot, Reason);
                return;
            }

            LoadedRoot = ProfileSave->Root;
            Completion.ExecuteIfBound(EYcSaveBackendResult::Success, LoadedRoot, FString());
        }));
}

void UYcSaveBackend_LocalSaveGame::SaveProfileRootAsync(const FYcProfileSaveKey& ProfileKey, const FYcProfileSaveRoot& Root, const FYcOnSaveProfileRoot& Completion)
{
    const FString SlotName = BuildSlotName(ProfileKey);
    if (SlotName.IsEmpty())
    {
        Completion.ExecuteIfBound(false, TEXT("Save failed: invalid profile key."));
        return;
    }

    UYcProfileSaveGame* SaveObject = Cast<UYcProfileSaveGame>(UGameplayStatics::CreateSaveGameObject(UYcProfileSaveGame::StaticClass()));
    if (!SaveObject)
    {
        const FString Reason = TEXT("Save failed: CreateSaveGameObject returned null.");
        UE_LOG(LogYcSaveCore, Error, TEXT("%s"), *Reason);
        Completion.ExecuteIfBound(false, Reason);
        return;
    }

    SaveObject->Root = Root;
    SaveObject->Root.Environment = ProfileKey.Environment;
    SaveObject->Root.AccountId = ProfileKey.AccountId;
    SaveObject->Root.ProfileId = ProfileKey.ProfileId;

    UGameplayStatics::AsyncSaveGameToSlot(SaveObject, SlotName, SaveUserIndex,
        FAsyncSaveGameToSlotDelegate::CreateWeakLambda(this, [Completion, SlotName](const FString& SavedSlotName, const int32 UserIndex, const bool bSuccess)
        {
            (void)SavedSlotName;
            (void)UserIndex;

            if (!bSuccess)
            {
                const FString Reason = FString::Printf(TEXT("Save failed: slot '%s' write failed."), *SlotName);
                UE_LOG(LogYcSaveCore, Error, TEXT("%s"), *Reason);
                Completion.ExecuteIfBound(false, Reason);
                return;
            }

            Completion.ExecuteIfBound(true, FString());
        }));
}
