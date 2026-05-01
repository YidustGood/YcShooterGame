// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "System/YcAccountTypes.h"
#include "YcAccountIdentityLibrary.generated.h"

/**
 * 账号身份蓝图库工具。
 * 用于把 C++ 中的身份校验、OwnerKey 组装和调试字符串能力暴露给蓝图，
 * 方便 UI、流程图和其他系统统一处理账号/角色身份。
 */
UCLASS()
class YICHENACCOUNTCORE_API UYcAccountIdentityLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** 判断平台侧身份是否有效，例如本地离线账号或平台账号是否具备稳定用户标识。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static bool IsPlatformIdentityValid(const FYcPlatformIdentity& PlatformIdentity);

    /** 判断账号身份是否有效。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static bool IsAccountIdentityValid(const FYcAccountIdentity& AccountIdentity);

    /** 判断角色身份是否有效。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static bool IsProfileIdentityValid(const FYcProfileIdentity& ProfileIdentity);

    /** 判断玩家是否已经完成账号认证。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static bool IsPlayerIdentityAuthenticated(const FYcPlayerIdentitySnapshot& PlayerIdentity);

    /** 判断玩家当前是否已选择激活角色。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static bool HasActiveProfileIdentity(const FYcPlayerIdentitySnapshot& PlayerIdentity);

    /** 判断玩家身份是否达到“可进入核心玩法”的就绪状态。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static bool IsPlayerIdentityReady(const FYcPlayerIdentitySnapshot& PlayerIdentity);

    /** 判断持久化归属键是否有效。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static bool IsPersistentOwnerKeyValid(const FYcPersistentOwnerKey& PersistentOwnerKey);

    /** 构造账号身份。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static FYcAccountIdentity MakeAccountIdentity(EYcAccountEnvironment Environment, const FString& AccountId);

    /** 构造角色身份。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static FYcProfileIdentity MakeProfileIdentity(EYcAccountEnvironment Environment, const FString& AccountId, const FString& ProfileId, const FString& DisplayName);

    /** 读取玩家当前激活的角色身份。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static FYcProfileIdentity GetActiveProfileIdentity(const FYcPlayerIdentitySnapshot& PlayerIdentity);

    /** 构造通用持久化归属键。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static FYcPersistentOwnerKey MakePersistentOwnerKey(EYcPersistentOwnerType OwnerType, const FString& OwnerId);

    /** 将账号身份转换为账号级持久化归属键。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static FYcPersistentOwnerKey MakeAccountOwnerKey(const FYcAccountIdentity& AccountIdentity);

    /** 将角色身份转换为角色级持久化归属键。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static FYcPersistentOwnerKey MakeProfileOwnerKey(const FYcProfileIdentity& ProfileIdentity);

    /** 根据玩家当前状态推导最合适的持久化归属键。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static FYcPersistentOwnerKey GetPlayerPersistentOwnerKey(const FYcPlayerIdentitySnapshot& PlayerIdentity);

    /** 获取账号身份对应的稳定 OwnerId。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static FString GetAccountOwnerId(const FYcAccountIdentity& AccountIdentity);

    /** 获取角色身份对应的稳定 OwnerId。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static FString GetProfileOwnerId(const FYcProfileIdentity& ProfileIdentity);

    /** 获取玩家当前最合适的持久化 OwnerId。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static FString GetPlayerPersistentOwnerId(const FYcPlayerIdentitySnapshot& PlayerIdentity);

    /** 获取持久化归属键内部保存的 OwnerId。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static FString GetPersistentOwnerKeyId(const FYcPersistentOwnerKey& PersistentOwnerKey);

    /** 生成平台身份的调试描述。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static FString DescribePlatformIdentity(const FYcPlatformIdentity& PlatformIdentity);

    /** 生成账号身份的调试描述。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static FString DescribeAccountIdentity(const FYcAccountIdentity& AccountIdentity);

    /** 生成角色身份的调试描述。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static FString DescribeProfileIdentity(const FYcProfileIdentity& ProfileIdentity);

    /** 生成玩家身份快照的调试描述。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static FString DescribePlayerIdentity(const FYcPlayerIdentitySnapshot& PlayerIdentity);

    /** 生成持久化归属键的调试描述。 */
    UFUNCTION(BlueprintPure, Category = "Account|Identity")
    static FString DescribePersistentOwnerKey(const FYcPersistentOwnerKey& PersistentOwnerKey);
};
