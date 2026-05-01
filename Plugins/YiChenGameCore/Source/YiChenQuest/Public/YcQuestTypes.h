// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "YcQuestTypes.generated.h"

/** 任务作用域，决定任务实例在哪个生命周期范围内有效。 */
UENUM(BlueprintType)
enum class EYcQuestScope : uint8
{
    GlobalPersistent UMETA(DisplayName = "GlobalPersistent"),
    CrossSession UMETA(DisplayName = "CrossSession"),
    MatchOnly UMETA(DisplayName = "MatchOnly")
};

/** 任务持久化策略。 */
UENUM(BlueprintType)
enum class EYcQuestPersistencePolicy : uint8
{
    Required UMETA(DisplayName = "Required"),
    Optional UMETA(DisplayName = "Optional"),
    None UMETA(DisplayName = "None")
};

/** 任务归属对象类型。 */
UENUM(BlueprintType)
enum class EYcQuestOwnerType : uint8
{
    Player UMETA(DisplayName = "Player"),
    SharedGroup UMETA(DisplayName = "SharedGroup"),
    Match UMETA(DisplayName = "Match")
};

/** 任务进度的归属模式。 */
UENUM(BlueprintType)
enum class EYcQuestProgressOwnershipMode : uint8
{
    PerPlayer UMETA(DisplayName = "PerPlayer"),
    Shared UMETA(DisplayName = "Shared")
};

/** 任务实例状态机。 */
UENUM(BlueprintType)
enum class EYcQuestState : uint8
{
    Available UMETA(DisplayName = "Available"),
    Accepted UMETA(DisplayName = "Accepted"),
    InProgress UMETA(DisplayName = "InProgress"),
    Completed UMETA(DisplayName = "Completed"),
    Failed UMETA(DisplayName = "Failed"),
    Aborted UMETA(DisplayName = "Aborted")
};

/** 任务阶段，用于资源和业务响应。 */
UENUM(BlueprintType)
enum class EYcQuestPhase : uint8
{
    OnAccepted UMETA(DisplayName = "OnAccepted"),
    OnStartedInMatch UMETA(DisplayName = "OnStartedInMatch"),
    OnReturnOutOfMatch UMETA(DisplayName = "OnReturnOutOfMatch"),
    OnCompleted UMETA(DisplayName = "OnCompleted"),
    OnFailed UMETA(DisplayName = "OnFailed"),
    OnAborted UMETA(DisplayName = "OnAborted")
};

/** 任务事件路由范围。 */
UENUM(BlueprintType)
enum class EYcQuestEventRouteScope : uint8
{
    Global UMETA(DisplayName = "Global"),
    Match UMETA(DisplayName = "Match"),
    Both UMETA(DisplayName = "Both")
};

/** 任务目标运行时状态。 */
UENUM(BlueprintType)
enum class EYcQuestObjectiveState : uint8
{
    Inactive UMETA(DisplayName = "Inactive"),
    Active UMETA(DisplayName = "Active"),
    Completed UMETA(DisplayName = "Completed"),
    Failed UMETA(DisplayName = "Failed"),
    Blocked UMETA(DisplayName = "Blocked")
};

/** 任务效果触发点。 */
UENUM(BlueprintType)
enum class EYcQuestEffectTrigger : uint8
{
    QuestAccepted UMETA(DisplayName = "QuestAccepted"),
    QuestCompleted UMETA(DisplayName = "QuestCompleted"),
    QuestFailed UMETA(DisplayName = "QuestFailed"),
    QuestAborted UMETA(DisplayName = "QuestAborted"),
    ObjectiveActivated UMETA(DisplayName = "ObjectiveActivated"),
    ObjectiveCompleted UMETA(DisplayName = "ObjectiveCompleted"),
    ObjectiveFailed UMETA(DisplayName = "ObjectiveFailed"),
    ObjectiveProgressChanged UMETA(DisplayName = "ObjectiveProgressChanged")
};

/**
 * 任务实例 Key。
 * 它唯一定位一条任务实例，既包含 QuestId，也包含任务归属者与作用域，
 * 用于路由事件、持久化快照和查找运行时实例。
 */
USTRUCT(BlueprintType)
struct YICHENQUEST_API FYcQuestInstanceKey
{
    GENERATED_BODY()

    /** 任务定义 Id。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FName QuestId = NAME_None;

    /** 任务归属对象类型，例如个人、共享组或对局。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    EYcQuestOwnerType OwnerType = EYcQuestOwnerType::Player;

    /** 归属对象 Id。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FString OwnerId;

    /** 任务作用域。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    EYcQuestScope Scope = EYcQuestScope::GlobalPersistent;

    /** 当前 Key 是否足以定位一个有效任务实例。 */
    bool IsValid() const
    {
        return !QuestId.IsNone() && !OwnerId.IsEmpty();
    }

    /** 任务实例 Key 相等比较。 */
    bool operator==(const FYcQuestInstanceKey& Other) const
    {
        return QuestId == Other.QuestId
            && OwnerType == Other.OwnerType
            && OwnerId == Other.OwnerId
            && Scope == Other.Scope;
    }
};

