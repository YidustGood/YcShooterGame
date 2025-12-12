// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YcRedDotManagerSubsystem.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "AsyncAction_ListenForRedDotCleared.generated.h"

/**
 * 异步监听红点清除事件的蓝图节点
 * 提供蓝图友好的异步接口，监听指定红点标签被清除时的事件
 * 继承自 UBlueprintAsyncActionBase，支持蓝图异步调用
 */
UCLASS()
class YICHENREDDOTSYSTEM_API UAsyncAction_ListenForRedDotCleared : public UBlueprintAsyncActionBase
{
	GENERATED_BODY()
public:
	/**
	 * 创建异步监听任务，监听指定红点标签被清除的事件
	 * @param WorldContextObject 世界上下文对象，用于获取关联的世界和游戏实例
	 * @param RedDotTag 要监听的红点标签。当该标签被 ClearRedDotStateInBranch() 清除时，会触发回调
	 * @return 异步任务对象。如果无法获取世界，返回 nullptr
	 * @note 该函数会自动注册到游戏实例，并在 Activate() 时开始监听
	 * @note 在蓝图中调用此函数后，需要绑定 OnRedDotCleared 事件来接收通知
	 * @note 该监听通过 RegisterRelier() 实现，仅在红点被清除时触发，不监听数量变化
	 */
	UFUNCTION(BlueprintCallable, meta = (WorldContext = "WorldContextObject", BlueprintInternalUseOnly = "true"))
	static UAsyncAction_ListenForRedDotCleared* ListenForRedDotCleared(
		UObject* WorldContextObject,
		FGameplayTag RedDotTag
	);
	
	/**
	 * 激活异步任务，开始监听红点清除事件
	 * 该函数由蓝图异步框架自动调用，不需要手动调用
	 */
	virtual void Activate() override;
	
	/**
	 * 准备销毁异步任务，注销依赖者
	 * 该函数由蓝图异步框架自动调用，不需要手动调用
	 */
	virtual void SetReadyToDestroy() override;
	
	/** 红点清除事件委托，无参数 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FRedDotCleared);
	
	/**
	 * 红点被清除事件
	 * 当监听的红点标签被 ClearRedDotStateInBranch() 清除时，该事件会被广播
	 */
	UPROPERTY(BlueprintAssignable)
	FRedDotCleared OnRedDotCleared;
	
private:
	void HandleRedDotCleared();

	TWeakObjectPtr<UWorld> WorldPtr;
	FGameplayTag RedDotTag;
	
	FYcRedRelierListenerHandle ListenerHandle;
};
