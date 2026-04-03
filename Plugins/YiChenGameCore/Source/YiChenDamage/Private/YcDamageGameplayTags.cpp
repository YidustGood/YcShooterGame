// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcDamageGameplayTags.h"

namespace YcDamageGameplayTags
{
	// ================================================================================
	// 伤害类型标签
	// ================================================================================

	UE_DEFINE_GAMEPLAY_TAG(Damage_Type_Physical, "Damage.Type.Physical");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Type_Magic, "Damage.Type.Magic");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Type_Fire, "Damage.Type.Fire");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Type_Ice, "Damage.Type.Ice");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Type_Lightning, "Damage.Type.Lightning");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Type_Poison, "Damage.Type.Poison");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Type_True, "Damage.Type.True");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Type_DOT, "Damage.Type.DOT");

	// ================================================================================
	// 伤害事件标签
	// ================================================================================

	UE_DEFINE_GAMEPLAY_TAG(Damage_Event_Critical, "Damage.Event.Critical");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Event_Dodged, "Damage.Event.Dodged");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Event_Immunity, "Damage.Event.Immunity");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Event_Blocked, "Damage.Event.Blocked");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Event_ArmorBroken, "Damage.Event.ArmorBroken");
	UE_DEFINE_GAMEPLAY_TAG(Damage_Event_OutOfHealth, "Damage.Event.OutOfHealth");
}
