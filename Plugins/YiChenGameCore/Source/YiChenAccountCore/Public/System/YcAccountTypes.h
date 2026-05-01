// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "YcAccountTypes.generated.h"

/** 账号环境，用于区分 PIE、离线、本地开发服或正式环境下的账号归属。 */
UENUM(BlueprintType)
enum class EYcAccountEnvironment : uint8
{
    Unknown UMETA(DisplayName = "Unknown"),
    Development UMETA(DisplayName = "Development"),
    PIE UMETA(DisplayName = "PIE"),
    Offline UMETA(DisplayName = "Offline"),
    Production UMETA(DisplayName = "Production")
};

/** 平台账号来源，描述玩家身份是如何被识别出来的。 */
UENUM(BlueprintType)
enum class EYcPlatformAccountSource : uint8
{
    None UMETA(DisplayName = "None"),
    LocalOffline UMETA(DisplayName = "LocalOffline"),
    CommandLine UMETA(DisplayName = "CommandLine"),
    PlatformService UMETA(DisplayName = "PlatformService")
};

/** 账号会话状态机。 */
UENUM(BlueprintType)
enum class EYcAccountSessionState : uint8
{
    Uninitialized UMETA(DisplayName = "Uninitialized"),
    SignedOut UMETA(DisplayName = "SignedOut"),
    Authenticating UMETA(DisplayName = "Authenticating"),
    AuthenticatedNoProfile UMETA(DisplayName = "AuthenticatedNoProfile"),
    ProfileSelecting UMETA(DisplayName = "ProfileSelecting"),
    Ready UMETA(DisplayName = "Ready"),
    Refreshing UMETA(DisplayName = "Refreshing"),
    Error UMETA(DisplayName = "Error")
};

/** 持久化数据的归属类型，用于背包、存档、任务等系统定位数据拥有者。 */
UENUM(BlueprintType)
enum class EYcPersistentOwnerType : uint8
{
    None UMETA(DisplayName = "None"),
    Account UMETA(DisplayName = "Account"),
    Profile UMETA(DisplayName = "Profile"),
    SharedGroup UMETA(DisplayName = "SharedGroup"),
    Match UMETA(DisplayName = "Match")
};

/**
 * 平台侧身份。
 * 表示玩家在平台、命令行或本地离线环境中的原始用户标识，
 * 是后续推导账号身份与恢复本地缓存会话的入口。
 */
USTRUCT(BlueprintType)
struct YICHENACCOUNTCORE_API FYcPlatformIdentity
{
    GENERATED_BODY()

    /** 平台身份来源。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    EYcPlatformAccountSource Source = EYcPlatformAccountSource::None;

    /** 平台用户唯一标识。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    FString PlatformUserId;

    /** 平台侧展示名。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    FString DisplayName;

    /** 当前登录过程生成或返回的认证票据。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    FString AuthTicket;

    /** 是否具备可用于识别用户的有效平台身份。 */
    bool IsValid() const
    {
        return Source != EYcPlatformAccountSource::None && !PlatformUserId.IsEmpty();
    }

    /** 生成便于日志排查的调试字符串。 */
    FString ToDebugString() const
    {
        return FString::Printf(TEXT("%d::%s"), static_cast<int32>(Source), *PlatformUserId);
    }

    /** 平台身份全量相等比较。 */
    bool operator==(const FYcPlatformIdentity& Other) const
    {
        return Source == Other.Source
            && PlatformUserId == Other.PlatformUserId
            && DisplayName == Other.DisplayName
            && AuthTicket == Other.AuthTicket;
    }
};

/**
 * 账号身份。
 * 平台身份经过账号体系映射后得到的稳定账号标识，
 * 用于跨会话、跨角色定位同一玩家账号。
 */
USTRUCT(BlueprintType)
struct YICHENACCOUNTCORE_API FYcAccountIdentity
{
    GENERATED_BODY()

    /** 当前账号所属的运行环境。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    EYcAccountEnvironment Environment = EYcAccountEnvironment::Unknown;

    /** 账号唯一标识。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    FString AccountId;

    /** 账号身份是否有效。 */
    bool IsValid() const
    {
        return Environment != EYcAccountEnvironment::Unknown && !AccountId.IsEmpty();
    }

    /** 生成可作为持久化主键的账号 OwnerId。 */
    FString ToOwnerId() const
    {
        return FString::Printf(TEXT("%d::%s"), static_cast<int32>(Environment), *AccountId);
    }

    /** 生成调试字符串。 */
    FString ToDebugString() const
    {
        return ToOwnerId();
    }

