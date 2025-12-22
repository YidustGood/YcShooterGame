// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameFramework/CheatManager.h"
#include "YcTeamCheats.generated.h"

/**
 * 团队相关的作弊命令扩展
 * 提供用于开发和调试的团队管理命令
 */
UCLASS()
class YICHENTEAMS_API UYcTeamCheats : public UCheatManagerExtension
{
	GENERATED_BODY()
public:
	/**
	 * 将当前玩家切换到下一个可用团队
	 * 如果当前在列表末尾，则循环回到第一个团队
	 * 注意：此函数只能在服务器端执行
	 */
	UFUNCTION(Exec, BlueprintAuthorityOnly)
	virtual void CycleTeam();

	/**
	 * 将当前玩家移动到指定的团队
	 * @param TeamID 目标团队ID
	 * 注意：此函数只能在服务器端执行
	 */
	UFUNCTION(Exec, BlueprintAuthorityOnly)
	virtual void SetTeam(int32 TeamID);

	/**
	 * 打印所有团队的列表
	 */
	UFUNCTION(Exec)
	virtual void ListTeams();
	
	/**
	 * 显示当前玩家所属的团队ID
	 */
	UFUNCTION(Exec)
	virtual void ShowMeTeam();
};
