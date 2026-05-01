// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "System/YcSaveBackendProvider.h"
#include "System/YcSaveCoreTypes.h"
#include "YcProfileSaveSubsystem.generated.h"

class UYcSaveBackendProvider;
class UYcSaveDomainProvider;

DECLARE_DELEGATE_TwoParams(FYcOnProfileLoadCompleted, bool /*bSuccess*/, const FString& /*Reason*/);
DECLARE_DELEGATE_TwoParams(FYcOnProfileSaveCompleted, bool /*bSuccess*/, const FString& /*Reason*/);

UENUM(BlueprintType)
enum class EYcSaveUnknownDomainPolicy : uint8
{
    /** 严格模式：遇到未知 DomainKey 立即失败。 */
    Strict UMETA(DisplayName="Strict"),
    /** 宽松模式：跳过未知 DomainKey 并记录告警。 */
    Lenient UMETA(DisplayName="Lenient")
};

/** Profile 聚合存档子系统（GameInstance 级）。 */
UCLASS(Config=Game)
class YICHENSAVECORE_API UYcProfileSaveSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** 初始化：准备后端实例。 */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    /** 反初始化：保存脏档并清理缓存。 */
    virtual void Deinitialize() override;

    /** 通过 WorldContext 获取子系统实例。 */
    static UYcProfileSaveSubsystem* Get(const UObject* WorldContextObject);

    /** 注册 Profile 对应的运行时上下文。 */
    void RegisterProfileContext(const FYcProfileIdentity& ProfileIdentity, UObject* ContextObject);
    /** 注销 Profile 对应的运行时上下文。 */
    void UnregisterProfileContext(const FYcProfileIdentity& ProfileIdentity, UObject* ContextObject);

    /** 异步加载指定 Profile。 */
    void LoadProfileAsync(const FYcProfileIdentity& ProfileIdentity, const FYcOnProfileLoadCompleted& Completion);
    /** 异步保存指定 Profile。 */
    void SaveProfileAsync(const FYcProfileIdentity& ProfileIdentity, const FYcOnProfileSaveCompleted& Completion);
    /** 异步保存全部脏 Profile。 */
    void SaveDirtyProfilesAsync(const FYcOnProfileSaveCompleted& Completion);
    /** 异步读取根对象。 */
    void LoadProfileRootAsync(const FYcProfileIdentity& ProfileIdentity, const FYcOnLoadProfileRoot& Completion);
    /** 异步保存根对象。 */
    void SaveProfileRootAsync(const FYcProfileIdentity& ProfileIdentity, const FYcProfileSaveRoot& Root, const FYcOnSaveProfileRoot& Completion);

    /** 同步加载指定 Profile。 */
    bool LoadProfileSync(const FYcProfileIdentity& ProfileIdentity, FString& OutReason);
    /** 同步保存指定 Profile。 */
    bool SaveProfileSync(const FYcProfileIdentity& ProfileIdentity, FString& OutReason);
    /** 同步保存全部脏 Profile。 */
    bool SaveDirtyProfilesSync(FString& OutReason);
    /** 仅调用后端读取 Profile 根对象。 */
    EYcSaveBackendResult LoadProfileRootSync(const FYcProfileIdentity& ProfileIdentity, FYcProfileSaveRoot& OutRoot, FString& OutReason);
    /** 仅调用后端保存 Profile 根对象。 */
    bool SaveProfileRootSync(const FYcProfileIdentity& ProfileIdentity, const FYcProfileSaveRoot& Root, FString& OutReason);

    /** 标记 Profile 为脏。 */
    void MarkProfileDirty(const FYcProfileIdentity& ProfileIdentity);
    /** 清理 Profile 脏标。 */
    void ClearProfileDirty(const FYcProfileIdentity& ProfileIdentity);
    /** 判断 Profile 是否为脏。 */
    bool IsProfileDirty(const FYcProfileIdentity& ProfileIdentity) const;

