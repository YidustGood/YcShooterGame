// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"
#include "GameplayMessageTypes.generated.h"

/**
 * 插件的游戏标签命名空间
 * 用于定义和声明项目中使用的各种游戏标签
 */
namespace YcGameplayTags
{
	/** 玩家登入游戏Controller初始化完成事件消息通道标签 */
	YICHENGAMECORE_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Gameplay_GameMode_PlayerInitialized);
	
	/** ASC组件初始化完成消息标签 */
	YICHENGAMECORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_State_AbilitySystemInitialized);
	
	/** ASC组件取消初始化消息标签 */
	YICHENGAMECORE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_State_AbilitySystemUninitialized);
}

/**
 * 玩家初始化完成事件消息
 * 当玩家或机器人加入游戏时触发，以及在无缝和非无缝旅行后触发，此时Controller已经初始化完成
 * 配合GameplayMessageSubsystem广播消息，实现跨模块低耦合的响应玩家加入事件
 * 消息从GameMode的GenericPlayerInitialization()中广播，由PostLogin()和HandleSeamlessTravelPlayer()发起
 */
USTRUCT(BlueprintType)
struct FGameModePlayerInitializedMessage
{
	GENERATED_BODY()
	
	/** 默认构造函数 */
	FGameModePlayerInitializedMessage() = default;
	
	/**
	 * 带参数的构造函数
	 * @param InGameMode 游戏模式对象
	 * @param InNewPlayer 新加入的控制器
	 */
	FGameModePlayerInitializedMessage(const TObjectPtr<AGameModeBase> InGameMode, const TObjectPtr<AController> InNewPlayer):
	GameMode(InGameMode), NewPlayer(InNewPlayer){}
	
	/** 游戏模式对象 */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AGameModeBase> GameMode;
	
	/** 新加入的控制器，可能是AI控制器也可能是真实玩家控制器 */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AController> NewPlayer;
};

/**
 * 游戏阶段切换消息
 * 用于发布游戏阶段切换事件，YiChenGamePhase模块的UYcGamePhaseComponent组件会监听此消息并处理游戏阶段切换逻辑
 * 其它模块可以通过GameplayMessageSubsystem发送此消息来触发阶段切换，实现模块间的解耦
 */
USTRUCT(BlueprintType)
struct FGamePhaseMessage
{
	GENERATED_BODY()
	
	/** 目标游戏阶段的标签 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag PhaseTag;
};

/**
 * 技能系统组件生命周期消息
 * 用于通知技能系统组件的初始化或反初始化事件
 * 配合GameplayMessageSubsystem使用，实现跨模块监听ASC生命周期变化
 */
USTRUCT(BlueprintType)
struct FAbilitySystemLifeCycleMessage
{
	GENERATED_BODY()
	
	/** 消息所属Actor, 监听方通过对比Owner可以判断是否为自己的ASC生命周期消息, 以此保证处理正确的ASC对象生命周期 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<AActor> Owner = nullptr;
	
	/** 技能系统组件对象指针 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UObject* ASC = nullptr;
};