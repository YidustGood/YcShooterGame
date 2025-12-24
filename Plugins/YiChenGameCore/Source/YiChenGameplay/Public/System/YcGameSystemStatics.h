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
};