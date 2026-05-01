// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YcQuestTypes.h"
#include "YcQuestPayloadLibrary.generated.h"

class UYcQuestInstance;

/** “区域驻留同步”场景使用的标准化负载结构。 */
USTRUCT(BlueprintType)
struct YICHENQUEST_API FYcQuestAreaPresenceSyncPayload
{
    GENERATED_BODY()

    /** 当前范围内玩家数量。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    int32 PlayersInRange = 0;

    /** 当前累计激活秒数。 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
    int32 ActiveSeconds = 0;
};

/**
 * 任务负载编解码辅助。
 * 负责把常见任务扩展数据打包成 FInstancedStruct，便于网络复制和事件传递时保持统一结构。
 *
 * Angelscript 调用名：
 * - remove `U` prefix and `Library` suffix => `YcQuestPayload`
 */
UCLASS()
class YICHENQUEST_API UYcQuestPayloadLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** 构造“区域驻留同步”负载。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Payload")
    static FInstancedStruct MakeAreaPresenceSyncPayload(int32 PlayersInRange, int32 ActiveSeconds);

    /** 从负载中解析“区域驻留同步”字段。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Payload")
    static bool ReadAreaPresenceSyncPayload(const FInstancedStruct& Payload, int32& OutPlayersInRange, int32& OutActiveSeconds);

    /** 从任务实例复制负载中解析“区域驻留同步”字段。 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest|Payload")
    static bool ReadAreaPresenceSyncPayloadFromQuestInstance(const UYcQuestInstance* QuestInstance, int32& OutPlayersInRange, int32& OutActiveSeconds);
};
