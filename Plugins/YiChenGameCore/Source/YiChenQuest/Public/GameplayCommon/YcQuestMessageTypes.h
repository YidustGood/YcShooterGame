// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"
#include "YcQuestTypes.h"
#include "YcQuestMessageTypes.generated.h"

/** 任务系统对外广播的 GameplayTag 消息键。 */
namespace YcQuestGameplayTags
{
    /** 任务资源 Bundle 开始加载。 */
    YICHENQUEST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Quest_Asset_BundleLoading);
    /** 任务资源 Bundle 加载完成。 */
    YICHENQUEST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Quest_Asset_BundleReady);
    /** 任务资源 Bundle 加载失败。 */
    YICHENQUEST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Quest_Asset_BundleFailed);
    /** 任务接取请求被拒绝。 */
    YICHENQUEST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Quest_Accept_Rejected);
    /** 任务状态发生变化。 */
    YICHENQUEST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Quest_State_Changed);
    /** 任务业务层状态发生变化。 */
    YICHENQUEST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Quest_Business_StateChanged);
    /** 示例倒计时开始。 */
    YICHENQUEST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Quest_Demo_Countdown_Started);
    /** 示例倒计时更新。 */
    YICHENQUEST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Quest_Demo_Countdown_Updated);
    /** 示例倒计时结束。 */
    YICHENQUEST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Quest_Demo_Countdown_Finished);
    /** 示例任务完成事件。 */
    YICHENQUEST_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Quest_Demo_Event_CountdownFinished);
}

/** 任务资源 Bundle 相关消息。 */
USTRUCT(BlueprintType)
struct YICHENQUEST_API FYcQuestAssetBundleMessage
{
    GENERATED_BODY()

    /** 关联的任务实例 Key。 */
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    FYcQuestInstanceKey InstanceKey;

    /** 任务定义 Id。 */
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    FName QuestId = NAME_None;

    /** Bundle 名称。 */
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    FName BundleName = NAME_None;

    /** 当前操作是否成功。 */
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    bool bSuccess = false;

    /** 附加说明。 */
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    FString Detail;
};

/** 任务接取被拒绝时的消息。 */
USTRUCT(BlueprintType)
struct YICHENQUEST_API FYcQuestAcceptRejectedMessage
{
    GENERATED_BODY()

    /** 原本尝试接取的任务实例 Key。 */
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    FYcQuestInstanceKey InstanceKey;

    /** 尝试接取的任务 Id。 */
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    FName QuestId = NAME_None;

    /** 与之冲突的任务 Id。 */
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    FName ConflictQuestId = NAME_None;

    /** 冲突所在的互斥组 Id。 */
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    FName ExclusiveGroupId = NAME_None;

    /** 拒绝原因。 */
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    FString Reason;
};

/** 任务状态变化消息。 */
USTRUCT(BlueprintType)
struct YICHENQUEST_API FYcQuestStateChangedMessage
{
    GENERATED_BODY()

    /** 关联的任务实例 Key。 */
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    FYcQuestInstanceKey InstanceKey;

    /** 任务定义 Id。 */
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    FName QuestId = NAME_None;

    /** 新状态。 */
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    EYcQuestState NewState = EYcQuestState::Available;

    /** 状态变化说明。 */
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    FString Detail;
};

/** 任务业务层状态变化消息。 */
USTRUCT(BlueprintType)
struct YICHENQUEST_API FYcQuestBusinessStateChangedMessage
{
    GENERATED_BODY()

    /** 关联的任务实例 Key。 */
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    FYcQuestInstanceKey InstanceKey;

    /** 任务定义 Id。 */
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    FName QuestId = NAME_None;

    /** 新状态。 */
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    EYcQuestState NewState = EYcQuestState::Available;

    /** 状态变化说明。 */
    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    FString Detail;

    /** 业务层扩展负载。 */
    UPROPERTY(BlueprintReadOnly, Category = "Quest|Business")
    FInstancedStruct BusinessPayload;
};
