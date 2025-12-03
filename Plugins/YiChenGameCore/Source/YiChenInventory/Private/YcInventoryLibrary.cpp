// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcInventoryLibrary.h"
#include "YcInventoryItemInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcInventoryLibrary)

TInstancedStruct<FYcInventoryItemFragment> UYcInventoryLibrary::FindItemFragment(const FYcInventoryItemDefinition& ItemDef,
                                                                                 const UScriptStruct* FragmentStructType)
{
	TInstancedStruct<FYcInventoryItemFragment> NullStruct;
	for (auto Frag : ItemDef.Fragments)
	{	
		if (FragmentStructType == Frag.GetScriptStruct())
		{
			return Frag;
		}
	}
	return NullStruct;
}
