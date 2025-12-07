// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameFramework/GameSession.h"
#include "YcGameSession.generated.h"

/**
 * 游戏会话类
 * 
 * 继承自AGameSession，用于管理游戏会话的基本功能。
 * 可根据项目需求进行扩展，实现自定义的会话管理逻辑。
 */
UCLASS(Config = Game)
class YICHENGAMEPLAY_API AYcGameSession : public AGameSession
{
	GENERATED_BODY()
public:
	AYcGameSession(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
