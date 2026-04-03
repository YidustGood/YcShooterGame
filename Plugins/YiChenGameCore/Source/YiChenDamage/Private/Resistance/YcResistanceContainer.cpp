// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Resistance/YcResistanceContainer.h"

//////////////////////////////////////////////////////////////////////
// FYcResistanceEntry

FString FYcResistanceEntry::GetDebugString() const
{
	return FString::Printf(TEXT("%s: %.2f"), *Tag.ToString(), Value);
}

// FYcResistanceEntry
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// FYcResistanceContainer

void FYcResistanceContainer::SetResistance(FGameplayTag Tag, float InValue)
{
	if (!Tag.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag was passed to SetResistance"), ELogVerbosity::Warning);
		return;
	}

	// 限制抗性范围
	const float ClampedValue = FMath::Clamp(InValue, MinResistance, MaxResistance);

	// 查找现有条目
	for (FYcResistanceEntry& Entry : Entries)
	{
		if (Entry.Tag != Tag)
		{
			continue;
		}

		Entry.Value = ClampedValue;
		TagToValueMap[Tag] = ClampedValue;
		MarkItemDirty(Entry);
		return;
	}

	// 创建新条目
	FYcResistanceEntry& NewEntry = Entries.Emplace_GetRef(Tag, ClampedValue);
	MarkItemDirty(NewEntry);
	TagToValueMap.Add(Tag, ClampedValue);
}

void FYcResistanceContainer::AddResistance(FGameplayTag Tag, float Delta)
{
	if (!Tag.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag was passed to AddResistance"), ELogVerbosity::Warning);
		return;
	}

	if (FMath::IsNearlyZero(Delta))
	{
		return;
	}

	// 查找现有条目
	for (FYcResistanceEntry& Entry : Entries)
	{
		if (Entry.Tag != Tag)
		{
			continue;
		}

		const float NewValue = FMath::Clamp(Entry.Value + Delta, MinResistance, MaxResistance);
		Entry.Value = NewValue;
		TagToValueMap[Tag] = NewValue;
		MarkItemDirty(Entry);
		return;
	}

	// 创建新条目
	const float NewValue = FMath::Clamp(Delta, MinResistance, MaxResistance);
	FYcResistanceEntry& NewEntry = Entries.Emplace_GetRef(Tag, NewValue);
	MarkItemDirty(NewEntry);
	TagToValueMap.Add(Tag, NewValue);
}

void FYcResistanceContainer::RemoveResistance(FGameplayTag Tag)
{
	if (!Tag.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag was passed to RemoveResistance"), ELogVerbosity::Warning);
		return;
	}

	for (auto It = Entries.CreateIterator(); It; ++It)
	{
		FYcResistanceEntry& Entry = *It;
		if (Entry.Tag != Tag)
		{
			continue;
		}

		It.RemoveCurrent();
		TagToValueMap.Remove(Tag);
		MarkArrayDirty();
		return;
	}
}

TArray<FGameplayTag> FYcResistanceContainer::GetAllResistanceTags() const
{
	TArray<FGameplayTag> Tags;
	TagToValueMap.GetKeys(Tags);
	return Tags;
}

void FYcResistanceContainer::ClearAllResistances()
{
	Entries.Empty();
	TagToValueMap.Empty();
	MarkArrayDirty();
}

void FYcResistanceContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		const FGameplayTag Tag = Entries[Index].Tag;
		TagToValueMap.Remove(Tag);
	}
}

void FYcResistanceContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FYcResistanceEntry& Entry = Entries[Index];
		TagToValueMap.Add(Entry.Tag, Entry.Value);
	}
}

void FYcResistanceContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		const FYcResistanceEntry& Entry = Entries[Index];
		TagToValueMap[Entry.Tag] = Entry.Value;
	}
}

// FYcResistanceContainer
//////////////////////////////////////////////////////////////////////
