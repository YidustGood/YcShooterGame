// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "System/YcSaveCoreTypes.h"
#include "YcSaveBackendProvider.generated.h"

/** 后端加载完成回调。 */
DECLARE_DELEGATE_ThreeParams(FYcOnLoadProfileRoot, EYcSaveBackendResult /*Result*/, const FYcProfileSaveRoot& /*Root*/, const FString& /*Reason*/);
/** 后端保存完成回调。 */
DECLARE_DELEGATE_TwoParams(FYcOnSaveProfileRoot, bool /*bSuccess*/, const FString& /*Reason*/);

/**
 * 存档后端抽象。
 * 负责 Profile 根对象的实际读写介质实现（本地/远端）。
 */
UCLASS(Abstract, BlueprintType)
class YICHENSAVECORE_API UYcSaveBackendProvider : public UObject
{
    GENERATED_BODY()

public:
    /** 异步加载指定 Profile 根对象。 */
    virtual void LoadProfileRootAsync(const FYcProfileKey& ProfileKey, const FYcOnLoadProfileRoot& Completion) PURE_VIRTUAL(UYcSaveBackendProvider::LoadProfileRootAsync, );
    /** 异步保存指定 Profile 根对象。 */
    virtual void SaveProfileRootAsync(const FYcProfileKey& ProfileKey, const FYcProfileSaveRoot& Root, const FYcOnSaveProfileRoot& Completion) PURE_VIRTUAL(UYcSaveBackendProvider::SaveProfileRootAsync, );
};
