// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace YcDamageGameplayTags
{
	// ================================================================================
	// 伤害类型标签 (Damage.Type.*)
	// 用于区分不同类型的伤害，驱动伤害计算组件过滤
	// ================================================================================

	YICHENDAMAGE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Type_Physical);
	YICHENDAMAGE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Type_Magic);
	YICHENDAMAGE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Type_Fire);
	YICHENDAMAGE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Type_Ice);
	YICHENDAMAGE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Type_Lightning);
	YICHENDAMAGE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Type_Poison);
	YICHENDAMAGE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Type_True);
	YICHENDAMAGE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Type_DOT);

	// ================================================================================
	// 伤害事件标签 (Damage.Event.*)
	// 用于发送伤害相关事件，在 TemporaryTags 中标记伤害状态
	// ================================================================================

	YICHENDAMAGE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Event_Critical);
	YICHENDAMAGE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Event_Dodged);
	YICHENDAMAGE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Event_Immunity);
	YICHENDAMAGE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Event_Blocked);
	YICHENDAMAGE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Event_ArmorBroken);
	YICHENDAMAGE_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(Damage_Event_OutOfHealth);
}
