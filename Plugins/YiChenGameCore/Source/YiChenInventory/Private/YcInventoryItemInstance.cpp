// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcInventoryItemInstance.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcInventoryItemInstance)

UYcInventoryItemInstance::UYcInventoryItemInstance(const FObjectInitializer& ObjectInitializer)
{
}

bool UYcInventoryItemInstance::IsSupportedForNetworking() const
{
	return true;
}

void UYcInventoryItemInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UYcInventoryItemInstance, ItemDef);
}

TInstancedStruct<FYcInventoryItemFragment> UYcInventoryItemInstance::FindItemFragment(
	const UScriptStruct* FragmentStructType) const
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

void UYcInventoryItemInstance::SetItemDef(const FYcInventoryItemDefinition& InItemDef)
{
	ItemDef = InItemDef;
}

void UYcInventoryItemInstance::SetItemInstId(const FName NewId)
{
	ItemInstId = NewId;
}
