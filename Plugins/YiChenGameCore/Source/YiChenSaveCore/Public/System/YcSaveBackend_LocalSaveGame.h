// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "System/YcSaveBackendProvider.h"
#include "YcSaveBackend_LocalSaveGame.generated.h"

class UYcProfileSaveGame;

/** 本地 SaveGame 后端实现。 */
UCLASS()
class YICHENSAVECORE_API UYcSaveBackend_LocalSaveGame : public UYcSaveBackendProvider
{
    GENERATED_BODY()

public:
    /** 从本地槽位同步加载 Profile 根对象。 */
    virtual EYcSaveBackendResult LoadProfileRootSync(const FYcProfileSaveKey& ProfileKey, FYcProfileSaveRoot& OutRoot, FString& OutReason) override;
    /** 将 Profile 根对象同步保存到本地槽位。 */
    virtual bool SaveProfileRootSync(const FYcProfileSaveKey& ProfileKey, const FYcProfileSaveRoot& Root, FString& OutReason) override;
    /** 从本地槽位加载 Profile 根对象。 */
    virtual void LoadProfileRootAsync(const FYcProfileSaveKey& ProfileKey, const FYcOnLoadProfileRoot& Completion) override;
    /** 将 Profile 根对象保存到本地槽位。 */
    virtual void SaveProfileRootAsync(const FYcProfileSaveKey& ProfileKey, const FYcProfileSaveRoot& Root, const FYcOnSaveProfileRoot& Completion) override;

private:
    /** 统一构建本地槽位名。 */
    static FString BuildSlotName(const FYcProfileSaveKey& ProfileKey);
};
