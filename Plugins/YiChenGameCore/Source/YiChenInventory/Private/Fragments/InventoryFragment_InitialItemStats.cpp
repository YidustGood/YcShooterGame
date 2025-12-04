// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Fragments/InventoryFragment_InitialItemStats.h"

#include "GameplayTagContainer.h"
#include "YcInventoryItemInstance.h"

void FInventoryFragment_InitialItemStats::OnInstanceCreated(UYcInventoryItemInstance* Instance) const
{
	for (const auto& Kvp : InitialItemStats)
	{
		Instance->AddStatTagStack(Kvp.Key, Kvp.Value);
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