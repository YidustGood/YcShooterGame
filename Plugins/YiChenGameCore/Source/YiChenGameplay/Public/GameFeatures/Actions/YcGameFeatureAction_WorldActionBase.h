// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameFeatureAction.h"
#include "GameFeaturesSubsystem.h"
#include "YcGameFeatureAction_WorldActionBase.generated.h"

/**
 * 支持多World实例的GameFeatureAction基类。
 * 该基类用于处理需要针对特定World进行操作的GameFeatureAction，
 * 支持PIE环境下多个World实例的场景，并通过World匹配校验确保Action在正确的World中执行。
 */
UCLASS(Abstract)
class YICHENGAMEPLAY_API UYcGameFeatureAction_WorldActionBase : public UGameFeatureAction
{
	GENERATED_BODY()
public:
	//~ Begin UGameFeatureAction interface
	/**
	 * 游戏功能激活时调用，为所有匹配的World添加Action功能。
	 * 该函数会遍历所有World实例，通过ShouldApplyToWorldContext()进行World匹配校验，
	 * 仅对匹配的World调用AddToWorld()函数。同时绑定GameInstance启动事件以处理后续创建的World。
	 * @param Context 游戏功能激活上下文
	 */
	virtual void OnGameFeatureActivating(FGameFeatureActivatingContext& Context) override;
	
	/**
	 * 游戏功能停用时调用，清理为World添加的Action功能。
	 * 该函数会解绑之前绑定的GameInstance启动事件委托。
	 * @param Context 游戏功能停用上下文
	 */
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
	//~ End of UGameFeatureAction interface

	/**
	 * 处理GameInstance启动事件的回调。
	 * 当新的GameInstance启动时调用，用于为新创建的World添加Action功能。
	 * @param GameInstance 启动的GameInstance实例
	 * @param ChangeContext 游戏功能状态变更上下文
	 */
	void HandleGameInstanceStart(UGameInstance* GameInstance, FGameFeatureStateChangeContext ChangeContext);

	/**
	 * 将Action功能添加到指定的World中。
	 * 子类应重写此函数实现具体的Action逻辑。传递的WorldContext已经过匹配校验，
	 * 确保该World与当前GameFeatureAction所属的World相匹配。
	 * @param WorldContext 目标World的上下文
	 * @param ChangeContext 游戏功能状态变更上下文
	 */
	virtual void AddToWorld(const FWorldContext& WorldContext, const FGameFeatureStateChangeContext& ChangeContext) PURE_VIRTUAL(USCGameFeatureAction_WorldActionBase::AddToWorld,);

private:
	/** GameInstance启动事件的委托句柄映射，用于GameFeatureAction停用时清理委托绑定 */
	TMap<FGameFeatureStateChangeContext, FDelegateHandle> GameInstanceStartHandles;
};
