// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StructUtils/InstancedStruct.h"
#include "YcQuestTypes.h"
#include "YcQuestObjective.generated.h"

class UYcQuestEffect;
class UYcQuestSubsystem;

/** 计数型目标的自定义运行时快照。 */
USTRUCT(BlueprintType)
struct YICHENQUEST_API FYcQuestCounterObjectiveSnapshot
{
    GENERATED_BODY()

    /** 当前累计值。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    float CurrentValue = 0.0f;
};

/** 区域驻留型目标的自定义运行时快照。 */
USTRUCT(BlueprintType)
struct YICHENQUEST_API FYcQuestHoldAreaObjectiveSnapshot
{
    GENERATED_BODY()

    /** 当前是否处于驻留状态。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    bool bHolding = false;

    /** 当前累计驻留秒数。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    float HeldSeconds = 0.0f;
};

/**
 * 任务目标基类。
 * 一个任务定义会以目标树的形式组织运行逻辑，每个节点都可以接收事件、逐帧推进、导出快照并触发效果。
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class YICHENQUEST_API UYcQuestObjective : public UObject
{
    GENERATED_BODY()

public:
    /** 目标唯一标识。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    FName ObjectiveId = NAME_None;

    /** 目标展示文本。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    FText DisplayText;

    /** 是否为可选目标。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    bool bOptional = false;

    /** 是否在公开进度中显示。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    bool bVisibleInPublicProgress = true;

    /** 子目标列表。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "Quest")
    TArray<TObjectPtr<UYcQuestObjective>> ChildObjectives;

    /** 绑定在当前目标上的效果列表。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Instanced, Category = "Quest")
    TArray<TObjectPtr<UYcQuestEffect>> Effects;

    /** 激活当前目标。 */
    void ActivateObjective(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey);
    /** 取消激活当前目标。 */
    void DeactivateObjective(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey);
    /** 向当前目标投递任务事件。 */
    void HandleQuestEvent(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const FYcQuestEvent& Event);
    /** Tick 当前目标。 */
    void TickObjective(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, float DeltaSeconds);
    /** 从快照恢复当前目标与子目标的运行时状态。 */
    void RestoreRuntimeSnapshot(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const TArray<FYcQuestObjectiveRuntimeSnapshot>& Snapshots);
    /** 把当前目标及其子目标的运行时状态写入快照数组。 */
    void AppendRuntimeSnapshots(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, FName ParentObjectiveId, TArray<FYcQuestObjectiveRuntimeSnapshot>& OutSnapshots) const;
    /** 收集当前目标树中允许公开显示的进度项。 */
    void GatherVisibleProgress(TArray<FYcQuestPublicProgress>& OutProgress) const;

    /** 获取当前目标状态。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
    EYcQuestObjectiveState GetObjectiveState() const;

    /** 获取当前目标对外公开的进度。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
    FYcQuestPublicProgress GetPublicProgress() const;

    /** 将目标标记为完成。 */
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void CompleteObjective(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey);

    /** 将目标标记为失败。 */
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void FailObjective(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey);

    /** 将目标标记为阻塞。 */
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void BlockObjective();

    /** 设置目标公开进度，并触发进度变化效果。 */
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void SetPublicProgress(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const FString& InDisplayText, float InCurrentValue, float InTargetValue, float InRemainingSeconds, bool bInVisible);

protected:
    /** 激活初始子目标，默认会激活所有仍处于 Inactive 的子节点。 */
    virtual void ActivateInitialChildren(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey);
    /** 把任务事件继续分发给子目标。 */
    virtual void HandleChildrenQuestEvent(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const FYcQuestEvent& Event);
    /** Tick 所有子目标。 */
    virtual void TickChildren(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, float DeltaSeconds);
    /** 根据子目标状态刷新当前目标的完成或失败结果。 */
    virtual void RefreshCompletionFromChildren(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey);

    /** 触发当前目标绑定的效果列表。 */
    void DispatchEffects(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, EYcQuestEffectTrigger Trigger) const;

    /** 目标激活时的扩展钩子。 */
    UFUNCTION(BlueprintNativeEvent, Category = "Quest")
    void OnActivated(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey);

    /** 目标停用时的扩展钩子。 */
    UFUNCTION(BlueprintNativeEvent, Category = "Quest")
    void OnDeactivated(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey);

    /** 收到任务事件时的扩展钩子。 */
    UFUNCTION(BlueprintNativeEvent, Category = "Quest")
    void OnQuestEvent(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const FYcQuestEvent& Event);

