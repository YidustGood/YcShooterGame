// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameFramework/CheatManager.h"
#include "YcCheatManager.generated.h"

#ifndef USING_CHEAT_MANAGER
#define USING_CHEAT_MANAGER (1 && !UE_BUILD_SHIPPING)
#endif // #ifndef USING_CHEAT_MANAGER

DECLARE_LOG_CATEGORY_EXTERN(LogYcGameCheat, Log, All);

/**
 * YiChenGameCore插件提供的作弊管理器
 * 需要在所使用的PlayerController中指定使用本类, 用于在开发阶段方便的修改一些内容做测试
 */
UCLASS()
class YICHENGAMEPLAY_API UYcCheatManager : public UCheatManager
{
	GENERATED_BODY()
public:
	UYcCheatManager();
	
	/** 初始化作弊管理器，在此阶段应用编辑器配置的自动执行作弊等逻辑 */
	virtual void InitCheatManager() override;
	
	/**
	 * 将文本输出到控制台和日志的辅助函数
	 * @param TextToOutput 要输出的文本内容
	 */
	static void CheatOutputText(const FString& TextToOutput);
	
	
	/**
	 * 在服务器上为当前拥有该控制器的玩家执行一条作弊命令
	 * @param Msg 作弊命令字符串，将被截断到合适长度后发送到服务器
	 */
	UFUNCTION(exec)
	void Cheat(const FString& Msg);

	/**
	 * 在服务器上为所有玩家执行一条作弊命令
	 * @param Msg 作弊命令字符串，将被截断到合适长度后广播到服务器
	 */
	UFUNCTION(exec)
	void CheatAll(const FString& Msg);

	/**
	 * 启动下一局游戏（重新加载当前关卡）
	 * 内部通过 UYcGameSystemStatics::PlayNextGame 实现，可选择是否使用无缝旅行
	 * @param bSeamlessTravel 是否尝试使用无缝旅行
	 */
	UFUNCTION(Exec, BlueprintAuthorityOnly)
	void PlayNextGame(bool bSeamlessTravel = false);
};