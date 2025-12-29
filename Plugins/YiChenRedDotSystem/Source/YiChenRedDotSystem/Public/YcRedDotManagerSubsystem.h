// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "YcRedDotTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "YcRedDotManagerSubsystem.generated.h"

class UYcRedDotConditionBase;
class UYcRedDotManagerSubsystem;

/**
 * 红点状态变化监听者的 Handle，用于管理监听者的生命周期
 * 通过 RegisterRedDotStateChangedListener() 获取该 Handle
 * 调用 Unregister() 可注销该监听者
 */
USTRUCT(BlueprintType)
struct YICHENREDDOTSYSTEM_API FYcRedDotStateChangedListenerHandle
{
	GENERATED_BODY()

	FYcRedDotStateChangedListenerHandle() {}

	/**
	 * 注销该监听者，停止接收红点状态变化通知
	 * @note 调用后该 Handle 会变为无效状态（ID 被重置为 0）
	 */
	void Unregister();

	/**
	 * 检查该 Handle 是否有效
	 * @return 如果 Handle 有效，返回 true；否则返回 false
	 */
	bool IsValid() const { return ID != 0; }

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<UYcRedDotManagerSubsystem> Subsystem;

	UPROPERTY(Transient)
	FGameplayTag RedDotTag;

	UPROPERTY(Transient)
	int32 ID = 0;

	friend UYcRedDotManagerSubsystem;

	FYcRedDotStateChangedListenerHandle(UYcRedDotManagerSubsystem* InSubsystem, FGameplayTag InRedDot, int32 InID) : Subsystem(InSubsystem), RedDotTag(InRedDot), ID(InID) {}
};

/**
 * 红点数据提供者的 Handle，用于管理提供者的一些回调绑定生命周期
 * 通过 RegisterRedDotDataProvider() 获取该 Handle
 * 调用 Unregister() 可注销
 */
USTRUCT(BlueprintType)
struct YICHENREDDOTSYSTEM_API FYcRedDotDataProviderHandle
{
	GENERATED_BODY()

	FYcRedDotDataProviderHandle() {}

	/**
	 * 注销停止接收红点清除通知
	 * @note 调用后该 Handle 会变为无效状态（ID 被重置为 0）
	 */
	void Unregister();

	/**
	 * 检查该 Handle 是否有效
	 * @return 如果 Handle 有效，返回 true；否则返回 false
	 */
	bool IsValid() const { return ID != 0; }

private:
	UPROPERTY(Transient)
	TWeakObjectPtr<UYcRedDotManagerSubsystem> Subsystem;

	UPROPERTY(Transient)
	FGameplayTag RedDotTag;

	UPROPERTY(Transient)
	int32 ID = 0;

	friend UYcRedDotManagerSubsystem;

	FYcRedDotDataProviderHandle(UYcRedDotManagerSubsystem* InSubsystem, FGameplayTag InRedDotTag, int32 InId) : Subsystem(InSubsystem), RedDotTag(InRedDotTag), ID(InId) {}
};


/**
 * 红点系统管理子系统, 提供红点系统的交互函数
 */
