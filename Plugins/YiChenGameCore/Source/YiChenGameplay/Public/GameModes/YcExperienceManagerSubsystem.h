// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "YcExperienceManagerSubsystem.generated.h"

/**
 * 游戏体验管理子系统
 * 
 * 引擎级别的子系统，主要用于在编辑器中管理多个PIE会话之间的GameFeature插件激活。
 * 跟踪GameFeature插件的激活次数，支持先入后出（LIFO）的激活管理策略，
 * 防止在多个PIE会话中重复激活或过早卸载插件。
 */
UCLASS(MinimalAPI)
class UYcExperienceManagerSubsystem : public UEngineSubsystem
{
	GENERATED_BODY()
public:
#if WITH_EDITOR
	/**
	 * 编辑器中开始Play In Editor时的回调函数
	 * 用于初始化PIE会话的相关状态
	 */
	YICHENGAMEPLAY_API void OnPlayInEditorBegun();

	/**
	 * 通知GameFeature插件已激活
	 * 增加指定插件的激活计数，支持多个会话同时激活同一个插件
	 * @param PluginURL GameFeature插件的URL
	 */
	static void NotifyOfPluginActivation(const FString PluginURL);

	/**
	 * 请求卸载GameFeature插件
	 * 减少指定插件的激活计数，仅当计数为0时才真正卸载插件
	 * @param PluginURL GameFeature插件的URL
	 * @return 如果插件被成功卸载返回true，否则返回false
	 */
	static bool RequestToDeactivatePlugin(const FString PluginURL);
#else
	static void NotifyOfPluginActivation(const FString PluginURL) {}
	static bool RequestToDeactivatePlugin(const FString PluginURL) { return true; }
#endif

private:
	/**
	 * GameFeature插件激活计数映射表
	 * 记录每个插件被激活的次数。支持先入后出（LIFO）的激活管理策略，
	 * 允许在PIE期间多个会话同时使用同一个插件，只有当所有会话都卸载后才真正卸载插件。
	 * 
	 * 工作流程：
	 * - NotifyOfPluginActivation(): 计数++
	 * - RequestToDeactivatePlugin(): 计数--，当计数为0时卸载插件
	 */
	TMap<FString, int32> GameFeaturePluginRequestCountMap;
};
