// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcTeamInfoBase.h"

#include "YcTeamSubsystem.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcTeamInfoBase)

AYcTeamInfoBase::AYcTeamInfoBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TeamId(INDEX_NONE)
{
	// 设置网络复制相关属性，确保团队信息能够正确同步到所有客户端
	bReplicates = true;
	bAlwaysRelevant = true;
	NetPriority = 3.0f;
	SetReplicatingMovement(false);
}

void AYcTeamInfoBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION(AYcTeamInfoBase, TeamId, COND_InitialOnly);
	DOREPLIFETIME(AYcTeamInfoBase, TeamTags);
}

void AYcTeamInfoBase::BeginPlay()
{
	Super::BeginPlay();
	// 在游戏开始时尝试向子系统注册团队信息
	TryRegisterWithTeamSubsystem();
}

void AYcTeamInfoBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{ 
	// 在游戏结束时注销团队信息
	if (TeamId != INDEX_NONE)
	{
		if (UYcTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UYcTeamSubsystem>())
		{
			// EndPlay可能在子系统已被销毁的异常时机调用，需要检查子系统是否存在
			TeamSubsystem->UnregisterTeamInfo(this);
		}
	}

	Super::EndPlay(EndPlayReason);
}

void AYcTeamInfoBase::RegisterWithTeamSubsystem(UYcTeamSubsystem* Subsystem)
{
	Subsystem->RegisterTeamInfo(this);
}

void AYcTeamInfoBase::TryRegisterWithTeamSubsystem()
{
	if (TeamId != INDEX_NONE)
	{
		UYcTeamSubsystem* TeamSubsystem = GetWorld()->GetSubsystem<UYcTeamSubsystem>();
		if (ensure(TeamSubsystem))
		{
			RegisterWithTeamSubsystem(TeamSubsystem);
		}
	}
}

void AYcTeamInfoBase::SetTeamId(const int32 NewTeamId)
{
	// 确保仅在服务器端调用，且TeamId尚未设置
	check(HasAuthority());
	check(TeamId == INDEX_NONE);
	check(NewTeamId != INDEX_NONE);

	TeamId = NewTeamId;

	// 设置团队ID后尝试注册到子系统
	TryRegisterWithTeamSubsystem();
}

void AYcTeamInfoBase::OnRep_TeamId()
{
	// 当TeamId通过网络复制到客户端时，尝试注册到子系统
	TryRegisterWithTeamSubsystem();
}