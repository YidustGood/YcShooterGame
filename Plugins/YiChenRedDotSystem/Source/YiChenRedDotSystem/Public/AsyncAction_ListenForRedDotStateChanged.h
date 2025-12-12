// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YcRedDotManagerSubsystem.h"
#include "YcRedDotTypes.h"
#include "Engine/CancellableAsyncAction.h"
#include "AsyncAction_ListenForRedDotStateChanged.generated.h"


/** 红点状态变化事件委托，参数为 (FGameplayTag, FRedDotInfo) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAsyncRedDotStateChangedDelegate,  FGameplayTag, RedDotTag, FRedDotInfo, RedDotInfo);

/**
 * 异步监听红点状态变化的蓝图节点
 * 提供蓝图友好的异步接口，自动管理监听者的生命周期
 * 继承自 UBlueprintAsyncActionBase，支持蓝图异步调用
 */
UCLASS()
class YICHENREDDOTSYSTEM_API UAsyncAction_ListenForRedDotStateChanged : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	/**
	 * 创建异步监听任务，监听指定红点标签的状态变化
	 * @param WorldContextObject 世界上下文对象，用于获取关联的世界和游戏实例
	 * @param RedDotTag 要监听的红点标签
	 * @param MatchType 标签匹配规则。默认为 PartialMatch（接收该标签及其子标签的变化）
	 * @param bUnregisterOnWorldDestroyed 世界销毁时是否自动注销该监听。默认为 true
	 * @return 异步任务对象。如果无法获取世界，返回 nullptr
	 * @note 该函数会自动注册到游戏实例，并在 Activate() 时开始监听
	 * @note 在蓝图中调用此函数后，需要绑定 OnRedDotStateChanged 事件来接收通知
	 */
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true"))
	static UAsyncAction_ListenForRedDotStateChanged* ListenForRedDotStateChanged(
		UObject* WorldContextObject,
		FGameplayTag RedDotTag,
		EYcRedDotTagMatch MatchType = EYcRedDotTagMatch::PartialMatch,
		bool bUnregisterOnWorldDestroyed = true
	);
	
	/**
	 * 激活异步任务，开始监听红点状态变化
	 * 该函数由蓝图异步框架自动调用，不需要手动调用
	 */
	virtual void Activate() override;
	
	/**
	 * 准备销毁异步任务，注销监听者
	 * 该函数由蓝图异步框架自动调用，不需要手动调用
	 */
	virtual void SetReadyToDestroy() override;

	/**
	 * 红点状态变化事件
	 * 当监听的红点状态发生变化时，该事件会被广播
	 * 参数：
	 *   - RedDotTag: 发生变化的红点标签
	 *   - RedDotInfo: 红点的最新信息
	 */
	UPROPERTY(BlueprintAssignable)
	FAsyncRedDotStateChangedDelegate OnRedDotStateChanged;
	
private:
	void HandleRedDotStateChanged(FGameplayTag Channel, const FRedDotInfo* RedDotInfo);

	TWeakObjectPtr<UWorld> WorldPtr;
	FGameplayTag RedDotTagRegister;
	EYcRedDotTagMatch RedDotTagMatchType = EYcRedDotTagMatch::PartialMatch;
	bool bUnregisterOnWorldDestroyed;
	
	FYcRedDotStateChangedListenerHandle ListenerHandle;
};