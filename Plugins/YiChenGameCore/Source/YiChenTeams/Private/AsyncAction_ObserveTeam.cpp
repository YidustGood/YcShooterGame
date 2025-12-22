// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "AsyncAction_ObserveTeam.h"

#include "YcTeamAgentInterface.h"
#include "YcTeamSubsystem.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncAction_ObserveTeam)

UAsyncAction_ObserveTeam::UAsyncAction_ObserveTeam(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UAsyncAction_ObserveTeam* UAsyncAction_ObserveTeam::ObserveTeam(AActor* TeamActor)
{
	// 尝试从Actor中查找团队代理接口
	TScriptInterface<IYcTeamAgentInterface> TeamAgent;
	if (UYcTeamSubsystem::FindTeamAgentFromActor(TeamActor, TeamAgent))
	{
		// 创建异步操作对象并设置团队接口引用
		UAsyncAction_ObserveTeam* Action  = NewObject<UAsyncAction_ObserveTeam>();
		Action->TeamInterfacePtr = TeamAgent;
		// 注册到游戏实例，确保生命周期管理
		Action->RegisterWithGameInstance(TeamAgent.GetObject());
		return Action;
	}
	return nullptr;
}

void UAsyncAction_ObserveTeam::Activate()
{
	bool bCouldSucceed = false;
	int32 CurrentTeamIndex = INDEX_NONE;

	// 如果团队接口仍然有效，获取当前团队ID并绑定变更委托
	if (IYcTeamAgentInterface* TeamInterface = TeamInterfacePtr.Get())
	{
		CurrentTeamIndex = GenericTeamIdToInteger(TeamInterface->GetGenericTeamId());
		// 绑定队伍更改委托回调，以便接收未来的团队变更通知
		TeamInterface->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnWatchedAgentChangedTeam);

		bCouldSucceed = true;
	}

	// 立即广播一次，让用户获得当前状态
	OnTeamChanged.Broadcast(CurrentTeamIndex != INDEX_NONE, CurrentTeamIndex);

	// 如果无法绑定到委托，则无法获得任何后续更新，标记为准备销毁
	if (!bCouldSucceed)
	{
		SetReadyToDestroy();
	}
}

void UAsyncAction_ObserveTeam::OnWatchedAgentChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	// 当被观察的团队代理变更团队时，广播变更事件
	OnTeamChanged.Broadcast(NewTeam != INDEX_NONE, NewTeam);
}

void UAsyncAction_ObserveTeam::SetReadyToDestroy()
{
	Super::SetReadyToDestroy();

	// 如果正在被取消，需要解除所有可能正在监听的委托绑定
	if (IYcTeamAgentInterface* TeamInterface = TeamInterfacePtr.Get())
	{
		TeamInterface->GetTeamChangedDelegateChecked().RemoveAll(this);
	}
}