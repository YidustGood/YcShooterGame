// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcTeamPublicInfo.h"

#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcTeamPublicInfo)

AYcTeamPublicInfo::AYcTeamPublicInfo(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
}

void AYcTeamPublicInfo::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION(AYcTeamPublicInfo, TeamAsset, COND_InitialOnly);
}

void AYcTeamPublicInfo::SetTeamAsset(const TObjectPtr<UYcTeamAsset> NewTeamAsset)
{
	// 确保仅在服务器端调用，且TeamAsset尚未设置
	check(HasAuthority());
	check(TeamAsset == nullptr);

	TeamAsset = NewTeamAsset;

	// 设置团队资源后尝试注册到子系统
	TryRegisterWithTeamSubsystem();
}

void AYcTeamPublicInfo::OnRep_TeamDisplayAsset()
{
	// 当TeamAsset通过网络复制到客户端时，尝试注册到子系统
	TryRegisterWithTeamSubsystem();
}