    /** 账号身份相等比较。 */
    bool operator==(const FYcAccountIdentity& Other) const
    {
        return Environment == Other.Environment
            && AccountId == Other.AccountId;
    }
};

/**
 * 角色身份。
 * 一个账号下可以拥有多个角色档案，角色身份用于区分每个独立的玩法进度与资产归属。
 */
USTRUCT(BlueprintType)
struct YICHENACCOUNTCORE_API FYcProfileIdentity
{
    GENERATED_BODY()

    /** 角色所属账号。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    FYcAccountIdentity AccountIdentity;

    /** 角色唯一标识。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    FString ProfileId;

    /** 角色展示名。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    FString DisplayName;

    /** 角色身份是否有效。 */
    bool IsValid() const
    {
        return AccountIdentity.IsValid() && !ProfileId.IsEmpty();
    }

    /** 生成可作为持久化主键的角色 OwnerId。 */
    FString ToOwnerId() const
    {
        return FString::Printf(TEXT("%s::%s"), *AccountIdentity.ToOwnerId(), *ProfileId);
    }

    /** 生成调试字符串。 */
    FString ToDebugString() const
    {
        return ToOwnerId();
    }

    /** 角色身份相等比较。 */
    bool operator==(const FYcProfileIdentity& Other) const
    {
        return AccountIdentity == Other.AccountIdentity
            && ProfileId == Other.ProfileId
            && DisplayName == Other.DisplayName;
    }
};

/**
 * 持久化归属键。
 * 它是账号系统与其他业务系统之间的桥梁，外部模块只需要关心“数据归谁”，
 * 不需要直接理解账号/角色结构本身。
 */
USTRUCT(BlueprintType)
struct YICHENACCOUNTCORE_API FYcPersistentOwnerKey
{
    GENERATED_BODY()

    /** 归属对象类型。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    EYcPersistentOwnerType OwnerType = EYcPersistentOwnerType::None;

    /** 与归属类型配套的稳定 OwnerId。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    FString OwnerId;

    /** 归属键是否有效。 */
    bool IsValid() const
    {
        return OwnerType != EYcPersistentOwnerType::None && !OwnerId.IsEmpty();
    }

    /** 生成调试字符串。 */
    FString ToDebugString() const
    {
        return FString::Printf(TEXT("%d::%s"), static_cast<int32>(OwnerType), *OwnerId);
    }

    /** 归属键相等比较。 */
    bool operator==(const FYcPersistentOwnerKey& Other) const
    {
        return OwnerType == Other.OwnerType
            && OwnerId == Other.OwnerId;
    }
};

/**
 * 玩家身份快照。
 * 汇总平台身份、账号身份和当前激活角色身份，
 * 供网络复制、UI 刷新和业务系统进行只读消费。
 */
USTRUCT(BlueprintType)
struct YICHENACCOUNTCORE_API FYcPlayerIdentitySnapshot
{
    GENERATED_BODY()

    /** 平台侧原始身份。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    FYcPlatformIdentity PlatformIdentity;

    /** 账号体系内的稳定账号身份。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    FYcAccountIdentity AccountIdentity;

    /** 当前激活的角色身份。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    FYcProfileIdentity ActiveProfileIdentity;

    /** 当前快照是否来自服务端权威结果。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    bool bIsAuthoritative = false;

    /** 是否已完成账号认证。 */
    bool IsAuthenticated() const
    {
        return AccountIdentity.IsValid();
    }

    /** 是否已选择有效角色。 */
    bool HasActiveProfile() const
    {
        return ActiveProfileIdentity.IsValid();
    }

    /** 是否已达到“账号 + 角色”双完整的可玩状态。 */
    bool IsReady() const
    {
        return IsAuthenticated() && HasActiveProfile();
    }

    /** 获取当前玩家最适合作为数据归属的 OwnerId。 */
    FString GetPersistentOwnerId() const
    {
        if (ActiveProfileIdentity.IsValid())
        {
            return ActiveProfileIdentity.ToOwnerId();
        }
        return AccountIdentity.ToOwnerId();
    }

    /** 获取当前玩家最适合作为数据归属的 OwnerKey。 */
    FYcPersistentOwnerKey GetPersistentOwnerKey() const
    {
        FYcPersistentOwnerKey OwnerKey;
        if (ActiveProfileIdentity.IsValid())
        {
            OwnerKey.OwnerType = EYcPersistentOwnerType::Profile;
            OwnerKey.OwnerId = ActiveProfileIdentity.ToOwnerId();
            return OwnerKey;
        }

        if (AccountIdentity.IsValid())
        {
            OwnerKey.OwnerType = EYcPersistentOwnerType::Account;
            OwnerKey.OwnerId = AccountIdentity.ToOwnerId();
        }
        return OwnerKey;
    }