/** 任务实例 Key 哈希，用于作为 TMap/TSet 键。 */
FORCEINLINE uint32 GetTypeHash(const FYcQuestInstanceKey& Key)
{
    uint32 Hash = GetTypeHash(Key.QuestId);
    Hash = HashCombineFast(Hash, GetTypeHash(static_cast<uint8>(Key.OwnerType)));
    Hash = HashCombineFast(Hash, GetTypeHash(Key.OwnerId));
    Hash = HashCombineFast(Hash, GetTypeHash(static_cast<uint8>(Key.Scope)));
    return Hash;
}

/** 推进任务的通用事件载体。 */
USTRUCT(BlueprintType)
struct YICHENQUEST_API FYcQuestEvent
{
    GENERATED_BODY()

    /** 事件唯一 Id，用于重复事件去重。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FGuid EventId;

    /** 事件标签，是目标匹配和分发的主要依据。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FGameplayTag EventTag;

    /** 事件发起者。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    TObjectPtr<UObject> Instigator = nullptr;

    /** 事件目标对象。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    TObjectPtr<UObject> Target = nullptr;

    /** 事件上下文标签。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FGameplayTagContainer ContextTags;

    /** 事件幅值，可用于计数、伤害、时长等数值推进。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    double Magnitude = 1.0;

    /** 事件扩展负载。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FInstancedStruct Payload;
};

/** 暴露给 UI 或外部系统的公开进度片段。 */
USTRUCT(BlueprintType)
struct YICHENQUEST_API FYcQuestPublicProgress
{
    GENERATED_BODY()

    /** 目标 Id。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FName ObjectiveId = NAME_None;

    /** 展示文本。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FString DisplayText;

    /** 当前进度值。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    float CurrentValue = 0.0f;

    /** 目标值。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    float TargetValue = 0.0f;

    /** 剩余秒数，常用于计时任务。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    float RemainingSeconds = 0.0f;

    /** 是否允许在公开进度中显示。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    bool bIsVisible = true;
};

/** 单个目标节点的运行时快照。 */
USTRUCT(BlueprintType)
struct YICHENQUEST_API FYcQuestObjectiveRuntimeSnapshot
{
    GENERATED_BODY()

    /** 父目标 Id，用于恢复目标树结构。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FName ParentObjectiveId = NAME_None;

    /** 当前目标 Id。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FName ObjectiveId = NAME_None;

    /** 目标类名，用于调试和校验恢复结果。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FString ObjectiveClassName;

    /** 目标运行时状态。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    EYcQuestObjectiveState State = EYcQuestObjectiveState::Inactive;

    /** 目标累计活跃秒数。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    float ActiveSeconds = 0.0f;

    /** 目标公开进度。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FYcQuestPublicProgress PublicProgress;

    /** 目标自定义快照数据。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FInstancedStruct CustomSnapshot;
};

/** 一条任务实例的完整运行时快照。 */
USTRUCT(BlueprintType)
struct YICHENQUEST_API FYcQuestRuntimeSnapshot
{
    GENERATED_BODY()

    /** 任务实例 Key。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FYcQuestInstanceKey InstanceKey;

    /** 任务定义 Id。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    FName QuestId = NAME_None;

    /** 当前任务状态。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    EYcQuestState State = EYcQuestState::Available;

    /** 版本号，用于增量同步和调试。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    int32 Version = 1;

    /** 最近更新时间戳。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    int64 LastUpdatedUnixTime = 0;

    /** 网络复制负载。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Network")
    FInstancedStruct ReplicatedPayload;

    /** 目标树运行时快照。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    TArray<FYcQuestObjectiveRuntimeSnapshot> ObjectiveSnapshots;

    /** 当前可见的任务公开进度列表。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    TArray<FYcQuestPublicProgress> VisibleObjectives;
};

/** 共享任务的成员映射快照。 */
USTRUCT(BlueprintType)
struct YICHENQUEST_API FYcQuestSharedMembership
{
    GENERATED_BODY()

    /** 对应的共享任务实例 Key。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Shared")
    FYcQuestInstanceKey InstanceKey;

    /** 共享组 Id，通常是队伍或阵营标识。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Shared")
    FString TeamId;

    /** 当前有效成员的 PlayerId 列表。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Shared")
    TArray<FString> ActiveMemberPlayerIds;

    /** 成员映射版本号。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Shared")
    int32 Version = 0;

    /** 最近更新时间。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest|Shared")
    int64 LastUpdatedTime = 0;
};

/** 任务系统整体存档快照。 */
USTRUCT(BlueprintType)
struct YICHENQUEST_API FYcQuestSaveSnapshot
{
    GENERATED_BODY()

    /** 存档快照版本。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    int32 SnapshotVersion = 2;

    /** 当前需要存储的任务实例快照列表。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    TArray<FYcQuestRuntimeSnapshot> RuntimeEntries;
};

/** Bundle 引用状态，用于跟踪引用计数和加载就绪标记。 */
USTRUCT()
struct YICHENQUEST_API FYcQuestBundleRefState
{
    GENERATED_BODY()

    /** 当前引用计数。 */
    int32 RefCount = 0;

    /** 当前 Bundle 是否已就绪。 */
    bool bReady = false;
};
