// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameplayCommon/GameplayMessageTypes.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(GameplayMessageTypes)

namespace YcGameplayTags
{
	// 定义玩家初始化完成事件消息通道标签
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Gameplay_GameMode_PlayerInitialized, "Gameplay_GameMode_PlayerInitialized", "玩家登入游戏Controller初始化完成事件消息通道");
	
	// 定义ASC组件初始化完成消息标签
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_State_AbilitySystemInitialized, "Ability.State.AbilitySystemInitialized", "ASC组件初始化完成消息");
	
	// 定义ASC组件取消初始化消息标签
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(TAG_Ability_State_AbilitySystemUninitialized, "Ability.State.AbilitySystemUninitialized", "ASC组件取消初始化消息");
}