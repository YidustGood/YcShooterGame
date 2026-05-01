// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "System/YcAccountTypes.h"
#include "YcAccountAdapter.generated.h"

class APlayerController;

/** 本地账号登录或缓存恢复完成后的统一回调。 */
DECLARE_DELEGATE_ThreeParams(FYcOnAccountAuthenticationCompleted, bool, const FYcSessionSnapshot&, const FString&);
/** 查询当前账号下可用角色列表后的统一回调。 */
DECLARE_DELEGATE_ThreeParams(FYcOnAccountProfileQueryCompleted, bool, const TArray<FYcProfileIdentity>&, const FString&);
/** 创建角色或切换激活角色完成后的统一回调。 */
DECLARE_DELEGATE_ThreeParams(FYcOnAccountProfileActivationCompleted, bool, const FYcSessionSnapshot&, const FString&);
/** 退出登录完成后的统一回调。 */
DECLARE_DELEGATE_TwoParams(FYcOnAccountLogoutCompleted, bool, const FString&);

/**
 * 账号适配器抽象基类。
 * 会话子系统只依赖这一层接口，不直接关心账号来源是本地离线、平台服务还是后端账号中心，
 * 从而让 YiChenGameCore 可以在不同项目里复用同一套会话流程。
 */
UCLASS(Abstract, BlueprintType)
class YICHENACCOUNTCORE_API UYcAccountAdapter : public UObject
{
    GENERATED_BODY()

public:
    /** 尝试从本地缓存恢复最近一次成功登录的会话。 */
    virtual void RestoreLocalSession(const UObject* WorldContextObject, const FYcOnAccountAuthenticationCompleted& Completion) PURE_VIRTUAL(UYcAccountAdapter::RestoreLocalSession, );

    /** 在本地上下文发起登录，常用于单机或无服务环境。 */
    virtual void AuthenticateLocalPlayer(const UObject* WorldContextObject, const FYcAuthRequest& Request, const FYcOnAccountAuthenticationCompleted& Completion) PURE_VIRTUAL(UYcAccountAdapter::AuthenticateLocalPlayer, );

    /** 在服务端基于 PlayerController 确认玩家身份并生成权威会话快照。 */
    virtual void AuthenticatePlayerControllerOnServer(const APlayerController* PlayerController, const FYcAuthRequest& Request, const FYcOnAccountAuthenticationCompleted& Completion) PURE_VIRTUAL(UYcAccountAdapter::AuthenticatePlayerControllerOnServer, );

    /** 查询当前账号下已有的角色/档案列表。 */
    virtual void QueryAvailableProfiles(const UObject* WorldContextObject, const FYcSessionSnapshot& CurrentSession, const FYcOnAccountProfileQueryCompleted& Completion) PURE_VIRTUAL(UYcAccountAdapter::QueryAvailableProfiles, );

    /** 创建新角色，并根据参数决定是否立即切换为当前激活角色。 */
    virtual void CreateProfile(const UObject* WorldContextObject, const FYcSessionSnapshot& CurrentSession, const FString& RequestedProfileId, const FString& DisplayName, bool bActivateNewProfile, const FYcOnAccountProfileActivationCompleted& Completion) PURE_VIRTUAL(UYcAccountAdapter::CreateProfile, );

    /** 激活指定角色，必要时可按策略自动补建角色。 */
    virtual void ActivateProfile(const UObject* WorldContextObject, const FYcSessionSnapshot& CurrentSession, const FString& RequestedProfileId, bool bCreateProfileIfMissing, const FYcOnAccountProfileActivationCompleted& Completion) PURE_VIRTUAL(UYcAccountAdapter::ActivateProfile, );

    /** 清理当前账号会话并执行退出登录流程。 */
    virtual void Logout(const UObject* WorldContextObject, const FYcSessionSnapshot& CurrentSession, const FYcOnAccountLogoutCompleted& Completion) PURE_VIRTUAL(UYcAccountAdapter::Logout, );
};
