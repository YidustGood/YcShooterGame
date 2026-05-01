// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "YcQuestTypes.h"
#include "YcQuestEffect.generated.h"

class UYcQuestSubsystem;

/**
 * 任务效果基类。
 * 用于在任务或目标发生关键状态变化时执行扩展逻辑，
 * 例如发奖、播消息、推进业务状态或触发其他系统联动。
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class YICHENQUEST_API UYcQuestEffect : public UObject
{
    GENERATED_BODY()

public:
    /** 效果唯一标识，便于配置层和调试日志识别具体效果。 */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    FName EffectId = NAME_None;

    /**
     * 执行任务效果。
     * @param QuestSubsystem 任务子系统上下文。
     * @param InstanceKey 当前任务实例 Key。
     * @param Trigger 触发原因，例如任务完成、目标完成、进度变化等。
     * @param ObjectiveId 关联的目标 Id；若为任务级效果则可能为空。
     * @param Progress 当前公开进度快照。
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Quest")
    void ExecuteEffect(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, EYcQuestEffectTrigger Trigger, FName ObjectiveId, const FYcQuestPublicProgress& Progress);
};
