// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayCueManager.h"
#include "YcGameplayCueManager.generated.h"

/**
 * UYcGameplayCueManager
 *
 * 项目特定的 GameplayCue 管理器
 * 
 * 功能说明：
 * - 管理 GameplayCue 的加载策略（前置加载、延迟加载、按需加载）
 * - 支持 GameFeature 插件动态添加/移除 GameplayCue 路径
 * - 实现 GameplayCue 的预加载和引用计数管理
 * - 提供调试工具用于查看已加载的 GameplayCue
 * 
 * 配置方法：
 * 在 DefaultGame.ini 的 [/Script/GameplayAbilities.AbilitySystemGlobals] 栏目下配置：
 * GlobalGameplayCueManagerClass=/Script/YiChenAbility.YcGameplayCueManager
 */
UCLASS()
class YICHENABILITY_API UYcGameplayCueManager : public UGameplayCueManager
{
	GENERATED_BODY()
	public:
	UYcGameplayCueManager(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 获取 YcGameplayCueManager 的单例实例 */
	static UYcGameplayCueManager* Get();

	//~UGameplayCueManager interface
	/** 当 GameplayCueManager 创建时调用，初始化延迟加载监听器 */
	virtual void OnCreated() override;
	
	/** 是否应该异步加载运行时对象库，根据加载模式决定 */
	virtual bool ShouldAsyncLoadRuntimeObjectLibraries() const override;
	
	/** 是否应该同步加载缺失的 GameplayCue（返回 false，使用异步加载） */
	virtual bool ShouldSyncLoadMissingGameplayCues() const override;
	
	/** 是否应该异步加载缺失的 GameplayCue（返回 true） */
	virtual bool ShouldAsyncLoadMissingGameplayCues() const override;
	//~End of UGameplayCueManager interface

	/** 
	 * 控制台命令：转储当前加载的所有 GameplayCue 信息
	 * 用法：Yc.DumpGameplayCues [Refs]
	 * 参数 Refs：可选，显示每个 Cue 的引用者
	 */
	static void DumpGameplayCues(const TArray<FString>& Args);

	/** 
	 * 加载必须始终保持加载状态的 GameplayCue
	 * 这些 Cue 通常是代码引用的或配置为 AlwaysLoaded 的
	 */
	void LoadAlwaysLoadedCues();

	/** 
	 * 刷新 GameplayCue 主资产的资产捆绑数据
	 * 当 GameplayCue 数量变化时（如 GameFeature 插件加载/卸载），需要调用此方法更新 AssetManager
	 */
	void RefreshGameplayCuePrimaryAsset();

private:
	/** 
	 * 当 GameplayTag 被加载时的回调函数
	 * 在任意线程中可能被调用，需要线程安全处理
	 */
	void OnGameplayTagLoaded(const FGameplayTag& Tag);
	
	/** 垃圾回收后的处理函数，处理在 GC 期间积累的待处理 Tag */
	void HandlePostGarbageCollect();
	
	/** 处理已加载的 GameplayTag 列表，触发相应 GameplayCue 的预加载 */
	void ProcessLoadedTags();
	
	/** 
	 * 处理需要预加载的 GameplayTag
	 * @param Tag 需要预加载的 GameplayTag
	 * @param OwningObject 引用该 Tag 的对象，用于引用计数管理（nullptr 表示 AlwaysLoaded）
	 */
	void ProcessTagToPreload(const FGameplayTag& Tag, UObject* OwningObject);
	
	/** 
	 * GameplayCue 预加载完成的回调函数
	 * @param Path 加载的资产路径
	 * @param OwningObject 引用该 Cue 的对象
	 * @param bAlwaysLoadedCue 是否为始终加载的 Cue
	 */
	void OnPreloadCueComplete(FSoftObjectPath Path, TWeakObjectPtr<UObject> OwningObject, bool bAlwaysLoadedCue);
	
	/** 
	 * 注册已预加载的 GameplayCue
	 * @param LoadedGameplayCueClass 已加载的 GameplayCue 类
	 * @param OwningObject 引用该 Cue 的对象（nullptr 表示 AlwaysLoaded）
	 */
	void RegisterPreloadedCue(UClass* LoadedGameplayCueClass, UObject* OwningObject);
	
	/** 
	 * 地图加载后的处理函数
	 * 清理无效的预加载 Cue 引用，释放不再需要的 Cue
	 */
	void HandlePostLoadMap(UWorld* NewWorld);
	
	/** 更新延迟加载的委托监听器，根据加载模式决定是否监听 */
	void UpdateDelayLoadDelegateListeners();
	
	/** 判断是否应该延迟加载 GameplayCue（客户端返回 true，专用服务器返回 false） */
	bool ShouldDelayLoadGameplayCues() const;

private:
	/** 待处理的已加载 GameplayTag 数据结构 */
	struct FLoadedGameplayTagToProcessData
	{
		FGameplayTag Tag;                    // 已加载的 GameplayTag
		TWeakObjectPtr<UObject> WeakOwner;   // 引用该 Tag 的对象（弱指针）

		FLoadedGameplayTagToProcessData() {}
		FLoadedGameplayTagToProcessData(const FGameplayTag& InTag, const TWeakObjectPtr<UObject>& InWeakOwner) 
			: Tag(InTag), WeakOwner(InWeakOwner) {}
	};

private:
	/** 
	 * 预加载的 GameplayCue 集合
	 * 这些 Cue 是由于被内容资产引用而预加载的，当引用计数归零时可能被卸载
	 */
	UPROPERTY(transient)
	TSet<TObjectPtr<UClass>> PreloadedCues;
	
	/** 
	 * 预加载 Cue 的引用者映射表
	 * Key: GameplayCue 类，Value: 引用该 Cue 的对象集合
	 * 用于跟踪哪些对象正在使用某个 Cue
	 */
	TMap<FObjectKey, TSet<FObjectKey>> PreloadedCueReferencers;

	/** 
	 * 始终加载的 GameplayCue 集合
	 * 这些 Cue 是代码引用的或显式配置为 AlwaysLoaded 的，永远不会被卸载
	 */
	UPROPERTY(transient)
	TSet<TObjectPtr<UClass>> AlwaysLoadedCues;

	/** 待处理的已加载 GameplayTag 列表（线程安全） */
	TArray<FLoadedGameplayTagToProcessData> LoadedGameplayTagsToProcess;
	
	/** 保护 LoadedGameplayTagsToProcess 的临界区 */
	FCriticalSection LoadedGameplayTagsToProcessCS;
	
	/** 标记是否需要在垃圾回收后处理已加载的 Tag */
	bool bProcessLoadedTagsAfterGC = false;
};