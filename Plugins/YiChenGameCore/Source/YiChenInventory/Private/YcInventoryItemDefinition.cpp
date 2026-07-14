// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcInventoryItemDefinition.h"

#include "YcInventoryItemInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcInventoryItemDefinition)

void FYcInventoryItemFragment::OnInstanceCreated(UYcInventoryItemInstance* Instance) const
{
	//@TODO 当物品实例对象创建后Fragment可对此做出响应
}

const void* FYcInventoryItemDefinition::FindTypedFragmentByStruct(const UScriptStruct* FragmentStructType) const
{
	if (!FragmentStructType)
	{
		return nullptr;
	}

	for (const TInstancedStruct<FYcInventoryItemFragment>& Fragment : Fragments)
	{
		const UScriptStruct* ScriptStruct = Fragment.GetScriptStruct();
		if (ScriptStruct && ScriptStruct->IsChildOf(FragmentStructType))
		{
			return Fragment.GetMemory();
		}
	}

	return nullptr;
}
