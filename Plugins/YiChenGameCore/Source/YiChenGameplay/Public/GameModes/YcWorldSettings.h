// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameFramework/WorldSettings.h"
#include "YcWorldSettings.generated.h"

class UYcExperienceDefinition;
class UGameFeatureAction;
class UYcExperienceActionSet;

/**
 * 世界设置类
 * 
 * 默认世界设置对象，主要用于配置在此地图上使用的默认游戏体验。
 * 支持在编辑器中配置Experience、GameFeatureAction和ActionSet，
 * 以便在地图加载时自动应用这些配置。
 */
UCLASS()
class YICHENGAMEPLAY_API AYcWorldSettings : public AWorldSettings
{
	GENERATED_BODY()
public:
	AYcWorldSettings(const FObjectInitializer& ObjectInitializer);
	
	/**
	 * 获取指定世界对象的YcWorldSettings实例
	 * @param WorldContent 世界对象
	 * @return 对应的YcWorldSettings实例，如果不存在则返回nullptr
	 */
	static AYcWorldSettings* GetYcWorldSettings(const UObject* WorldContent);
	
#if WITH_EDITOR
	/**
	 * 地图检查函数
	 * 从Map_Check内部调用，允许此Actor检查自身的潜在错误，
	 * 并将错误注册到地图检查对话框中
	 */
	virtual void CheckForErrors() override;
#endif
	
#if WITH_EDITORONLY_DATA
	/**
	 * 强制Standalone网络模式标记
	 * 标记当前世界是否为UI前端或其他独立体验。
	 * 当设置为true时，在编辑器中点击Play时会强制使用Standalone模式，不会进行联机。
	 * 实现在项目的Editor模块->EditorEngine->PreCreatePIEInstances()方法中
	 */
	UPROPERTY(EditDefaultsOnly, Category=PIE)
	bool ForceStandaloneNetMode = false;
#endif
	
	/**
	 * 获取此地图的默认游戏体验
	 * 返回服务器打开此地图时使用的默认体验，如果未被用户设置的体验覆盖
	 * @return 默认游戏体验的主资产ID
	 */
	FPrimaryAssetId GetDefaultGameplayExperience() const;
	
protected:
	/**
	 * 默认游戏体验资产
	 * 当服务器打开此地图时使用的默认体验。如果未被用户设置的体验覆盖，则使用此体验。
	 * 类似于地图的GameMode设置。
	 */
	UPROPERTY(EditDefaultsOnly, Category=GameMode)
	TSoftClassPtr<UYcExperienceDefinition> DefaultGameplayExperience;
	
public:
	/**
	 * 是否启用GameplayExperience功能
	 * 控制当前World是否启用GameplayExperience功能。设置为false时，
	 * GameMode不会尝试加载Experience。
	 */
	UPROPERTY(EditDefaultsOnly, Category=GameMode)
	bool bEnableGameplayExperience = true;
	
	/**
	 * 直接配置在WorldSettings上的GameFeatureAction列表
	 * 用于在地图上快速添加功能以便测试。对于正式的游戏内容制作，
	 * 应该采用UYcExperienceDefinition+UYcExperienceActionSet的标准流程。
	 * 这些Action会在Experience加载时执行。
	 */
	UPROPERTY(EditDefaultsOnly, Instanced, Category=GameMode)
	TArray<TObjectPtr<UGameFeatureAction>> Actions;

	/**
	 * 直接配置在WorldSettings上的ActionSet列表
	 * 用于在地图上快速添加功能集合以便测试。对于正式的游戏内容制作，
	 * 应该采用UYcExperienceDefinition+UYcExperienceActionSet的标准流程。
	 * 这些ActionSet会在Experience加载时合并到Experience中。
	 */
	UPROPERTY(EditDefaultsOnly, Category=GameMode)
	TArray<TObjectPtr<UYcExperienceActionSet>> ActionSets;
};
