// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "System/YcSaveDomainProvider.h"
#include "YcQuestSaveDomainProvider.generated.h"

/**
 * 任务存档域提供者。
 * 用于把任务子系统的运行时数据接入 YiChenGameCore 的通用存档框架，
 * 使任务状态能够按域独立序列化与恢复。
 */
UCLASS()
class YICHENQUEST_API UYcQuestSaveDomainProvider : public UYcSaveDomainProvider
{
    GENERATED_BODY()

public:
    /** 任务存档域的固定 DomainKey。 */
    static FName DomainKey;

    /** 返回任务存档域 Key。 */
    virtual FName GetDomainKey() const override;
    /** 返回任务存档域版本。 */
    virtual int32 GetDomainVersion() const override;
    /** 判断给定上下文是否可由该 Provider 处理。 */
    virtual bool CanHandleContext(const UObject* ContextObject) const override;
    /** 从任务子系统构建域负载字节流。 */
    virtual bool BuildDomainPayload(const UObject* ContextObject, TArray<uint8>& OutPayloadBytes, FString& OutReason) const override;
    /** 把域负载字节流应用回任务子系统。 */
    virtual bool ApplyDomainPayload(UObject* ContextObject, const TArray<uint8>& PayloadBytes, FString& OutReason) const override;
};
