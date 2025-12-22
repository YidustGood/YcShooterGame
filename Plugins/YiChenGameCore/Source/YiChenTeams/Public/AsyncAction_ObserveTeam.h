// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Engine/CancellableAsyncAction.h"
#include "AsyncAction_ObserveTeam.generated.h"

class IYcTeamAgentInterface;

/**
 * 观察指定对象的团队变更的异步操作
 * 用于监听Actor的团队归属变化
 */
UCLASS()
class YICHENTEAMS_API UAsyncAction_ObserveTeam : public UCancellableAsyncAction
{
	GENERATED_BODY()
public:
	UAsyncAction_ObserveTeam(const FObjectInitializer& ObjectInitializer);
	
	/**
	 * 观察指定团队代理的团队变更
	 * - 会立即触发一次以提供当前的团队分配状态
	 * - 对于任何可以实现团队归属的对象（实现了IYcTeamAgentInterface），
	 *   它也会监听未来的团队分配变更
	 * @param TeamActor 要观察的团队Actor
	 * @return 异步操作对象，如果Actor不包含团队代理接口则返回nullptr
	 */
	UFUNCTION(BlueprintCallable, meta=(BlueprintInternalUseOnly="true", Keywords="Watch"))
	static UAsyncAction_ObserveTeam* ObserveTeam(AActor* TeamActor);
	
	//~UBlueprintAsyncActionBase interface
	virtual void Activate() override;
	virtual void SetReadyToDestroy() override;
	//~End of UBlueprintAsyncActionBase interface
	
	/** 团队观察异步委托，当团队设置或变更时触发 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FTeamObservedAsyncDelegate, bool, bTeamSet, int32, TeamId);
	
	/** 当团队被设置或变更时调用 */
	UPROPERTY(BlueprintAssignable)
	FTeamObservedAsyncDelegate OnTeamChanged;

private:
	/** 被观察的团队代理变更团队时的回调函数 */
	UFUNCTION()
	void OnWatchedAgentChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);

	/** 弱引用指针，指向被观察的团队代理接口 */
	TWeakInterfacePtr<IYcTeamAgentInterface> TeamInterfacePtr;
};