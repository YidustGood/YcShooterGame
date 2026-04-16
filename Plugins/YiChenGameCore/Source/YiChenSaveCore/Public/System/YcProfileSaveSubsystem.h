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
    void RegisterProfileContext(const FYcProfileKey& ProfileKey, UObject* ContextObject);
    /** 注销 Profile 对应的运行时上下文。 */
    void UnregisterProfileContext(const FYcProfileKey& ProfileKey, UObject* ContextObject);

    /** 异步加载指定 Profile。 */
    void LoadProfileAsync(const FYcProfileKey& ProfileKey, const FYcOnProfileLoadCompleted& Completion);
    /** 异步保存指定 Profile。 */
    void SaveProfileAsync(const FYcProfileKey& ProfileKey, const FYcOnProfileSaveCompleted& Completion);
    /** 异步保存全部脏 Profile。 */
    void SaveDirtyProfilesAsync(const FYcOnProfileSaveCompleted& Completion);

    /** 同步加载指定 Profile。 */
    bool LoadProfileSync(const FYcProfileKey& ProfileKey, FString& OutReason);
    /** 同步保存指定 Profile。 */
    bool SaveProfileSync(const FYcProfileKey& ProfileKey, FString& OutReason);
    /** 同步保存全部脏 Profile。 */
    bool SaveDirtyProfilesSync(FString& OutReason);
    /** 仅调用后端读取 Profile 根对象。 */
    EYcSaveBackendResult LoadProfileRootSync(const FYcProfileKey& ProfileKey, FYcProfileSaveRoot& OutRoot, FString& OutReason);
    /** 仅调用后端保存 Profile 根对象。 */
    bool SaveProfileRootSync(const FYcProfileKey& ProfileKey, const FYcProfileSaveRoot& Root, FString& OutReason);

    /** 标记 Profile 为脏。 */
    void MarkProfileDirty(const FYcProfileKey& ProfileKey);
    /** 清理 Profile 脏标。 */
    void ClearProfileDirty(const FYcProfileKey& ProfileKey);
    /** 判断 Profile 是否为脏。 */
    bool IsProfileDirty(const FYcProfileKey& ProfileKey) const;

private:
    /** 从缓存中解析 Profile 对应的 Context。 */
    UObject* ResolveContext(const FYcProfileKey& ProfileKey) const;
    /** 确保后端实例已创建。 */
    void EnsureBackendProvider();
    /** 从 Context 构建 Profile 根对象。 */
    bool BuildRootFromContext(const FYcProfileKey& ProfileKey, UObject* ContextObject, FYcProfileSaveRoot& OutRoot, FString& OutReason) const;
    /** 将 Profile 根对象应用到 Context。 */
    bool ApplyRootToContext(const FYcProfileKey& ProfileKey, UObject* ContextObject, const FYcProfileSaveRoot& Root, FString& OutReason) const;
    /** 收集所有已注册 Domain Provider CDO。 */
    void GatherDomainProviders(TArray<const UYcSaveDomainProvider*>& OutProviders) const;

private:
    /** 当前后端实例。 */
    UPROPERTY(Transient)
    TObjectPtr<UYcSaveBackendProvider> BackendProvider = nullptr;

    /** ProfileKey -> Context 映射。 */
    UPROPERTY(Transient)
    TMap<FYcProfileKey, TObjectPtr<UObject>> ContextByProfile;

    /** 脏 Profile 集合。 */
    UPROPERTY(Transient)
    TSet<FYcProfileKey> DirtyProfiles;

    /** 后端类配置（可切换本地/远端）。 */
    UPROPERTY(Config)
    TSoftClassPtr<UYcSaveBackendProvider> BackendProviderClass;

    /** 未知域处理策略。 */
    UPROPERTY(Config, EditAnywhere, Category="Save")
    EYcSaveUnknownDomainPolicy UnknownDomainPolicy = EYcSaveUnknownDomainPolicy::Lenient;
};