UCLASS(BlueprintType, meta = (DisplayName = "Yc Red Dot Manager"))
class YICHENREDDOTSYSTEM_API UYcRedDotManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
public:
	/**
	 * 获取指定世界上下文对象关联的游戏实例的红点管理子系统
	 * @param WorldContextObject 世界上下文对象，用于获取关联的世界和游戏实例
	 * @return 红点管理子系统的引用。如果子系统不存在，会触发断言
	 * @note 该函数会进行断言检查，确保子系统存在。如果不存在会导致程序崩溃
	 */
	static UYcRedDotManagerSubsystem& Get(const UObject* WorldContextObject);

	/**
	 * 检查指定世界中是否存在有效的红点管理子系统
	 * @param WorldContextObject 世界上下文对象，用于获取关联的世界和游戏实例
	 * @return 如果子系统存在且有效，返回 true；否则返回 false
	 * @note 该函数不会触发断言，可安全用于检查子系统是否存在
	 */
	static bool HasInstance(const UObject* WorldContextObject);
	
	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	//~ End USubsystem Interface
	
	/**
	 * 注册红点标签，初始化其状态数据
	 * @param RedDotTag 要注册的红点标签。如果标签无效，会输出警告日志
	 * @note 该函数会在 RedDotStates 中创建该标签的条目，初始化其 FRedDotInfo 数据
	 * @note 通常在游戏启动时调用，预先注册所有可能的红点标签
	 */
	UFUNCTION(BlueprintCallable, Category = "RedDot|Management")
	void RegisterRedDotTag(FGameplayTag RedDotTag);
	
	/**
	 * 增加或减少红点数量，自动触发向上穿透更新
	 * @param RedDotTag 目标红点标签。如果标签无效，会输出警告日志
	 * @param Count 变化量。正数表示增加，负数表示减少。默认为 0
	 * @note 如果红点数量为 0 且 Count <= 0，则不会触发任何更新
	 * @note 红点数量不会低于 0，会被自动限制在 [0, +∞) 范围内
	 * @note 该函数会自动计算 Delta（变化量）并触发 BroadcastRedDotChangedForward，实现数据穿透
	 */
	UFUNCTION(BlueprintCallable, Category = "RedDot|Management")
	void AddRedDotCount(FGameplayTag RedDotTag, int32 Count = 0);
	
	/**
	 * 获取指定红点标签的信息
	 * @param RedDotTag 要查询的红点标签
	 * @param InOutInfo 输出参数，存储查询到的红点信息。如果标签不存在，该参数不会被修改
	 * @return 如果找到该红点标签，返回 true；否则返回 false
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RedDot|Query")
	bool GetRedDotInfo(const FGameplayTag& RedDotTag, FRedDotInfo& InOutInfo);
	
	/**
	 * 查询红点是否激活（Count > 0）
	 * @param RedDotTag 要查询的红点标签
	 * @return 如果红点的 Count > 0，返回 true；否则返回 false
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RedDot|Query")
	bool IsRedDotActive(const FGameplayTag& RedDotTag) const;
	
	/**
	 * 获取所有激活的红点标签（Count > 0 的标签）
	 * @return 包含所有激活红点标签的数组
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RedDot|Query")
	TArray<FGameplayTag> GetAllActiveRedDotTags() const;
	
	/**
	 * 获取所有红点的信息
	 * @param InOutRedDots 输出参数，存储所有红点的信息数组
	 * @note 该函数会遍历 RedDotStates 中的所有条目，包括 Count 为 0 的红点
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "RedDot|Query")
	void GetAllRedDotInfos(TArray<FRedDotInfo>& InOutRedDots);
	
	/**
	 * 清除指定父标签分支下的所有红点数据
	 * @param ParentTag 父标签。该函数会清除该标签及其所有子标签的红点数据
	 * @note 该函数会：
	 *   1. 将父标签的 Count 设置为 0，并触发向上穿透更新
	 *   2. 调用 BroadcastRedDotCleared(ParentTag) 通知红点标签所有提供者
	 *   3. 将所有子标签的 Count 设置为 0，并调用 BroadcastRedDotCleared 通知相关的提供者让它们清除红点数据状态
	 * @note 仅会处理已注册在 RedDotStates 中的标签
	 */
	UFUNCTION(BlueprintCallable, Category = "RedDot|Batch")
	void ClearRedDotStateInBranch(const FGameplayTag& ParentTag);
	
	/**
	 * 注册红点状态变化监听者
	 * @param RedDotTag 要监听的红点标签
	 * @param Callback 状态变化时的回调函数，参数为 (FGameplayTag, const FRedDotInfo*)
	 * @param MatchType 标签匹配规则。ExactMatch 仅接收该标签的变化；PartialMatch 接收该标签及其子标签的变化
	 * @param bUnregisterOnWorldDestroyed 世界销毁时是否自动注销该监听
	 * @return 监听 Handle，用于后续注销监听。调用 Handle.Unregister() 可手动注销
	 * @note 回调函数会在 BroadcastRedDotChangedForward 中被调用
	 * @note 如果 MatchType 为 PartialMatch，当父标签变化时，子标签的监听者也会收到通知
	 */
	FYcRedDotStateChangedListenerHandle RegisterRedDotStateChangedListener(
		FGameplayTag RedDotTag,
		TFunction<void(FGameplayTag, const FRedDotInfo*)>&& Callback,
		EYcRedDotTagMatch MatchType,
		bool bUnregisterOnWorldDestroyed);
	
	/**
	 * 注册红点提供者, 包含了清理回调函数, 在红点被清除时触发回调
	 * @param RedDotTag 提供者要提供数据的目标红点标签
	 * @param Callback 红点被清除时的回调函数，无参数
	 * @return 提供者 Handle，用于后续注销。调用 Handle.Unregister() 可手动注销
	 * @note 回调函数会在 BroadcastRedDotCleared 中被调用
	 */
	FYcRedDotDataProviderHandle RegisterRedDotDataProvider(FGameplayTag RedDotTag, TFunction<void()>&& Callback);

