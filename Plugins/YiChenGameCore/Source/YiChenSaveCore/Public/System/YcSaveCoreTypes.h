// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "YcSaveCoreTypes.generated.h"

UENUM(BlueprintType)
enum class EYcSaveBackendResult : uint8
{
    /** 后端读写成功。 */
    Success,
    /** 目标档案不存在。 */
    NotFound,
    /** 后端执行失败。 */
    Failed
};

/** Profile 唯一键：账号 + 档位。 */
USTRUCT(BlueprintType)
struct YICHENSAVECORE_API FYcProfileKey
{
    GENERATED_BODY()

    /** 默认构造。 */
    FYcProfileKey() = default;

    /** 便捷构造。 */
    FYcProfileKey(const FString& InAccountId, const FString& InProfileId)
        : AccountId(InAccountId)
        , ProfileId(InProfileId)
    {
    }

    /** 账号标识。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FString AccountId;

    /** 档位标识。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FString ProfileId;

    /** 是否为有效键（账号与档位都非空）。 */
    bool IsValid() const
    {
        return !AccountId.IsEmpty() && !ProfileId.IsEmpty();
    }

    /** 调试输出文本。 */
    FString ToDebugString() const
    {
        return FString::Printf(TEXT("%s::%s"), *AccountId, *ProfileId);
    }

    /** 键相等判断。 */
    bool operator==(const FYcProfileKey& Other) const
    {
        return AccountId == Other.AccountId && ProfileId == Other.ProfileId;
    }
};

FORCEINLINE uint32 GetTypeHash(const FYcProfileKey& Key)
{
    // 组合账号和档位哈希，便于作为 TMap/TSet 键使用。
    return HashCombine(GetTypeHash(Key.AccountId), GetTypeHash(Key.ProfileId));
}

/** 单个业务域载荷。 */
USTRUCT(BlueprintType)
struct YICHENSAVECORE_API FYcProfileDomainPayload
{
    GENERATED_BODY()

    /** 业务域唯一键（例如 Inventory、LevelProgress）。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FName DomainKey = NAME_None;

    /** 业务域载荷版本。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    int32 Version = 1;

    /** 业务域二进制载荷。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    TArray<uint8> PayloadBytes;
};

/** Profile 根存档对象。 */
USTRUCT(BlueprintType)
struct YICHENSAVECORE_API FYcProfileSaveRoot
{
    GENERATED_BODY()

    /** 账号标识。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FString AccountId;

    /** 档位标识。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    FString ProfileId;

    /** 根快照版本（SaveCore 级）。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    int32 SnapshotVersion = 1;

    /** 最近保存时间（Unix 秒）。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    int64 LastSavedUnixTime = 0;

    /** 所有业务域载荷集合。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save")
    TArray<FYcProfileDomainPayload> Domains;
};