private:
    bool BuildProfileKey(const FYcProfileIdentity& ProfileIdentity, FYcProfileSaveKey& OutProfileKey, FString& OutReason) const;
    void RegisterProfileContextByKey(const FYcProfileSaveKey& ProfileKey, UObject* ContextObject);
    void UnregisterProfileContextByKey(const FYcProfileSaveKey& ProfileKey, UObject* ContextObject);
    void LoadProfileAsyncByKey(const FYcProfileSaveKey& ProfileKey, const FYcOnProfileLoadCompleted& Completion);
    void SaveProfileAsyncByKey(const FYcProfileSaveKey& ProfileKey, const FYcOnProfileSaveCompleted& Completion);
    void SaveDirtyProfilesAsyncByKeyArray(const TArray<FYcProfileSaveKey>& ProfileKeys, const FYcOnProfileSaveCompleted& Completion);
    void LoadProfileRootAsyncByKey(const FYcProfileSaveKey& ProfileKey, const FYcOnLoadProfileRoot& Completion);
    void SaveProfileRootAsyncByKey(const FYcProfileSaveKey& ProfileKey, const FYcProfileSaveRoot& Root, const FYcOnSaveProfileRoot& Completion);
    bool LoadProfileSyncByKey(const FYcProfileSaveKey& ProfileKey, FString& OutReason);
    bool SaveProfileSyncByKey(const FYcProfileSaveKey& ProfileKey, FString& OutReason);
    EYcSaveBackendResult LoadProfileRootSyncByKey(const FYcProfileSaveKey& ProfileKey, FYcProfileSaveRoot& OutRoot, FString& OutReason);
    bool SaveProfileRootSyncByKey(const FYcProfileSaveKey& ProfileKey, const FYcProfileSaveRoot& Root, FString& OutReason);
    void MarkProfileDirtyByKey(const FYcProfileSaveKey& ProfileKey);
    void ClearProfileDirtyByKey(const FYcProfileSaveKey& ProfileKey);
    bool IsProfileDirtyByKey(const FYcProfileSaveKey& ProfileKey) const;
    /** 从缓存中解析 Profile 对应的 Context。 */
    UObject* ResolveContext(const FYcProfileSaveKey& ProfileKey) const;
    /** 确保后端实例已创建。 */
    void EnsureBackendProvider();
    /** 从 Context 构建 Profile 根对象。 */
    bool BuildRootFromContext(const FYcProfileSaveKey& ProfileKey, UObject* ContextObject, FYcProfileSaveRoot& OutRoot, FString& OutReason) const;
    /** 将 Profile 根对象应用到 Context。 */
    bool ApplyRootToContext(const FYcProfileSaveKey& ProfileKey, UObject* ContextObject, const FYcProfileSaveRoot& Root, FString& OutReason) const;
    /** 收集所有已注册 Domain Provider CDO。 */
    void GatherDomainProviders(TArray<const UYcSaveDomainProvider*>& OutProviders) const;

private:
    /** 当前后端实例。 */
    UPROPERTY(Transient)
    TObjectPtr<UYcSaveBackendProvider> BackendProvider = nullptr;

    /** ProfileKey -> Context 映射。 */
    UPROPERTY(Transient)
    TMap<FYcProfileSaveKey, TObjectPtr<UObject>> ContextByProfile;

    /** 脏 Profile 集合。 */
    UPROPERTY(Transient)
    TSet<FYcProfileSaveKey> DirtyProfiles;

    /** 后端类配置（可切换本地/远端）。 */
    UPROPERTY(Config)
    TSoftClassPtr<UYcSaveBackendProvider> BackendProviderClass;

    /** 未知域处理策略。 */
    UPROPERTY(Config, EditAnywhere, Category="Save")
    EYcSaveUnknownDomainPolicy UnknownDomainPolicy = EYcSaveUnknownDomainPolicy::Lenient;
};
