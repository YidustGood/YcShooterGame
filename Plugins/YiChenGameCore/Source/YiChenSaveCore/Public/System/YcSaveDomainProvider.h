// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "YcSaveDomainProvider.generated.h"

/**
 * 存档业务域 Provider 抽象基类。
 * 各模块通过实现此类接入 SaveCore。
 */
UCLASS(Abstract)
class YICHENSAVECORE_API UYcSaveDomainProvider : public UObject
{
    GENERATED_BODY()

public:
    /** 返回域唯一键。 */
    virtual FName GetDomainKey() const PURE_VIRTUAL(UYcSaveDomainProvider::GetDomainKey, return NAME_None;);
    /** 返回域载荷版本。 */
    virtual int32 GetDomainVersion() const { return 1; }
    /** 判断当前 Provider 是否能处理该上下文。 */
    virtual bool CanHandleContext(const UObject* ContextObject) const
    {
        return IsValid(ContextObject);
    }

    /** 从运行时上下文构建域载荷。 */
    virtual bool BuildDomainPayload(const UObject* ContextObject, TArray<uint8>& OutPayloadBytes, FString& OutReason) const PURE_VIRTUAL(UYcSaveDomainProvider::BuildDomainPayload, return false;);
    /** 将域载荷应用回运行时上下文。 */
    virtual bool ApplyDomainPayload(UObject* ContextObject, const TArray<uint8>& PayloadBytes, FString& OutReason) const PURE_VIRTUAL(UYcSaveDomainProvider::ApplyDomainPayload, return false;);
};