    /** 生成调试字符串。 */
    FString ToDebugString() const
    {
        return FString::Printf(TEXT("platform=%s account=%s profile=%s auth=%d"),
            *PlatformIdentity.ToDebugString(),
            *AccountIdentity.ToDebugString(),
            *ActiveProfileIdentity.ToDebugString(),
            bIsAuthoritative ? 1 : 0);
    }

    /** 玩家身份快照相等比较。 */
    bool operator==(const FYcPlayerIdentitySnapshot& Other) const
    {
        return PlatformIdentity == Other.PlatformIdentity
            && AccountIdentity == Other.AccountIdentity
            && ActiveProfileIdentity == Other.ActiveProfileIdentity
            && bIsAuthoritative == Other.bIsAuthoritative;
    }
};

/**
 * 会话快照。
 * 这是账号模块对外暴露的核心状态载体，包含状态机、玩家身份、可选角色列表和错误信息。
 */
USTRUCT(BlueprintType)
struct YICHENACCOUNTCORE_API FYcSessionSnapshot
{
    GENERATED_BODY()

    /** 当前会话状态。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    EYcAccountSessionState State = EYcAccountSessionState::Uninitialized;

    /** 当前玩家身份。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    FYcPlayerIdentitySnapshot PlayerIdentity;

    /** 当前账号下已知的角色列表。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    TArray<FYcProfileIdentity> AvailableProfiles;

    /** 最近一次失败的错误信息。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    FString ErrorMessage;

    /** 当前会话是否已经通过账号认证。 */
    bool IsAuthenticated() const
    {
        return PlayerIdentity.IsAuthenticated();
    }

    /** 当前会话是否已经拥有激活角色。 */
    bool HasActiveProfile() const
    {
        return PlayerIdentity.HasActiveProfile();
    }

    /** 当前会话是否已经可直接进入主要玩法。 */
    bool IsReady() const
    {
        return State == EYcAccountSessionState::Ready && PlayerIdentity.IsReady();
    }
};

/**
 * 登录请求参数。
 * 用于向适配器传递本次登录期望的用户提示信息与默认角色选择策略。
 */
USTRUCT(BlueprintType)
struct YICHENACCOUNTCORE_API FYcAuthRequest
{
    GENERATED_BODY()

    /** 平台用户标识提示，可用于覆盖默认的平台用户推导逻辑。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    FString PlatformUserIdHint;

    /** 展示名提示。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    FString DisplayNameHint;

    /** 希望默认激活的角色 Id。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    FString RequestedProfileId = TEXT("Main");

    /** 当目标角色不存在时，是否允许自动创建。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Account")
    bool bCreateProfileIfMissing = true;
};

/** 平台身份哈希，用于作为 TSet/TMap 键。 */
FORCEINLINE uint32 GetTypeHash(const FYcPlatformIdentity& Identity)
{
    uint32 Hash = GetTypeHash(static_cast<uint8>(Identity.Source));
    Hash = HashCombine(Hash, GetTypeHash(Identity.PlatformUserId));
    Hash = HashCombine(Hash, GetTypeHash(Identity.DisplayName));
    return Hash;
}

/** 账号身份哈希，用于作为 TSet/TMap 键。 */
FORCEINLINE uint32 GetTypeHash(const FYcAccountIdentity& Identity)
{
    uint32 Hash = GetTypeHash(static_cast<uint8>(Identity.Environment));
    Hash = HashCombine(Hash, GetTypeHash(Identity.AccountId));
    return Hash;
}

/** 角色身份哈希，用于作为 TSet/TMap 键。 */
FORCEINLINE uint32 GetTypeHash(const FYcProfileIdentity& Identity)
{
    uint32 Hash = GetTypeHash(Identity.AccountIdentity);
    Hash = HashCombine(Hash, GetTypeHash(Identity.ProfileId));
    return Hash;
}

/** 持久化归属键哈希，用于作为 TSet/TMap 键。 */
FORCEINLINE uint32 GetTypeHash(const FYcPersistentOwnerKey& OwnerKey)
{
    uint32 Hash = GetTypeHash(static_cast<uint8>(OwnerKey.OwnerType));
    Hash = HashCombine(Hash, GetTypeHash(OwnerKey.OwnerId));
    return Hash;
}
