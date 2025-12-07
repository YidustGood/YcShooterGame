// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "ModularGameState.h"
#include "YcGameState.generated.h"

class UYcExperienceManagerComponent;

/**
 * 游戏状态类
 * 
 * 负责管理游戏体验的加载和生命周期。持有ExperienceManagerComponent组件，
 * 该组件负责Experience的加载、GameFeature插件的激活、以及GameFeatureAction的执行。
 * 作为GameState的扩展，支持模块化架构。
 */
UCLASS(Config = Game)
class YICHENGAMEPLAY_API AYcGameState : public AModularGameStateBase
{
	GENERATED_BODY()
public:
	AYcGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
private:
	/**
	 * 游戏体验管理组件
	 * 负责加载和管理当前游戏体验，包括Experience资产的加载、GameFeature插件的激活、
	 * 以及GameFeatureAction的执行。支持网络复制以同步客户端状态。
	 */
	UPROPERTY()
	TObjectPtr<UYcExperienceManagerComponent> ExperienceManagerComponent;
	
	// @TODO: 为GameState接入ASC组件，以便通过AbilitySystem来组织和控制游戏对局阶段的逻辑
};