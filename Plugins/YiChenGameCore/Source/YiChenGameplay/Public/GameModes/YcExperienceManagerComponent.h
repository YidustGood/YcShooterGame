// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "LoadingProcessInterface.h"
#include "Components/GameStateComponent.h"
#include "YcExperienceManagerComponent.generated.h"

#define UE_API YICHENGAMEPLAY_API

namespace UE::GameFeatures
{
	struct FResult;
}

class UYcExperienceDefinition;

/**
 * 游戏体验加载完成委托
 * 当游戏体验完全加载并激活后触发，广播参数为加载的游戏体验定义对象
 */
DECLARE_MULTICAST_DELEGATE_OneParam(FOnYcExperienceLoaded, const UYcExperienceDefinition* /*Experience*/);

/**
 * 游戏体验加载状态枚举
 * 定义Experience从初始化到完全加载的各个阶段
 */
enum class EYcExperienceLoadState
{
	// 未加载状态，Experience还未开始加载
	Unloaded,
	
	// 加载中状态，正在加载Experience实例对象以及相关资产Assets
	Loading,
	
	// 加载GameFeature插件中，在OnExperienceLoadComplete()中遍历Experience中配置的GameFeature进行加载
	LoadingGameFeatures,
	
	// 模拟加载延迟状态，用于测试长加载时间的场景
	// 延迟时间可通过控制台命令 Yc.chaos.ExperienceDelayLoad.MinSecs 设置
	LoadingChaosTestingDelay,
	
	// 执行GameFeatureAction中，正在执行UYcExperienceDefinition中配置的Actions
	// 这一阶段会进行很多自定义的业务逻辑工作，如添加UI、输入、组件等
	ExecutingActions,
	
	// 加载完成状态，Experience已完全加载并激活
	Loaded,
	
	// 反激活/卸载中状态，Experience正在被禁用或卸载
	Deactivating
};

/**
 * 游戏体验管理组件，负责控制加载/网络复制/卸载关卡的Experience，因为需要网络复制数据和通知客户端, 所以由GameState持有该组件
 */
UCLASS(MinimalAPI)
class UYcExperienceManagerComponent final : public UGameStateComponent, public ILoadingProcessInterface
{
	GENERATED_BODY()

public:
	UE_API UYcExperienceManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	UE_API virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	//~UActorComponent interface
	UE_API virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of UActorComponent interface

	//~ILoadingProcessInterface interface
	UE_API virtual bool ShouldShowLoadingScreen(FString& OutReason) const override;
	//~End of ILoadingProcessInterface

	/**
	 * 设置当前游戏体验
	 * 通过FPrimaryAssetId加载指定的Experience，如果有效则开启加载流程。
	 * 仅在服务器/权威端调用，客户端通过网络复制自动同步。
	 * 
	 * 加载流程：
	 * 1. 通过ID加载ExperienceClass类对象，获取Experience的CDO对象
	 * 2. 调用StartExperienceLoad开始加载Experience中配置的资产
	 * 3. 加载完成后加载GameFeature插件
	 * 4. 执行GameFeatureAction
	 * 
	 * @param ExperienceId 要加载的Experience主资产ID
	 */
	UE_API void SetCurrentExperience(const FPrimaryAssetId& ExperienceId);

	/**
	 * 注册高优先级的Experience加载完成委托
	 * 确保委托在Experience加载完成时被调用，且在其他优先级的委托之前调用。
	 * 如果Experience已经加载，则立即调用委托。
	 * @param Delegate 要注册的委托
	 */
	UE_API void CallOrRegister_OnExperienceLoaded_HighPriority(FOnYcExperienceLoaded::FDelegate&& Delegate);

	/**
	 * 注册中等优先级的Experience加载完成委托
	 * 确保委托在Experience加载完成时被调用。
	 * 如果Experience已经加载，则立即调用委托。
	 * @param Delegate 要注册的委托
	 */
	UE_API void CallOrRegister_OnExperienceLoaded(FOnYcExperienceLoaded::FDelegate&& Delegate);

