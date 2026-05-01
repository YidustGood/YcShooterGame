// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "YcQuestTypes.h"
#include "YcQuestAssetPolicy.generated.h"

class UYcQuestDefinition;

/**
 * 任务资源策略。
 * 负责决定某个任务在不同阶段应该预加载哪些 Bundle，以及任务结束后哪些资源需要继续保留，
 * 用来把任务玩法流程和具体资源管理策略解耦。
 */
UCLASS(Blueprintable, EditInlineNew, DefaultToInstanced)
class YICHENQUEST_API UYcQuestAssetPolicy : public UObject
{
    GENERATED_BODY()

public:
    /**
     * 返回指定任务在给定阶段需要加载或保留的 Bundle 列表。
     * 常用于接取任务、进入对局、完成任务等阶段的资源预热。
     */
    UFUNCTION(BlueprintNativeEvent, Category = "Quest|Assets")
    void GetBundlesForPhase(FName QuestId, const UYcQuestDefinition* QuestDefinition, EYcQuestPhase Phase, TArray<FName>& OutBundles) const;

    /**
     * 判断任务完成后是否仍需保留指定 Bundle。
     * 适合用于剧情后续衔接、领奖界面或完成后回流流程的资源保活控制。
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintPure, Category = "Quest|Assets")
    bool ShouldRetainBundleAfterComplete(FName BundleName) const;
};