protected:
	/** 获取或创建红点信息 */
	FRedDotInfo& GetOrCreateRedDotInfo(const FGameplayTag& RedDotTag);
	
	/** 更新红点状态（内部使用） */
	void UpdateRedDotState(const FGameplayTag& RedDotTag, const FRedDotInfo& NewInfo);
	
	/**
	 * 查找指定父标签下的所有子标签
	 * @param ParentTag 父标签
	 * @return 该父标签下的所有子标签数组
	 * @note 使用 TagHierarchyCache 进行缓存以加速查询
	 * @note 当新标签被注册时，会自动清理相关的缓存条目
	 */
	TArray<FGameplayTag> FindAllChildTags(const FGameplayTag& ParentTag) const;
	
	/**
	 * 清理指定标签的所有相关缓存
	 * 当新标签被注册时调用，清理所有可能受影响的父标签缓存
	 * @param NewTag 新注册的标签
	 * @note 该函数会遍历 TagHierarchyCache，移除所有包含 NewTag 作为子标签的缓存条目
	 * @note 例如：注册 A.B.C.D.E 时，会清理 A、A.B、A.B.C、A.B.C.D 的缓存
	 */
	void InvalidateTagHierarchyCache(const FGameplayTag& NewTag);
	
	/** 向前(向上)穿透广播红点变化事件, 可以实现正确的红点数据穿透更新  */
	void BroadcastRedDotChangedForward(FGameplayTag RedDotTag, const FRedDotInfo* RedDotInfo);
	
	/** 单一节点广播红点变化事件, 用于清理子节点红点数据 */
	void BroadcastRedDotChangedSingle(FGameplayTag RedDotTag, const FRedDotInfo* RedDotInfo);
	
	/** 清理目标红点, 内部调用所有相关提供者的清理事件回调函数, 以实现通知提供者清理数据 */
	void BroadcastRedDotCleared(FGameplayTag RedDotTag);
	
	/** 注销红点数据状态监听者 */
	void UnregisterStateChangedListener(FYcRedDotStateChangedListenerHandle Handle);
	
	/** 注销红点提供者 */
	void UnregisterRedDotDataProvider(FYcRedDotDataProviderHandle Handle);

private:
	/**
	 * 红点状态数据存储容器
	 * 存储所有已注册红点标签的状态信息
	 * Key: 红点标签
	 * Value: 红点信息（Count、Type、Priority 等）
	 * @note 通过 RegisterRedDotTag() 和 AddRedDotCount() 进行修改
	 * @note 在 Deinitialize() 时会被清空
	 */
	UPROPERTY()
	TMap<FGameplayTag, FRedDotInfo> RedDotStates;
    
	/**
	 * 标签层级关系缓存，用于加速子标签查询
	 * Key: 父标签
	 * Value: 该父标签下的所有子标签数组
	 * @note 在 FindAllChildTags() 中使用，避免重复遍历 RedDotStates
	 * @note 当新标签被注册时，InvalidateTagHierarchyCache() 会自动清理所有相关的缓存条目
	 * @note 缓存失效机制确保了动态注册的新标签不会被遗漏
	 */
	TMap<FGameplayTag, TArray<FGameplayTag>> TagHierarchyCache;
	
	/**
	 * 红点监听者和数据提供者包装
	 * 存储一个红点标签所有的状态变化监听者和数据提供者
	 */
	struct FYcRedDotListenerList
	{
		/** 该标签的所有状态变化监听者 */
		TArray<FYcRedDotStateChangedListenerData> Listeners;
		
		/** 该标签的所有数据提供者（这里主要是用于清除时触发提供者清除回调, 以便提供者能清理自身数据状态） */
		TArray<FYcRedDotDataProviderData> DataProviders;
		
		/** Handle ID 计数器，用于生成唯一的 Handle ID */
		int32 HandleID = 0;
	};

	/**
	 * 监听者和提供者监听映射表
	 * 存储每个红点标签对应的监听者和提供者列表
	 * Key: 红点标签
	 * Value: 该标签的监听者和提供者列表（FYcRedDotListenerList）
	 * @note 通过 RegisterRedDotStateChangedListener() 和 RegisterRedDotDataProvider() 进行修改
	 * @note 在 BroadcastRedDotChangedForward()、BroadcastRedDotChangedSingle() 和 BroadcastRedDotCleared() 中查询
	 * @note 当监听者或提供者列表为空时，会从 ListenerMap 中移除该标签条目
	 */
	TMap<FGameplayTag, FYcRedDotListenerList> ListenerMap;
	
	friend FYcRedDotStateChangedListenerHandle;
	friend FYcRedDotDataProviderHandle;
};
