// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Fragments/InventoryFragment_InitialItemStats.h"

#include "GameplayTagContainer.h"
#include "YcInventoryItemInstance.h"

void FInventoryFragment_InitialItemStats::OnInstanceCreated(UYcInventoryItemInstance* Instance) const
{
	// 初始化整数属性
	for (const auto& Kvp : InitialItemStats)
	{
		Instance->AddStatTagStack(Kvp.Key, Kvp.Value);
	}
	
	// 初始化浮点数属性
	for (const auto& Kvp : InitialFloatStats)
	{
		Instance->SetFloatTagStack(Kvp.Key, Kvp.Value);
	}
}

int32 FInventoryFragment_InitialItemStats::GetItemStatByTag(FGameplayTag Tag) const
{
	if (const int32* StatPtr = InitialItemStats.Find(Tag))
	{
		return *StatPtr;
	}

	return 0;
}

float FInventoryFragment_InitialItemStats::GetItemFloatStatByTag(FGameplayTag Tag) const
{
	if (const float* StatPtr = InitialFloatStats.Find(Tag))
	{
		return *StatPtr;
	}
	
	return 0.0f;
}