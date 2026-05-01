// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "System/YcAccountTypes.h"
#include "YcLocalAccountSessionSaveGame.generated.h"

/**
 * 本地账号会话存档对象。
 * 本地离线适配器会把会话快照序列化到这里，以便下次启动时恢复账号与角色状态。
 */
UCLASS()
class YICHENACCOUNTCORE_API UYcLocalAccountSessionSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    /** 本地会话存档的数据版本，便于未来升级结构时兼容旧数据。 */
    UPROPERTY()
    int32 DataVersion = 1;

    /** 需要持久化的完整会话快照。 */
    UPROPERTY()
    FYcSessionSnapshot SessionSnapshot;

    /** 对应的平台用户 Id，用于校验槽位内容是否匹配当前玩家。 */
    UPROPERTY()
    FString PlatformUserId;
};
