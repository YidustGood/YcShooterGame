// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "YcGameData.generated.h"

/**
 * 全局游戏数据资源
 *
 * 运行时不可变的数据资产，包含项目级全局配置。
 * 适合存放经验系统、流程配置、模式配置等“内容驱动”的主资产数据。
 *
 */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "YcGameData", ShortTooltip = "Data asset containing global game data."))
class YICHENGAMEPLAY_API UYcGameData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UYcGameData();

	/**
	 * 获取全局游戏数据
	 * @return 加载的全局游戏数据引用
	 */
	static const UYcGameData& Get();
};
