// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "System/YcAccountAdapter.h"
#include "YcLocalAccountAdapter.generated.h"

class UWorld;

/**
 * 本地离线账号适配器。
 * 这是 YiChenGameCore 默认提供的账号实现，适用于单机、PIE 和无后端联调阶段，
 * 通过 SaveGame 持久化会话与角色列表，保证账号流程在基础项目中即可直接跑通。
 */
UCLASS(Config = Game)
class YICHENACCOUNTCORE_API UYcLocalAccountAdapter : public UYcAccountAdapter
{
    GENERATED_BODY()

public:
    /** 从本地 SaveGame 中恢复历史会话。 */
    virtual void RestoreLocalSession(const UObject* WorldContextObject, const FYcOnAccountAuthenticationCompleted& Completion) override;
    /** 在本地构造离线账号并立即建立会话。 */
    virtual void AuthenticateLocalPlayer(const UObject* WorldContextObject, const FYcAuthRequest& Request, const FYcOnAccountAuthenticationCompleted& Completion) override;
    /** 服务端基于 PlayerController 生成权威离线会话。 */
    virtual void AuthenticatePlayerControllerOnServer(const APlayerController* PlayerController, const FYcAuthRequest& Request, const FYcOnAccountAuthenticationCompleted& Completion) override;
    /** 查询当前本地账号下保存的角色列表。 */
    virtual void QueryAvailableProfiles(const UObject* WorldContextObject, const FYcSessionSnapshot& CurrentSession, const FYcOnAccountProfileQueryCompleted& Completion) override;
    /** 创建新的本地角色档案。 */
    virtual void CreateProfile(const UObject* WorldContextObject, const FYcSessionSnapshot& CurrentSession, const FString& RequestedProfileId, const FString& DisplayName, bool bActivateNewProfile, const FYcOnAccountProfileActivationCompleted& Completion) override;
    /** 激活已有角色，或按策略补建角色。 */
    virtual void ActivateProfile(const UObject* WorldContextObject, const FYcSessionSnapshot& CurrentSession, const FString& RequestedProfileId, bool bCreateProfileIfMissing, const FYcOnAccountProfileActivationCompleted& Completion) override;
    /** 清理本地缓存的账号会话。 */
    virtual void Logout(const UObject* WorldContextObject, const FYcSessionSnapshot& CurrentSession, const FYcOnAccountLogoutCompleted& Completion) override;

private:
    /** 根据环境与平台用户 Id 生成稳定的本地存档槽名。 */
    static FString BuildSessionSlotName(const UWorld* World, const FString& PlatformUserId);
    /** 将当前会话快照写入本地 SaveGame。 */
    static bool PersistSessionSnapshot(const UWorld* World, const FYcSessionSnapshot& SessionSnapshot);
    /** 从本地 SaveGame 读取历史会话快照。 */
    static bool LoadPersistedSessionSnapshot(const UWorld* World, const FString& PlatformUserId, FYcSessionSnapshot& OutSessionSnapshot);
    /** 删除指定平台用户的本地会话缓存。 */
    static void DeletePersistedSessionSnapshot(const UWorld* World, const FString& PlatformUserId);
    /** 基于当前环境、平台身份和默认角色构造一份新的离线会话。 */
    static FYcSessionSnapshot BuildSessionSnapshot(const UWorld* World, const FString& PlatformUserId, const FString& DisplayName, const FString& RequestedProfileId);
};
