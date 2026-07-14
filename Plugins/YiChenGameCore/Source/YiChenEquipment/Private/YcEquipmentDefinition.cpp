// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcEquipmentDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcEquipmentDefinition)

const void* FYcEquipmentDefinition::FindTypedFragmentByStruct(const UScriptStruct* FragmentStructType) const
{
	if (!FragmentStructType)
	{
		return nullptr;
	}

	for (const TInstancedStruct<FYcEquipmentFragment>& Fragment : Fragments)
	{
		const UScriptStruct* ScriptStruct = Fragment.GetScriptStruct();
		if (ScriptStruct && ScriptStruct->IsChildOf(FragmentStructType))
		{
			return Fragment.GetMemory();
		}
	}

	return nullptr;
}
