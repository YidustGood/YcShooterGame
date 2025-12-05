// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcInventoryItemInstance.h"
#include "Net/UnrealNetwork.h"
#include "GameplayTagContainer.h"

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
	DOREPLIFETIME(UYcInventoryItemInstance, TagsStack);
}

TInstancedStruct<FYcInventoryItemFragment> UYcInventoryItemInstance::FindItemFragment(
	const UScriptStruct* FragmentStructType)
{
	TInstancedStruct<FYcInventoryItemFragment> NullStruct;
	if (!GetItemDef()) return NullStruct;
	
	for (auto& Frag : ItemDef->Fragments)
	{	
		if (FragmentStructType == Frag.GetScriptStruct())
		{
			return Frag;
		}
	}
	return NullStruct;
}

void UYcInventoryItemInstance::SetItemDefRowHandle(const FDataTableRowHandle& InItemDefRowHandle)
{
	const FDataTableRowHandle& LastRowHandle = ItemDefRowHandle;
	ItemDefRowHandle = InItemDefRowHandle;
	OnRep_ItemDefRowHandle(LastRowHandle);
}

void UYcInventoryItemInstance::SetItemInstId(const FName NewId)
{
	ItemInstId = NewId;
}

const FYcInventoryItemDefinition* UYcInventoryItemInstance::GetItemDef()
{
	ItemDef = ItemDef ? ItemDef : ItemDefRowHandle.GetRow<FYcInventoryItemDefinition>(TEXT("UYcInventoryItemInstance::GetItemDef()"));
	return ItemDef;
}

void UYcInventoryItemInstance::AddStatTagStack(const FGameplayTag Tag, const int32 StackCount)
{
	TagsStack.AddStack(Tag, StackCount);
}

void UYcInventoryItemInstance::RemoveStatTagStack(const FGameplayTag Tag, const int32 StackCount)
{
	TagsStack.RemoveStack(Tag, StackCount);
}

int32 UYcInventoryItemInstance::GetStatTagStackCount(const FGameplayTag Tag) const
{
	return TagsStack.GetStackCount(Tag);
}

bool UYcInventoryItemInstance::HasStatTag(const FGameplayTag Tag) const
{
	return TagsStack.ContainsTag(Tag);
}

bool UYcInventoryItemInstance::K2_GetItemDef(FYcInventoryItemDefinition& OutItemDef)
{
	if (ItemDef == nullptr) return false;
	OutItemDef = *ItemDef;
	return true;
}

void UYcInventoryItemInstance::OnRep_ItemDefRowHandle(FDataTableRowHandle LastItemDefRowHandle)
{
	ItemDef = ItemDefRowHandle.GetRow<FYcInventoryItemDefinition>(TEXT("OnRep_ItemDefRowHandle"));
}