	/**
	 * 注册低优先级的Experience加载完成委托
	 * 确保委托在Experience加载完成时被调用，且在其他优先级的委托之后调用。
	 * 如果Experience已经加载，则立即调用委托。
	 * @param Delegate 要注册的委托
	 */
	UE_API void CallOrRegister_OnExperienceLoaded_LowPriority(FOnYcExperienceLoaded::FDelegate&& Delegate);

	/**
	 * 获取当前加载的Experience定义对象
	 * 仅在Experience完全加载后调用此函数，否则会触发断言。
	 * @return 当前加载的Experience定义对象
	 */
	UE_API const UYcExperienceDefinition* GetCurrentExperienceChecked() const;

	/**
	 * 检查Experience是否已完全加载
	 * @return 如果Experience已完全加载返回true，否则返回false
	 */
	UE_API bool IsExperienceLoaded() const;
	
private:
	/** CurrentExperience的复制通知函数, CurrentExperience复制到客户端后会自动调用这个函数以在客户端开始Experience的加载 */
	UFUNCTION()
	void OnRep_CurrentExperience();

	// 开始加载体验的相关资产Assets，服务器上在SetCurrentExperience()函数中调用，客户端则由OnRep_CurrentExperience()属性同步通知函数调用
	/** 第二步： Experience对象已经加载完毕, 通过它加载其中配置的相关资产, 完成后触发OnExperienceLoadComplete开始处理Experience中配置的内容, 本阶段LoadState = ESCExperienceLoadState::Loading */
	void StartExperienceLoad();		
	
	// 当体验的资产Assets加载完成后的回调函数(此时为Loading结束阶段)，该函数的功能是加载CurrentExperience对象中的GameFeature(),为LoadingGameFeatures状态
	// 第三步： Experience对象及其依赖的资产已加载完毕, 开始处理业务逻辑, 例如开启Experience中配置的所有GameFeaturePlugin, 本阶段LoadState = ESCExperienceLoadState::LoadingGameFeatures */
	void OnExperienceLoadComplete();	
	
	// 当体验完全加载完成后的回调函数
	/** 第四步：配置的GameFeaturePlugin也开启了, 限制开始执行配置中的GameFeatureAction, 这一步会对游戏做很多操作, 例如添加UI、添加输入、添加组件等一系列游戏业务逻辑
	 * 我们可以通过实现自定义的GameFeatureAction来实现各种不同的行为, 将这些行为(GameFeatureAction)按照GameFeaturePlugin进行组织, 然后配置到Experience中实现基于数据驱动的游戏业务功能组装
	 * 本阶段LoadState = ESCExperienceLoadState::ExecutingActions; 过度到 4. LoadState = ESCExperienceLoadState::Loaded; */
	void OnExperienceFullLoadCompleted();

	// 当体验的单个GameFeaturePlugin加载完成后的回调函数进行计数器减1，全部加载完成后调用OnExperienceFullLoadCompleted()
	void OnGameFeaturePluginLoadComplete(const UE::GameFeatures::FResult& Result);
	
	// 当单个Action反激活完成
	void OnActionDeactivationCompleted();	// 5.完全加载完成，处理后续操作
	// 当所有Action反激活完成后的回调函数
	void OnAllActionsDeactivated();
	
	UPROPERTY(ReplicatedUsing=OnRep_CurrentExperience)
	TObjectPtr<const UYcExperienceDefinition> CurrentExperience;

	EYcExperienceLoadState LoadState = EYcExperienceLoadState::Unloaded;

	// 当前体验正在加载中的GameFeaturePlugins数量，加载完成一个GFP之后由OnGameFeaturePluginLoadComplete()回调函数进行计量减1
	int32 NumGameFeaturePluginsLoading = 0;
	int32 NumGameFeatureActionsLoading = 0;
	TArray<FString> GameFeaturePluginURLs;

	int32 NumObservedPausers = 0;
	int32 NumExpectedPausers = 0;

	/**
	 * Delegate called when the experience has finished loading just before others
	 * (e.g., subsystems that set up for regular gameplay)
	 */
	FOnYcExperienceLoaded OnExperienceLoaded_HighPriority;

	/** Delegate called when the experience has finished loading */
	FOnYcExperienceLoaded OnExperienceLoaded;

	/** Delegate called when the experience has finished loading */
	FOnYcExperienceLoaded OnExperienceLoaded_LowPriority;
};

#undef UE_API