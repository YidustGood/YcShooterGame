// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "System/YcQuestResolverInterfaces.h"
#include "System/YcSaveCoreTypes.h"
#include "Tickable.h"
#include "YcQuestTypes.h"
#include "YcQuestSubsystem.generated.h"

class AActor;
class UYcQuestAssetPolicy;
class UYcQuestDefinition;
class UYcQuestEffect;
class UYcQuestInstance;
class UYcQuestObjective;
class UWorld;
struct FStreamableHandle;

/**
 * 任务子系统。
 * 这是项目运行时访问任务系统的主入口，负责接取任务、维护任务实例、路由任务事件、
 * 管理目标树推进、处理共享成员、加载任务资源以及与存档系统对接。
 */
UCLASS(Config=Game)
class YICHENQUEST_API UYcQuestSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    /** 初始化任务子系统并注册世界清理监听。 */
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    /** 释放任务子系统运行时状态。 */
    virtual void Deinitialize() override;

    /** 通过世界上下文获取任务子系统。 */
    static UYcQuestSubsystem* Get(const UObject* WorldContextObject);

    /** 依据玩家身份接取任务，并自动解析实例归属。 */
    UFUNCTION(BlueprintCallable, Category = "Quest")
    bool ServerAcceptQuest(const FYcPlayerIdentitySnapshot& PlayerIdentity, FName QuestId, FYcQuestInstanceKey& OutInstanceKey);

    /** 直接为指定 Owner 接取任务。 */
    UFUNCTION(BlueprintCallable, Category = "Quest")
    bool ServerAcceptQuestForOwner(FName QuestId, EYcQuestOwnerType OwnerType, const FString& OwnerId, FYcQuestInstanceKey& OutInstanceKey);

    /** 根据持久化归属键接取任务。 */
    UFUNCTION(BlueprintCallable, Category = "Quest")
    bool ServerAcceptQuestForPersistentOwner(FName QuestId, const FYcPersistentOwnerKey& PersistentOwnerKey, FYcQuestInstanceKey& OutInstanceKey);

    /** 将指定任务实例标记为完成。 */
    UFUNCTION(BlueprintCallable, Category = "Quest")
    bool ServerCompleteQuestByInstance(const FYcQuestInstanceKey& InstanceKey, const FString& Detail = TEXT("Completed"));

    /** 将指定任务实例标记为失败。 */
    UFUNCTION(BlueprintCallable, Category = "Quest")
    bool ServerFailQuestByInstance(const FYcQuestInstanceKey& InstanceKey, const FString& Detail = TEXT("Failed"));

    /** 将指定任务实例标记为中止。 */
    UFUNCTION(BlueprintCallable, Category = "Quest")
    bool ServerAbortQuestByInstance(const FYcQuestInstanceKey& InstanceKey, const FString& Detail = TEXT("Aborted"));

    /** 按路由范围向任务系统广播一个任务事件。 */
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void ServerSubmitQuestEventRouted(const FYcQuestEvent& Event, EYcQuestEventRouteScope RouteScope = EYcQuestEventRouteScope::Both);

    /** 向指定任务实例投递任务事件。 */
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void ServerSubmitQuestEventToInstance(const FYcQuestInstanceKey& InstanceKey, const FYcQuestEvent& Event);

    /** 请求任务在指定阶段所需的 Bundle。 */
    UFUNCTION(BlueprintCallable, Category = "Quest|Assets")
    bool RequestQuestBundlesByInstance(const FYcQuestInstanceKey& InstanceKey, EYcQuestPhase Phase);

    /** 释放任务在指定阶段使用的 Bundle。 */
    UFUNCTION(BlueprintCallable, Category = "Quest|Assets")
    void ReleaseQuestBundlesByInstance(const FYcQuestInstanceKey& InstanceKey, EYcQuestPhase Phase);

    /** 判断某个任务实例的指定 Bundle 是否已经准备完成。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Assets")
    bool IsQuestBundleReadyByInstance(const FYcQuestInstanceKey& InstanceKey, FName BundleName) const;

    /** 获取指定任务实例的完整运行时快照。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Query")
    bool GetQuestRuntimeSnapshotByInstanceKey(const FYcQuestInstanceKey& InstanceKey, FYcQuestRuntimeSnapshot& OutSnapshot) const;

    /** 获取当前所有运行时任务快照。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Query")
    void GetRuntimeQuestSnapshots(TArray<FYcQuestRuntimeSnapshot>& OutSnapshots) const;

    /** 按 Key 获取运行时任务实例对象。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Query")
    bool GetQuestInstanceByKey(const FYcQuestInstanceKey& InstanceKey, UYcQuestInstance*& OutInstance) const;

    /** 获取指定任务实例的公开进度列表。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Query")
    bool GetQuestPublicProgressByInstance(const FYcQuestInstanceKey& InstanceKey, TArray<FYcQuestPublicProgress>& OutProgress) const;

    /** 运行时修改指定任务实例中某个 CounterObjective 的 TargetValue。 */
    UFUNCTION(BlueprintCallable, Category = "Quest")
    bool ServerSetCounterObjectiveTargetValueByInstance(const FYcQuestInstanceKey& InstanceKey, FName ObjectiveId, float NewTargetValue);

    /** 设置任务实例的复制扩展负载。 */
    UFUNCTION(BlueprintCallable, Category = "Quest|Network")
    bool ServerSetQuestReplicatedPayloadByInstance(const FYcQuestInstanceKey& InstanceKey, const FInstancedStruct& Payload);

    /** 获取任务实例的复制扩展负载。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Network")
    bool GetQuestReplicatedPayloadByInstance(const FYcQuestInstanceKey& InstanceKey, FInstancedStruct& OutPayload) const;

    /** 更新共享任务实例的成员列表。 */
    UFUNCTION(BlueprintCallable, Category = "Quest|Shared")
    bool ServerUpdateSharedQuestMembers(const FYcQuestInstanceKey& InstanceKey, const TArray<FString>& MemberIds);

    /** 判断某玩家是否属于该共享任务。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Shared")
    bool IsPlayerInSharedQuest(const FYcQuestInstanceKey& InstanceKey, const FString& PlayerId) const;

    /** 获取共享任务当前成员列表。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Shared")
    bool GetSharedQuestMembers(const FYcQuestInstanceKey& InstanceKey, TArray<FString>& OutMemberIds) const;

    /** 判断任务是否处于活跃状态。 */
    bool IsQuestActive(const FYcQuestInstanceKey& InstanceKey) const;
    /** 构建任务系统存档快照。 */
    bool BuildSaveSnapshot(FYcQuestSaveSnapshot& OutSnapshot) const;
    /** 应用任务系统存档快照。 */
    bool ApplySaveSnapshot(const FYcQuestSaveSnapshot& InSnapshot);

    /** 触发任务级或目标级效果。 */
    void DispatchQuestEffects(const UYcQuestInstance* Instance, EYcQuestEffectTrigger Trigger, FName ObjectiveId, const FYcQuestPublicProgress& Progress) const;

    /** 每帧更新任务目标树。 */
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override;

