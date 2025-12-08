// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameFeaturesProjectPolicies.h"
#include "YcGameFeaturePolicy.generated.h"

/**
 * Manager to keep track of the state machines that bring a game feature plugin into memory and active
 * This class discovers plugins either that are built-in and distributed with the game or are reported externally (i.e. by a web service or other endpoint)
 * 允许项目实现特定的游戏功能插件规则。创建子类并在"项目设置 -> 游戏功能"中指定
 */
UCLASS(MinimalAPI, Config = Game)
class UYcGameFeaturePolicy : public UDefaultGameFeaturesProjectPolicies
{
	GENERATED_BODY()
public:
	YICHENGAMEPLAY_API static UYcGameFeaturePolicy& Get();
	
	UYcGameFeaturePolicy(const FObjectInitializer& ObjectInitializer);
	
	//~UGameFeaturesProjectPolicies interface
	/** 当游戏功能管理器初始化时调用, 执行策略类的初始化逻辑，如注册自定义处理器、加载配置文件等 */
	virtual void InitGameFeatureManager() override;
	/** 当游戏功能管理器关闭时调用, 执行清理逻辑，释放资源，保存状态等 */
	virtual void ShutdownGameFeatureManager() override;
	/** 确定插件是否允许被加载, 可实现基于许可证、用户权限、配置的功能开关 */
	virtual bool IsPluginAllowed(const FString& PluginURL, FString* OutReason) const override;
	/** 当游戏功能插件进入"加载"状态时，确定需要额外预加载的资源, 为插件指定必须预加载的资产减少运行时卡顿 */
	virtual TArray<FPrimaryAssetId> GetPreloadAssetListForGameFeature(const UGameFeatureData* GameFeatureToLoad, bool bIncludeLoadedAssets = false) const override;
	/** 返回用于GetPreloadAssetListForGameFeature()返回资产的捆绑状态， 指定资产的加载分组策略, 控制加载顺序和优先级, 返回资产捆绑状态名称数组 */
	virtual const TArray<FName> GetPreloadBundleStateForGameFeature() const override;
	/** 确定应作为客户端、服务器还是两者来加载数据, 专用服务器不需要客户端资源 , 客户端可能不需要服务器专用数据*/
	virtual void GetGameFeatureLoadingMode(bool& bLoadClientData, bool& bLoadServerData) const override;
	/** 判断插件是否应包含在最终构建中, 用于优化打包大小, 例如排除不需要的平台插件 */
	virtual bool WillPluginBeCooked(const FString& PluginFilename, const FGameFeaturePluginDetails& PluginDetails) const override;
	/** 是否应读取插件的 .uplugin 详情文件, 可做性能优化, 例如避免读取不必要的 .uplugin 文件 */
	virtual bool ShouldReadPluginDetails(const FString& PluginDescriptorFilename) const override;
	//~End of UGameFeaturesProjectPolicies interface

private:
	UPROPERTY(Transient)
	TArray<TObjectPtr<UObject>> Observers;
};
