// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Fragments/InventoryFragment_ItemTags.h"

#include "YcInventoryItemInstance.h"

void FInventoryFragment_ItemTags::OnInstanceCreated(UYcInventoryItemInstance* Instance) const
{
	if (Instance == nullptr)
	{
		return;
	}
	
	// 将配置的标签添加到实例的 OwnedTags 中
	for (const FGameplayTag& Tag : OwnedTags)
	{
		Instance->AddOwnedTag(Tag);
	}
}
