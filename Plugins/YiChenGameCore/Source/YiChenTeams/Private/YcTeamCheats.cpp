// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcTeamCheats.h"

#include "YcTeamSubsystem.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcTeamCheats)

void UYcTeamCheats::CycleTeam()
{
	UYcTeamSubsystem& TeamSubsystem = UYcTeamSubsystem::Get(this);
	
	APlayerController* PC = GetPlayerController();

	// 获取当前玩家的旧团队ID和所有可用团队ID列表
	const int32 OldTeamId = TeamSubsystem.FindTeamFromObject(PC);
	const TArray<int32> TeamIds = TeamSubsystem.GetTeamIDs();
		
	if (TeamIds.Num())
	{
		// 找到当前团队在列表中的索引，计算下一个团队的索引（循环）
		const int32 IndexOfOldTeam = TeamIds.Find(OldTeamId);
		const int32 IndexToUse = (IndexOfOldTeam + 1) % TeamIds.Num();

		const int32 NewTeamId = TeamIds[IndexToUse];

		// 切换到新团队
		TeamSubsystem.ChangeTeamForActor(PC, NewTeamId);
	}

	// 获取实际切换后的团队ID并输出日志
	const int32 ActualNewTeamId = TeamSubsystem.FindTeamFromObject(PC);
	UE_LOG(LogConsoleResponse, Log, TEXT("Changed to team %d (from team %d)"), ActualNewTeamId, OldTeamId);
}

void UYcTeamCheats::SetTeam(int32 TeamID)
{
	UYcTeamSubsystem& TeamSubsystem = UYcTeamSubsystem::Get(this);
	
	// 检查指定的团队是否存在，存在则切换
	if (TeamSubsystem.DoesTeamExist(TeamID))
	{
		APlayerController* PC = GetPlayerController();
		TeamSubsystem.ChangeTeamForActor(PC, TeamID);
	}
}

void UYcTeamCheats::ListTeams()
{
	UYcTeamSubsystem& TeamSubsystem = UYcTeamSubsystem::Get(this);
	
	// 获取所有团队ID并逐个输出到控制台
	const TArray<int32> TeamIDs = TeamSubsystem.GetTeamIDs();

	for (const int32 TeamID : TeamIDs)
	{
		UE_LOG(LogConsoleResponse, Log, TEXT("Team ID %d"), TeamID);
	}
}

void UYcTeamCheats::ShowMeTeam()
{
	UYcTeamSubsystem& TeamSubsystem = UYcTeamSubsystem::Get(this);
	APlayerController* PC = GetPlayerController();
	// 查找并输出当前玩家所属的团队ID
	UE_LOG(LogConsoleResponse, Log, TEXT("TeamID: %d"), TeamSubsystem.FindTeamFromObject(PC));
}