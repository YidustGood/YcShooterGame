// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YcQuestTypes.h"
#include "YcQuestRouteLibrary.generated.h"

class UYcQuestInstance;

/**
 * 任务路由蓝图库。
 * 用于在不知道具体任务子系统持有者的情况下，按作用域和实例 Key 把调用统一路由到任务子系统。
 */
UCLASS()
class YICHENQUEST_API UYcQuestRouteLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** 判断任务实例 Key 是否有效。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Route")
    static bool IsQuestInstanceKeyValid(const FYcQuestInstanceKey& InstanceKey);

    /** 按实例 Key 向目标任务投递事件。 */
    UFUNCTION(BlueprintCallable, Category = "Quest|Route", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject", HidePin = "WorldContextObject"))
    static bool SubmitQuestEventToInstanceByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, const FYcQuestEvent& Event);

    /** 按实例 Key 请求服务端完成任务。 */
    UFUNCTION(BlueprintCallable, Category = "Quest|Route", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject", HidePin = "WorldContextObject"))
    static bool ServerCompleteQuestByInstanceByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, const FString& Detail = TEXT("Completed"));

    /** 按实例 Key 请求任务相关 Bundle。 */
    UFUNCTION(BlueprintCallable, Category = "Quest|Route", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject", HidePin = "WorldContextObject"))
    static bool RequestQuestBundlesByInstanceByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, EYcQuestPhase Phase);

    /** 按实例 Key 释放任务相关 Bundle。 */
    UFUNCTION(BlueprintCallable, Category = "Quest|Route", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject", HidePin = "WorldContextObject"))
    static void ReleaseQuestBundlesByInstanceByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, EYcQuestPhase Phase);

    /** 按实例 Key 设置任务复制负载。 */
    UFUNCTION(BlueprintCallable, Category = "Quest|Route", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject", HidePin = "WorldContextObject"))
    static bool ServerSetQuestReplicatedPayloadByInstanceByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, const FInstancedStruct& Payload);

    /** 按实例 Key 获取任务复制负载。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Route", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject", HidePin = "WorldContextObject"))
    static bool GetQuestReplicatedPayloadByInstanceByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, FInstancedStruct& OutPayload);

    /** 按实例 Key 获取当前任务公开进度。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Route", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject", HidePin = "WorldContextObject"))
    static bool GetQuestPublicProgressByInstanceByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, TArray<FYcQuestPublicProgress>& OutProgress);

    /** 按实例 Key 更新共享任务成员列表。 */
    UFUNCTION(BlueprintCallable, Category = "Quest|Route", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject", HidePin = "WorldContextObject"))
    static bool ServerUpdateSharedQuestMembersByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, const TArray<FString>& MemberIds);

    /** 判断指定玩家是否属于某共享任务。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Route", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject", HidePin = "WorldContextObject"))
    static bool IsPlayerInSharedQuestByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, const FString& PlayerId);

    /** 获取某共享任务的当前成员列表。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Route", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject", HidePin = "WorldContextObject"))
    static bool GetSharedQuestMembersByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, TArray<FString>& OutMemberIds);

    /** 获取指定作用域下当前本地可见的任务实例列表。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Route", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject", HidePin = "WorldContextObject"))
    static void GetLocalQuestInstancesByScope(UObject* WorldContextObject, EYcQuestScope Scope, TArray<UYcQuestInstance*>& OutInstances);

    /** 按实例 Key 获取运行时任务实例对象。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Route", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject", HidePin = "WorldContextObject"))
    static bool GetQuestInstanceByKeyByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, UYcQuestInstance*& OutInstance);
};
