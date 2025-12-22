// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcTeamComponent.h"

#include "YiChenTeams.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcTeamComponent)

UYcTeamComponent::UYcTeamComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicated(true);
}

void UYcTeamComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	// 配置基于推送的网络复制参数
	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	// 注册团队ID和小队ID的网络复制
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MyTeamID, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, MySquadID, SharedParams);
}

void UYcTeamComponent::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	// 仅在服务器端允许设置团队ID
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		const FGenericTeamId OldTeamID = MyTeamID;

		// 标记属性为脏，触发网络复制
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MyTeamID, this);
		MyTeamID = NewTeamID;
		// 条件性地广播团队变更事件
		ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);
	}
	else
	{
		UE_LOG(LogYcTeamSystem, Error, TEXT("Cannot set team for %s on non-authority"), *GetPathName(this));
	}
}

FGenericTeamId UYcTeamComponent::GetGenericTeamId() const
{
	return MyTeamID;
}

FOnYcTeamIndexChangedDelegate* UYcTeamComponent::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

int32 UYcTeamComponent::GetTeamId() const
{
	// 将通用团队ID转换为整数形式
	return GenericTeamIdToInteger(MyTeamID);
}

int32 UYcTeamComponent::GetSquadId() const
{
	return MySquadID;
}

int32 UYcTeamComponent::K2_GetTeamId() const
{
	return GenericTeamIdToInteger(MyTeamID);
}

void UYcTeamComponent::SetSquadID(int32 NewSquadID)
{
	// 仅在服务器端允许设置小队ID
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		// 标记属性为脏，触发网络复制
		MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, MySquadID, this);

		MySquadID = NewSquadID;
	}
}

void UYcTeamComponent::OnRep_MyTeamID(FGenericTeamId OldTeamID)
{
	// 当团队ID通过网络复制到客户端时，可以在这里处理相关的客户端逻辑
	ConditionalBroadcastTeamChanged(this, OldTeamID, MyTeamID);
}

void UYcTeamComponent::OnRep_MySquadID(int32 OldSquadId)
{
	// 当小队ID通过网络复制到客户端时，可以在这里处理相关的客户端逻辑
	// 例如创建小队子系统来处理团队中的小队
}