private:
    /** 世界清理时处理对局级任务与资源释放。 */
    void HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);
    /** 重置 MatchOnly 作用域的任务实例。 */
    void ResetMatchScopeQuestInstances();
    /** 判断状态是否为终态。 */
    bool IsTerminalState(EYcQuestState State) const;
    /** 校验当前调用是否处于服务端权限。 */
    bool HasServerAuthority(const TCHAR* ActionName) const;
    /** 判断任务状态迁移是否合法。 */
    bool IsValidStateTransition(EYcQuestState CurrentState, EYcQuestState NewState) const;
    /** 获取或创建任务资源策略对象。 */
    UYcQuestAssetPolicy* GetOrCreateAssetPolicy();
    /** 获取或创建任务 Owner 解析器。 */
    UObject* GetOrCreateOwnerResolver();
    /** 获取或创建共享任务解析器。 */
    UObject* GetOrCreateShareResolver();
    /** 根据 QuestId 解析任务定义资产。 */
    const UYcQuestDefinition* ResolveQuestDefinition(FName QuestId) const;
    /** 接取任务时解析实例 Key。 */
    bool ResolveAcceptInstanceKey(const UYcQuestDefinition* QuestDef, const FYcPlayerIdentitySnapshot& PlayerIdentity, FYcQuestInstanceKey& OutInstanceKey) const;
    /** 任务事件推进时解析实例 Key。 */
    bool ResolveEventInstanceKey(const UYcQuestDefinition* QuestDef, const FYcQuestEvent& Event, FYcQuestInstanceKey& OutInstanceKey) const;
    /** 判断事件是否为重复提交。 */
    bool IsDuplicateEvent(const FYcQuestEvent& Event);
    /** 清理事件去重缓存。 */
    void PruneEventDedupCache();

    /** 确保任务实例存在，必要时根据定义创建。 */
    bool EnsureQuestInstance(const FYcQuestInstanceKey& InstanceKey, const UYcQuestDefinition* QuestDef, UYcQuestInstance*& OutInstance);
    /** 销毁指定任务实例。 */
    void DestroyQuestInstance(const FYcQuestInstanceKey& InstanceKey);
    /** 驱动任务实例状态迁移。 */
    bool TransitionInstanceState(const FYcQuestInstanceKey& InstanceKey, EYcQuestState NewState, EYcQuestPhase Phase, const FString& Detail);
    /** 实际处理任务事件推进。 */
    void ProcessQuestEventInternal(const FYcQuestInstanceKey& InstanceKey, const FYcQuestEvent& Event);
    /** 通过共享解析器刷新共享任务成员。 */
    bool RefreshSharedQuestMembersFromResolver(const FYcQuestInstanceKey& InstanceKey);
    /** 解析某共享任务实例的成员列表。 */
    bool ResolveSharedMembersForInstance(const FYcQuestInstanceKey& InstanceKey, TArray<FString>& OutMemberIds) const;
    /** 从任意对象推导玩家身份。 */
    bool TryResolvePlayerIdentityFromObject(const UObject* SourceObject, FYcPlayerIdentitySnapshot& OutPlayerIdentity) const;
    /** 判断共享事件的触发者是否有资格推进该共享任务。 */
    bool IsSharedEventInstigatorAllowed(const FYcQuestInstanceKey& InstanceKey, const FYcQuestEvent& Event);
    /** 根据根目标状态刷新任务实例状态。 */
    void RefreshInstanceStateFromObjective(UYcQuestInstance* Instance, const FYcQuestInstanceKey& InstanceKey);

    /** 解析某个任务阶段需要处理的 Bundle 列表。 */
    TArray<FName> ResolveBundlesForPhase(const FYcQuestInstanceKey& InstanceKey, EYcQuestPhase Phase) const;
    /** 将 QuestId 转换为任务定义资产 Id。 */
    FPrimaryAssetId ResolveQuestAssetId(FName QuestId) const;
    /** 生成 Bundle 引用状态表使用的稳定键。 */
    FString MakeBundleRefKey(const FPrimaryAssetId& AssetId, FName BundleName) const;

