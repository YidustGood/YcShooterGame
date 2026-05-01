// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "YiChenGameCore/Public/YcReplicableObject.h"
#include "YcQuestTypes.h"
#include "YcQuestInstance.generated.h"

class UYcQuestEffect;
class UYcQuestObjective;

/**
 * 任务运行时实例。
 * 它承载一条任务在当前世界中的状态、版本、复制负载以及运行时目标树，
 * 是任务子系统在内存里管理任务的核心对象。
 */
UCLASS(BlueprintType)
class YICHENQUEST_API UYcQuestInstance : public UYcReplicableObject
{
    GENERATED_BODY()

public:
    UYcQuestInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    /** 注册需要进行网络复制的字段。 */
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /** 初始化一条全新的任务实例。 */
    UFUNCTION(BlueprintCallable, Category = "Quest|Instance")
    void InitializeInstance(const FYcQuestInstanceKey& InInstanceKey, FName InQuestId, EYcQuestState InInitialState);

    /** 从快照恢复一条已有的任务实例。 */
    UFUNCTION(BlueprintCallable, Category = "Quest|Instance")
    void RestoreFromSnapshot(const FYcQuestInstanceKey& InInstanceKey, FName InQuestId, EYcQuestState InState, int32 InVersion, int64 InLastUpdatedUnixTime, const FInstancedStruct& InReplicatedPayload);

    /** 更新任务状态。 */
    UFUNCTION(BlueprintCallable, Category = "Quest|Instance")
    void SetState(EYcQuestState InState, const FString& Detail);

    /** 设置需要网络复制给客户端的扩展负载。 */
    UFUNCTION(BlueprintCallable, Category = "Quest|Instance")
    void SetReplicatedPayload(const FInstancedStruct& InPayload);

    /** 仅提升版本与时间戳，不改变其他语义数据。 */
    UFUNCTION(BlueprintCallable, Category = "Quest|Instance")
    void TouchVersion();

    /** 获取任务实例 Key。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Instance")
    const FYcQuestInstanceKey& GetInstanceKey() const;

    /** 获取任务定义 Id。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Instance")
    FName GetQuestId() const;

    /** 获取当前任务状态。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Instance")
    EYcQuestState GetState() const;

    /** 获取版本号。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Instance")
    int32 GetVersion() const;

    /** 获取最近更新时间。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Instance")
    int64 GetLastUpdatedUnixTime() const;

    /** 获取复制负载。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Instance")
    const FInstancedStruct& GetReplicatedPayload() const;

    /** 判断任务是否处于活跃状态。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Instance")
    bool IsActiveState() const;

    /** 设置运行时根目标对象。 */
    void SetRuntimeRootObjective(UYcQuestObjective* InRootObjective);
    /** 获取运行时根目标对象。 */
    UYcQuestObjective* GetRuntimeRootObjective() const;

    /** 设置运行时任务级效果列表。 */
    void SetRuntimeQuestEffects(const TArray<TObjectPtr<UYcQuestEffect>>& InQuestEffects);
    /** 获取运行时任务级效果列表。 */
    const TArray<TObjectPtr<UYcQuestEffect>>& GetRuntimeQuestEffects() const;

private:
    /** 更新版本号和最近更新时间。 */
    void UpdateTimestampAndVersion();

private:
    /** 任务实例 Key。 */
    UPROPERTY(Replicated)
    FYcQuestInstanceKey InstanceKey;

    /** 任务定义 Id。 */
    UPROPERTY(Replicated)
    FName QuestId = NAME_None;

    /** 当前任务状态。 */
    UPROPERTY(Replicated)
    EYcQuestState State = EYcQuestState::Available;

    /** 当前实例版本号。 */
    UPROPERTY(Replicated)
    int32 Version = 1;

    /** 最近一次更新时间戳。 */
    UPROPERTY(Replicated)
    int64 LastUpdatedUnixTime = 0;

    /** 需要向客户端同步的扩展负载。 */
    UPROPERTY(Replicated)
    FInstancedStruct ReplicatedPayload;

    /** 运行时根目标对象，不参与网络复制。 */
    UPROPERTY(Transient)
    TObjectPtr<UYcQuestObjective> RuntimeRootObjective = nullptr;

    /** 运行时任务级效果列表，不参与网络复制。 */
    UPROPERTY(Transient)
    TArray<TObjectPtr<UYcQuestEffect>> RuntimeQuestEffects;
};
