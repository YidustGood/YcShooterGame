// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameFeatureStateChangeObserver.h"
#include "UObject/Object.h"
#include "YcGameFeature_AddGameplayCuePaths.generated.h"

/**
 * UYcGameFeature_AddGameplayCuePaths
 * 
 * GameFeature 状态变化观察者：监听 GameFeature 插件的注册/注销事件
 * 
 * 功能说明：
 * - 实现 IGameFeatureStateChangeObserver 接口
 * - 在 GameFeature 插件注册时，动态添加 GameplayCue 扫描路径
 * - 在 GameFeature 插件注销时，动态移除 GameplayCue 扫描路径
 * - 触发 GameplayCueManager 重新初始化运行时对象库
 * - 更新 AssetManager 的资产捆绑数据
 * 
 * 工作流程：
 * 1. 由 YcGameFeaturePolicy 在初始化时创建并注册到 GameFeaturesSubsystem
 * 2. 监听所有 GameFeature 插件的状态变化
 * 3. 当插件注册时，遍历其 GameFeatureData 中的 YcGameFeatureAction_AddGameplayCuePath
 * 4. 将配置的路径添加到 GameplayCueManager
 * 5. 重建运行时库并更新资产捆绑
 * 
 * @see YcGameFeaturePolicy::InitGameFeatureManager()
 * @see YcGameFeatureAction_AddGameplayCuePath
 */
UCLASS()
class YICHENGAMEPLAY_API UYcGameFeature_AddGameplayCuePaths : public UObject, public IGameFeatureStateChangeObserver
{
	GENERATED_BODY()
public:
	/**
	 * 当 GameFeature 插件注册时调用
	 * 此时插件内容路径已挂载，可以安全地添加 GameplayCue 扫描路径
	 * 
	 * @param GameFeatureData 插件的 GameFeatureData 资产
	 * @param PluginName 插件名称（如 "YcShooterExample"）
	 * @param PluginURL 插件的 URL
	 */
	virtual void OnGameFeatureRegistering(const UGameFeatureData* GameFeatureData, const FString& PluginName, const FString& PluginURL) override;
	
	/**
	 * 当 GameFeature 插件注销时调用
	 * 需要移除之前添加的 GameplayCue 扫描路径，避免访问已卸载的内容
	 * 
	 * @param GameFeatureData 插件的 GameFeatureData 资产
	 * @param PluginName 插件名称
	 * @param PluginURL 插件的 URL
	 */
	virtual void OnGameFeatureUnregistering(const UGameFeatureData* GameFeatureData, const FString& PluginName, const FString& PluginURL) override;
};
