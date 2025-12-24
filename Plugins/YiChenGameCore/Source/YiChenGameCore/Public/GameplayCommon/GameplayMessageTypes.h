// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"
#include "GameplayMessageTypes.generated.h"

namespace YcGameplayTags
{
	YICHENGAMECORE_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_GameMode_PlayerInitialized);
}

/**
 * 玩家登录后事件消息, 当玩家或机器人加入游戏时触, 以及在无缝和非无缝旅行后触发, 此时Controller已经初始化完成了
 * 配合GameplayMessageSubsystem广播消息, 实现跨模块低耦合的响应玩家加入事件
 * 是从GameMode的GenericPlayerInitialization()中广播消息, 由PostLogin()和HandleSeamlessTravelPlayer()发起的
 */
USTRUCT(BlueprintType)
struct FGameModePlayerInitializedMessage
{
	GENERATED_BODY()
	FGameModePlayerInitializedMessage() = default;
	FGameModePlayerInitializedMessage(const TObjectPtr<AGameModeBase> InGameMode, const TObjectPtr<AController> InNewPlayer):
	GameMode(InGameMode), NewPlayer(InNewPlayer){}
	
	/** GameMode对象 */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AGameModeBase> GameMode;
	
	/** 新的控制器, 可能是AI也可能是真实玩家 */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AController> NewPlayer;
};

/**
 * 游戏阶段消息, 用于发布切换游戏阶段的消息,
 * YiChenGamePhase模块的UYcGamePhaseComponent组件会监听消息处理游戏阶段切换逻辑
 * 其它模块可以通过GameplayMessageSubsystem发送这个消息来与之交互, 实现模块间的解耦
 */
USTRUCT(BlueprintType)
struct FGamePhaseMessage
{
	GENERATED_BODY()
	/** 游戏阶段标签 */
	UPROPERTY(BlueprintReadOnly)
	FGameplayTag PhaseTag;
};