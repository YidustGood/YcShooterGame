// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "System/YcSaveDomainProvider.h"
#include "YcInventorySaveDomainProvider.generated.h"

/** Inventory 业务域存档 Provider。 */
UCLASS()
class YICHENINVENTORY_API UYcInventorySaveDomainProvider : public UYcSaveDomainProvider
{
    GENERATED_BODY()

public:
    /** Inventory 域键。 */
    static FName DomainKey;

    /** 返回域键。 */
    virtual FName GetDomainKey() const override;
    /** 返回域版本。 */
    virtual int32 GetDomainVersion() const override;
    /** 仅处理 Inventory 场景上下文。 */
    virtual bool CanHandleContext(const UObject* ContextObject) const override;
    /** 构建 Inventory 域载荷。 */
    virtual bool BuildDomainPayload(const UObject* ContextObject, TArray<uint8>& OutPayloadBytes, FString& OutReason) const override;
    /** 应用 Inventory 域载荷。 */
    virtual bool ApplyDomainPayload(UObject* ContextObject, const TArray<uint8>& PayloadBytes, FString& OutReason) const override;
};