private:
    /** 当前使用的任务资源策略实例。 */
    UPROPERTY(Transient)
    TObjectPtr<UYcQuestAssetPolicy> AssetPolicy = nullptr;

    /** 当前使用的任务 Owner 解析器对象。 */
    UPROPERTY(Transient)
    TObjectPtr<UObject> OwnerResolverObject = nullptr;

    /** 当前使用的共享任务解析器对象。 */
    UPROPERTY(Transient)
    TObjectPtr<UObject> ShareResolverObject = nullptr;

    /** 任务资源策略类配置。 */
    UPROPERTY(Config)
    TSoftClassPtr<UYcQuestAssetPolicy> AssetPolicyClass;

    /** 任务 Owner 解析器类配置。 */
    UPROPERTY(Config)
    TSoftClassPtr<UObject> OwnerResolverClass;

    /** 共享任务解析器类配置。 */
    UPROPERTY(Config)
    TSoftClassPtr<UObject> ShareResolverClass;

    /** 当前运行时任务实例表。 */
    UPROPERTY(Transient)
    TMap<FYcQuestInstanceKey, TObjectPtr<UYcQuestInstance>> RuntimeQuestInstances;

    /** Bundle 引用计数与就绪状态表。 */
    UPROPERTY(Transient)
    TMap<FString, FYcQuestBundleRefState> BundleRefStates;

    /** 事件去重时间戳缓存。 */
    UPROPERTY(Transient)
    TMap<FGuid, int64> EventDedupTimestamps;

    /** 共享任务成员映射表。 */
    UPROPERTY(Transient)
    TMap<FYcQuestInstanceKey, FYcQuestSharedMembership> SharedQuestMemberships;

    /** 当前持有的 Bundle StreamableHandle。 */
    TMap<FString, TSharedPtr<FStreamableHandle>> BundleHandles;
    /** 世界清理监听句柄。 */
    FDelegateHandle WorldCleanupHandle;
};
