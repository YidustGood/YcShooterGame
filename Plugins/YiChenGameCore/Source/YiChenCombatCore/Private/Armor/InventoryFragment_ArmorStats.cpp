// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Armor/InventoryFragment_ArmorStats.h"
#include "YcInventoryItemInstance.h"
#include "Armor/YcArmorGameplayTags.h"

FGameplayTag FInventoryFragment_ArmorStats::GetDurabilityTag()
{
	return YcArmorGameplayTags::Armor_Durability;
}

void FInventoryFragment_ArmorStats::OnInstanceCreated(UYcInventoryItemInstance* Instance) const
{
	if (!Instance) return;

	// 初始化耐久度为最大值
	Instance->SetFloatTagStack(GetDurabilityTag(), MaxDurability);
}

float FInventoryFragment_ArmorStats::GetCurrentDurability(const UYcInventoryItemInstance* Instance)
{
	if (!Instance) return 0.0f;
	return Instance->GetFloatTagStackValue(GetDurabilityTag());
}

void FInventoryFragment_ArmorStats::SetCurrentDurability(UYcInventoryItemInstance* Instance, float Value)
{
	if (!Instance) return;
	Instance->SetFloatTagStack(GetDurabilityTag(), Value);
}

bool FInventoryFragment_ArmorStats::IsBroken(const UYcInventoryItemInstance* Instance)
{
	if (!Instance) return true;
	return GetCurrentDurability(Instance) <= 0.0f;
}
