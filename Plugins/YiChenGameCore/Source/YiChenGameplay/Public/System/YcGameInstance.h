// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonGameInstance.h"
#include "YcGameInstance.generated.h"

class AYcPlayerController;

/**
 * 插件的游戏实例基类，提供游戏全局状态管理和核心系统访问
 * 
 * 该游戏实例类继承自UCommonGameInstance，是YiChen游戏框架的核心组件之一。
 * 负责管理游戏全局状态、系统初始化和游戏流程控制。
 * 
 * 主要功能：
 * - 提供对主玩家控制器的便捷访问
 * - 管理游戏生命周期（初始化、关闭）
 * - 作为全局系统和子系统的访问入口
 * 
 * 使用要求：
 * - 使用YiChenCore插件的项目必须使用YcGameInstance或其子类作为游戏实例
 * - 项目设置中需设置游戏视口客户端类为CommonGameViewportClient或其子类
   - 项目设置中需设置本地玩家类为YcLocalPlayer或其子类(继承自CommonLocalPlayer)
 * - 可以在项目设置中配置为游戏实例类
 */
UCLASS(Config = Game)
class YICHENGAMEPLAY_API UYcGameInstance : public UCommonGameInstance
{
	GENERATED_BODY()

public:
	AYcPlayerController* GetPrimaryPlayerController() const;
	
protected:
    virtual void Init() override;
    virtual void Shutdown() override;
};
