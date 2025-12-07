// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "System/YcGameData.h"

#include "System/YcAssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameData)

UYcGameData::UYcGameData()
{
}

const UYcGameData& UYcGameData::Get()
{
	// 从资源管理器获取全局游戏数据
	return UYcAssetManager::Get().GetGameData();
}