    /** 目标 Tick 时的扩展钩子。 */
    UFUNCTION(BlueprintNativeEvent, Category = "Quest")
    void OnObjectiveTick(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, float DeltaSeconds);

    /** 目标完成时的扩展钩子。 */
    UFUNCTION(BlueprintNativeEvent, Category = "Quest")
    void OnCompleted(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey);

    /** 目标失败时的扩展钩子。 */
    UFUNCTION(BlueprintNativeEvent, Category = "Quest")
    void OnFailed(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey);

    /** 导出当前目标的自定义快照。 */
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Quest")
    FInstancedStruct ExportCustomSnapshot(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey) const;

    /** 导入当前目标的自定义快照。 */
    UFUNCTION(BlueprintNativeEvent, Category = "Quest")
    void ImportCustomSnapshot(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const FInstancedStruct& Snapshot);

protected:
    /** 运行时状态。 */
    UPROPERTY(Transient)
    EYcQuestObjectiveState RuntimeState = EYcQuestObjectiveState::Inactive;

    /** 目标激活后累计的活跃时间。 */
    UPROPERTY(Transient)
    float RuntimeActiveSeconds = 0.0f;

    /** 当前目标公开给外部系统的进度状态。 */
    UPROPERTY(Transient)
    FYcQuestPublicProgress RuntimePublicProgress;
};

/** 顺序目标，要求子目标按顺序逐个完成。 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class YICHENQUEST_API UYcQuestSequenceObjective : public UYcQuestObjective
{
    GENERATED_BODY()

protected:
    virtual void ActivateInitialChildren(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey) override;
    virtual void RefreshCompletionFromChildren(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey) override;
};

/** 并行目标，允许多个子目标同时推进。 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class YICHENQUEST_API UYcQuestParallelObjective : public UYcQuestObjective
{
    GENERATED_BODY()

protected:
    virtual void ActivateInitialChildren(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey) override;
    virtual void RefreshCompletionFromChildren(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey) override;
};

/** 计数型目标，通过事件次数或事件幅值累计推进。 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class YICHENQUEST_API UYcQuestCounterObjective : public UYcQuestObjective
{
    GENERATED_BODY()

public:
    /** 监听的任务事件 Tag。收到匹配事件时推进一次，或按 Magnitude 推进。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    FGameplayTag EventTag;

    /** 目标进度值。CurrentValue 达到该值后目标完成。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    float TargetValue = 1.0f;

    /** 是否使用事件的 Magnitude 作为推进值；否则每次固定推进 1。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    bool bUseEventMagnitude = false;

protected:
    virtual void OnActivated_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey) override;
    virtual void OnQuestEvent_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const FYcQuestEvent& Event) override;
    virtual FInstancedStruct ExportCustomSnapshot_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey) const override;
    virtual void ImportCustomSnapshot_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const FInstancedStruct& Snapshot) override;

private:
    /** 运行时累计值。 */
    UPROPERTY(Transient)
    float CurrentValue = 0.0f;
};

/** 计时型目标，激活后持续累计时间直到满足要求。 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class YICHENQUEST_API UYcQuestTimedObjective : public UYcQuestObjective
{
    GENERATED_BODY()

public:
    /** 目标要求持续的秒数。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    float RequiredSeconds = 1.0f;

protected:
    virtual void OnActivated_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey) override;
    virtual void OnObjectiveTick_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, float DeltaSeconds) override;
};

/** 区域驻留型目标，需要玩家进入保持状态并累计驻留时长。 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class YICHENQUEST_API UYcQuestHoldAreaObjective : public UYcQuestObjective
{
    GENERATED_BODY()

public:
    /** 开始驻留时监听的事件 Tag。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    FGameplayTag HoldStartedTag;

    /** 结束驻留时监听的事件 Tag。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    FGameplayTag HoldEndedTag;

    /** 目标要求累计的驻留秒数。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    float RequiredSeconds = 1.0f;

    /** 离开区域时是否重置累计进度。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    bool bResetProgressOnExit = true;

protected:
    virtual void OnActivated_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey) override;
    virtual void OnQuestEvent_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const FYcQuestEvent& Event) override;
    virtual void OnObjectiveTick_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, float DeltaSeconds) override;
    virtual FInstancedStruct ExportCustomSnapshot_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey) const override;
    virtual void ImportCustomSnapshot_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const FInstancedStruct& Snapshot) override;

private:
    /** 当前是否处于驻留状态。 */
    UPROPERTY(Transient)
    bool bHolding = false;

    /** 当前已累计的驻留秒数。 */
    UPROPERTY(Transient)
    float HeldSeconds = 0.0f;
};
