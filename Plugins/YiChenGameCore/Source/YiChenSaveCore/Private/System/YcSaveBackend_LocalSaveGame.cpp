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

FString UYcSaveBackend_LocalSaveGame::BuildSlotName(const FYcProfileKey& ProfileKey)
{
    // 统一槽位命名：YcProfile_<Account>_<Profile>
    const FString SafeAccount = ProfileKey.AccountId.IsEmpty() ? TEXT("DefaultAccount") : ProfileKey.AccountId;
    const FString SafeProfile = ProfileKey.ProfileId.IsEmpty() ? TEXT("Slot01") : ProfileKey.ProfileId;
    return FString::Printf(TEXT("YcProfile_%s_%s"), *SafeAccount, *SafeProfile);
}

void UYcSaveBackend_LocalSaveGame::LoadProfileRootAsync(const FYcProfileKey& ProfileKey, const FYcOnLoadProfileRoot& Completion)
{
    FYcProfileSaveRoot Root;
    const FString SlotName = BuildSlotName(ProfileKey);

    if (!UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex))
    {
        // 不存在时返回 NotFound，由上层决定初始化逻辑。
        Completion.ExecuteIfBound(EYcSaveBackendResult::NotFound, Root, FString());
        return;
    }

    USaveGame* SaveGameObject = UGameplayStatics::LoadGameFromSlot(SlotName, SaveUserIndex);
    UYcProfileSaveGame* ProfileSave = Cast<UYcProfileSaveGame>(SaveGameObject);
    if (!ProfileSave)
    {
        const FString Reason = FString::Printf(TEXT("Load failed: slot '%s' exists but save type mismatched."), *SlotName);
        UE_LOG(LogYcSaveCore, Warning, TEXT("%s"), *Reason);
        Completion.ExecuteIfBound(EYcSaveBackendResult::Failed, Root, Reason);
        return;
    }

    Root = ProfileSave->Root;
    // 加载成功后直接返回根对象。
    Completion.ExecuteIfBound(EYcSaveBackendResult::Success, Root, FString());
}

void UYcSaveBackend_LocalSaveGame::SaveProfileRootAsync(const FYcProfileKey& ProfileKey, const FYcProfileSaveRoot& Root, const FYcOnSaveProfileRoot& Completion)
{
    const FString SlotName = BuildSlotName(ProfileKey);

    UYcProfileSaveGame* SaveObject = Cast<UYcProfileSaveGame>(UGameplayStatics::CreateSaveGameObject(UYcProfileSaveGame::StaticClass()));
    if (!SaveObject)
    {
        const FString Reason = TEXT("Save failed: CreateSaveGameObject returned null.");
        UE_LOG(LogYcSaveCore, Error, TEXT("%s"), *Reason);
        Completion.ExecuteIfBound(false, Reason);
        return;
    }

    SaveObject->Root = Root;
    // 以传入 ProfileKey 为准覆盖主键信息。
    SaveObject->Root.AccountId = ProfileKey.AccountId;
    SaveObject->Root.ProfileId = ProfileKey.ProfileId;

    const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveObject, SlotName, SaveUserIndex);
    if (!bSaved)
    {
        const FString Reason = FString::Printf(TEXT("Save failed: slot '%s' write failed."), *SlotName);
        UE_LOG(LogYcSaveCore, Error, TEXT("%s"), *Reason);
        Completion.ExecuteIfBound(false, Reason);
        return;
    }

    Completion.ExecuteIfBound(true, FString());
}
