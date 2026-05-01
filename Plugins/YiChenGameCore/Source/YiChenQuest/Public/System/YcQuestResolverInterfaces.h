// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "YcQuestTypes.h"
#include "System/YcAccountTypes.h"
#include "YcQuestResolverInterfaces.generated.h"

class UYcQuestDefinition;

/** 任务归属解析器接口，用于把玩家身份或事件上下文映射为任务 Owner。 */
UINTERFACE(BlueprintType)
class YICHENQUEST_API UYcQuestOwnerResolver : public UInterface
{
    GENERATED_BODY()
};

class YICHENQUEST_API IYcQuestOwnerResolver
{
    GENERATED_BODY()

public:
    /**
     * 接取任务时解析 Owner。
     * 返回 true 表示成功输出 OwnerType 与 OwnerId。
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quest|Resolver")
    bool ResolveOwnerForAccept(const FYcPlayerIdentitySnapshot& PlayerIdentity, const UYcQuestDefinition* QuestDefinition, EYcQuestOwnerType& OutOwnerType, FString& OutOwnerId) const;

    /**
     * 事件推进时解析事件归属 Owner。
     * 返回 true 表示成功输出 OwnerType 与 OwnerId。
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quest|Resolver")
    bool ResolveOwnerForEvent(const FYcQuestEvent& Event, const UYcQuestDefinition* QuestDefinition, EYcQuestOwnerType& OutOwnerType, FString& OutOwnerId) const;
};

/** 共享任务解析器接口，用于解析共享组 Owner 和成员列表。 */
UINTERFACE(BlueprintType)
class YICHENQUEST_API UYcQuestShareResolver : public UInterface
{
    GENERATED_BODY()
};

class YICHENQUEST_API IYcQuestShareResolver
{
    GENERATED_BODY()

public:
    /** 共享任务接取时解析共享 Owner，通常映射为 SharedGroup + TeamId。 */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quest|Resolver")
    bool ResolveSharedOwnerForAccept(const FYcPlayerIdentitySnapshot& PlayerIdentity, const UYcQuestDefinition* QuestDefinition, EYcQuestOwnerType& OutOwnerType, FString& OutOwnerId) const;

    /** 共享任务事件推进时解析共享 Owner，通常映射为 SharedGroup + TeamId。 */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quest|Resolver")
    bool ResolveSharedOwnerForEvent(const FYcQuestEvent& Event, const UYcQuestDefinition* QuestDefinition, EYcQuestOwnerType& OutOwnerType, FString& OutOwnerId) const;

    /**
     * 解析共享任务当前有效成员列表。
     * @param OwnerType 共享 Owner 类型，通常为 SharedGroup。
     * @param OwnerId 共享 OwnerId，通常为 TeamId。
     * @param OutMemberPlayerIds 输出当前有效成员 PlayerId 列表。
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quest|Resolver")
    bool ResolveSharedMembers(EYcQuestOwnerType OwnerType, const FString& OwnerId, TArray<FString>& OutMemberPlayerIds) const;
};
