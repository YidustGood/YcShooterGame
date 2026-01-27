// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "ModularGameState.h"
#include "YcGameState.generated.h"

struct FYcGameVerbMessage;
class UYcExperienceManagerComponent;
class UYcAbilitySystemComponent;

/**
 * 游戏状态类
 * 
 * 负责管理游戏体验的加载和生命周期。持有ExperienceManagerComponent组件，
 * 该组件负责Experience的加载、GameFeature插件的激活、以及GameFeatureAction的执行。
 * 作为GameState的扩展，支持模块化架构。
 */
UCLASS(Config = Game)
class YICHENGAMEPLAY_API AYcGameState : public AModularGameStateBase, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AYcGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	//~AActor interface
	/* 组件初始化完成后设置ASC组件的Avatar为自己 */
	virtual void PostInitializeComponents() override;
	//~End of AActor interface
	
	//~IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~End of IAbilitySystemInterface
	
	/**
	 * 向所有客户端发送不可靠消息（可能丢失）
	 * 仅用于可以容忍丢失的客户端通知，如击杀提示、玩家加入消息等
	 * 
	 * @param Message - 要发送的游戏消息
	 */
	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable, Category = "YcGameCore|GameState")
	void MulticastMessageToClients(const FYcGameVerbMessage Message);

	/**
	 * 向所有客户端发送可靠消息（保证送达）
	 * 仅用于不能容忍丢失的重要客户端通知
	 * 
	 * @param Message - 要发送的游戏消息
	 */
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "YcGameCore|GameState")
	void MulticastReliableMessageToClients(const FYcGameVerbMessage Message);
	
private:
	/**
	 * 游戏体验管理组件
	 * 负责加载和管理当前游戏体验，包括Experience资产的加载、GameFeature插件的激活、
	 * 以及GameFeatureAction的执行。支持网络复制以同步客户端状态。
	 */
	UPROPERTY()
	TObjectPtr<UYcExperienceManagerComponent> ExperienceManagerComponent;
	
	/**
	 * 游戏级别的能力系统组件
	 * 
	 * 功能说明：
	 * 用于处理游戏全局范围的GAS功能，主要用于播放全局性的Gameplay Cues（游戏提示效果）。
	 * 
	 * 典型应用场景：
	 * - 对局阶段：通过GA来组织游戏对局的阶段
	 * - 环境状态：毒圈收缩、安全区提示、全局Buff/Debuff效果
	 * - 全局音效：游戏开始/结束音效、回合切换音效
	 * - 全局特效：天气变化、昼夜循环、全场景光效
	 * - 全局UI提示：击杀播报、任务完成通知、系统公告
	 * 
	 * 设计考量：
	 * 与PlayerState/Pawn上的ASC不同，此组件不绑定到特定玩家或角色，
	 * 而是作为游戏全局状态的一部分，用于触发影响所有玩家的视听效果。
	 */
	UPROPERTY(VisibleAnywhere, Category = "YcGameCore|GameState")
	TObjectPtr<UYcAbilitySystemComponent> AbilitySystemComponent;
};