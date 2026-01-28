// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "YcGameSystemStatics.generated.h"

/**
 * 全局游戏系统相关的蓝图静态函数库
 */
UCLASS()
class YICHENGAMEPLAY_API UYcGameSystemStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * 重新开始当前游戏对局（重新加载当前关卡）
	 * 会基于当前世界的 LastURL 重新执行 ServerTravel，可选择是否使用无缝旅行
	 * @param WorldContextObject 世界上下文对象（通常传入 GameInstance / World / Actor 等）
	 * @param bSeamlessTravel 是否尝试使用无缝旅行（SeamlessTravel），默认关闭
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="YcGameCore", meta = (WorldContext = "WorldContextObject"))
	static void PlayNextGame(const UObject* WorldContextObject, bool bSeamlessTravel = false);
	
	/** 获取引擎构建版本 */
	UFUNCTION(BlueprintPure, Category="YcGameCore")
	static FString GetBuildVersion();

	/** 获取游戏运行时长 */
	UFUNCTION(BlueprintPure, Category="YcGameCore")
	static double GetGameTime();

	/**
	 * 获取游戏版本号
	 * 实际上就是项目设置中的ProjectVersion
	 * /Script/EngineSettings.GeneralProjectSettings
	 */
	UFUNCTION(BlueprintPure, Category="YcGameCore")
	static FString GetGameVersion();

	/**
	 * 获取启动命令行参数
	 * @return 启动命令行参数
	 */
	UFUNCTION(BlueprintPure, Category="YcGameCore")
	static FString GetCommandLine();
	
	/**
	 * 获取匹配的启动命令行参数
	 * @param Key 参数名称
	 * @param OutValue 参数数值, 匹配到参数后面的值(截止到下一个空格字符)
	 * @return 是否存在
	 * 例如有启动命令:-port=7777 -name=测试,调用该函数传入-port=,OutValue的值将会是7777
	 */
	UFUNCTION(BlueprintPure, Category="YcGameCore")
	static bool GetMatchedCommandLine(const FString& Key, FString& OutValue);

	
	/**
	 * 是否存在匹配的启动命令行参数
	 * @param Key 参数名称
	 * @return 是否存在
	 */
	UFUNCTION(BlueprintPure, Category="YcGameCore")
	static bool HasMatchedCommandLine(const FString& Key);

	/**
	 * 获取当前是否为PIE模式
	 * @return 是否为PIE模式
	 */
	UFUNCTION(BlueprintPure, Category="YcGameCore")
	static bool IsPlayInEditor();
};