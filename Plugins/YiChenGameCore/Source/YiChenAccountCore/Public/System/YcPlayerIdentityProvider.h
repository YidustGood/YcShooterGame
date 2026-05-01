// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "System/YcAccountTypes.h"
#include "YcPlayerIdentityProvider.generated.h"

/**
 * 玩家身份提供者接口。
 * 当某个 Actor、Component 或系统对象需要向外暴露“当前归属于哪个玩家/账号/角色”时，
 * 可以实现此接口，供外部统一读取身份快照。
 */
UINTERFACE(BlueprintType)
class YICHENACCOUNTCORE_API UYcPlayerIdentityProvider : public UInterface
{
    GENERATED_BODY()
};

class YICHENACCOUNTCORE_API IYcPlayerIdentityProvider
{
    GENERATED_BODY()

public:
    /** 获取当前对象代表的玩家身份快照。 */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Account")
    bool GetPlayerIdentity(FYcPlayerIdentitySnapshot& OutIdentity) const;
};
