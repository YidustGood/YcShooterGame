// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "System/YcAccountTypes.h"
#include "YcAccountSessionSubsystem.generated.h"

class APlayerController;
class UYcAccountAdapter;

/** 会话快照发生变化时广播，供 UI 和业务流程同步刷新。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FYcOnAccountSessionChanged, FYcSessionSnapshot, SessionSnapshot);
/** 玩家身份变化时广播，便于角色数据、背包等系统重新绑定归属。 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FYcOnPlayerIdentityChanged, FYcPlayerIdentitySnapshot, PlayerIdentity);

/**
 * 账号会话子系统。
 * 这是项目侧访问账号能力的主入口，负责驱动登录、恢复会话、查询角色、切换角色与登出流程，
 * 并把适配器返回的结果整理成统一的 SessionSnapshot 暴露给蓝图和其他运行时系统。
 */
UCLASS(Config = Game)
class YICHENACCOUNTCORE_API UYcAccountSessionSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** 初始化默认会话状态并准备账号适配器。 */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    /** 子系统销毁时释放适配器与缓存快照。 */
    virtual void Deinitialize() override;

    /** 通过世界上下文获取当前 GameInstance 上的账号会话子系统。 */
    static UYcAccountSessionSubsystem* Get(const UObject* WorldContextObject = nullptr);

    /** 获取完整会话快照。 */
    UFUNCTION(BlueprintPure, Category = "Account")
    FYcSessionSnapshot GetCurrentSession() const { return CurrentSession; }

    /** 获取当前玩家身份快照。 */
    UFUNCTION(BlueprintPure, Category = "Account")
    FYcPlayerIdentitySnapshot GetCurrentPlayerIdentity() const { return CurrentSession.PlayerIdentity; }

    /** 获取当前会话状态机状态。 */
    UFUNCTION(BlueprintPure, Category = "Account")
    EYcAccountSessionState GetCurrentSessionState() const { return CurrentSession.State; }

    /** 获取当前账号下可见的角色列表。 */
    UFUNCTION(BlueprintPure, Category = "Account")
    TArray<FYcProfileIdentity> GetAvailableProfiles() const { return CurrentSession.AvailableProfiles; }

    /** 获取当前激活角色。 */
    UFUNCTION(BlueprintPure, Category = "Account")
    FYcProfileIdentity GetActiveProfileIdentity() const { return CurrentSession.PlayerIdentity.ActiveProfileIdentity; }

    /** 判断是否已完成账号认证。 */
    UFUNCTION(BlueprintPure, Category = "Account")
    bool HasAuthenticatedSession() const { return CurrentSession.PlayerIdentity.IsAuthenticated(); }

    /** 判断是否已进入带激活角色的可玩状态。 */
    UFUNCTION(BlueprintPure, Category = "Account")
    bool HasActiveProfile() const { return CurrentSession.PlayerIdentity.HasActiveProfile(); }

    /** 尝试恢复本地缓存的历史会话。 */
    UFUNCTION(BlueprintCallable, Category = "Account")
    bool RestoreLocalSession(APlayerController* PlayerController);

    /** 发起本地登录流程。 */
    UFUNCTION(BlueprintCallable, Category = "Account")
    bool BeginLocalLogin(APlayerController* PlayerController, const FYcAuthRequest& Request);

    /** 刷新当前账号下可用角色列表。 */
    UFUNCTION(BlueprintCallable, Category = "Account")
    bool RefreshAvailableProfiles(APlayerController* PlayerController);

    /** 创建新角色。 */
    UFUNCTION(BlueprintCallable, Category = "Account")
    bool CreateProfile(APlayerController* PlayerController, const FString& RequestedProfileId, const FString& DisplayName, bool bActivateNewProfile = true);

    /** 切换当前激活角色。 */
    UFUNCTION(BlueprintCallable, Category = "Account")
    bool SwitchActiveProfile(APlayerController* PlayerController, const FString& RequestedProfileId, bool bCreateProfileIfMissing = true);

    /** 退出当前账号会话。 */
    UFUNCTION(BlueprintCallable, Category = "Account")
    bool SignOut(APlayerController* PlayerController);

    /** 服务端执行身份认证并返回权威会话快照。 */
    bool AuthenticatePlayerControllerOnServer(APlayerController* PlayerController, const FYcAuthRequest& Request, FYcSessionSnapshot& OutSessionSnapshot);
    /** 服务端恢复本地缓存会话。 */
    bool RestoreLocalSessionOnServer(APlayerController* PlayerController, FYcSessionSnapshot& OutSessionSnapshot);
    /** 服务端查询角色列表。 */
    bool RefreshAvailableProfilesOnServer(APlayerController* PlayerController, TArray<FYcProfileIdentity>& OutProfiles);
    /** 服务端创建角色。 */
    bool CreateProfileOnServer(APlayerController* PlayerController, const FString& RequestedProfileId, const FString& DisplayName, bool bActivateNewProfile, FYcSessionSnapshot& OutSessionSnapshot);
    /** 服务端切换激活角色。 */
    bool SwitchActiveProfileOnServer(APlayerController* PlayerController, const FString& RequestedProfileId, bool bCreateProfileIfMissing, FYcSessionSnapshot& OutSessionSnapshot);
    /** 服务端退出登录。 */
    bool SignOutPlayerOnServer(APlayerController* PlayerController);
    /** 接收网络复制来的玩家身份，并更新本地会话可见状态。 */
    void AdoptReplicatedPlayerIdentity(const FYcPlayerIdentitySnapshot& PlayerIdentitySnapshot);
    /** 使当前会话失效并记录错误信息。 */
    void InvalidateCurrentSession(const FString& ErrorMessage, EYcAccountSessionState ErrorState = EYcAccountSessionState::Error);

public:
    /** 会话快照变化事件。 */
    UPROPERTY(BlueprintAssignable, Category = "Account")
    FYcOnAccountSessionChanged OnAccountSessionChanged;

    /** 玩家身份变化事件。 */
    UPROPERTY(BlueprintAssignable, Category = "Account")
    FYcOnPlayerIdentityChanged OnPlayerIdentityChanged;

private:
    /** 按配置创建账号适配器，没有配置时回退到本地离线适配器。 */
    void EnsureAdapter();
    /** 广播会话与身份变化。 */
    void BroadcastSessionChanged();
    /** 仅更新状态机状态，并保留当前快照中的其他数据。 */
    void SetSessionState(EYcAccountSessionState NewState, const FString& ErrorMessage = FString());
    /** 应用适配器返回的会话结果，并按身份完整度修正最终状态。 */
    void ApplyResolvedSession(const FYcSessionSnapshot& SessionSnapshot, bool bBroadcastIdentity);

private:
    /** 账号适配器类型，可在项目配置中替换为接平台或后端的实现。 */
    UPROPERTY(Config)
    TSoftClassPtr<UYcAccountAdapter> AdapterClass;

    /** 当前实际使用的账号适配器实例。 */
    UPROPERTY(Transient)
    TObjectPtr<UYcAccountAdapter> Adapter = nullptr;

    /** 当前会话快照，是 UI 与运行时逻辑读取账号状态的统一来源。 */
    UPROPERTY(Transient)
    FYcSessionSnapshot CurrentSession;
};
