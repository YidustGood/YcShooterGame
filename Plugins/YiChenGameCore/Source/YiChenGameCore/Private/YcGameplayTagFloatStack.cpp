// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcGameplayTagFloatStack.h"

//////////////////////////////////////////////////////////////////////
// FYcGameplayTagFloatStack

FString FYcGameplayTagFloatStack::GetDebugString() const
{
	return FString::Printf(TEXT("%s=%.2f"), *Tag.ToString(), Value);
}

// FYcGameplayTagFloatStack
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// FYcGameplayTagFloatStackContainer

void FYcGameplayTagFloatStackContainer::AddStack(FGameplayTag Tag, float Value)
{
	if (!Tag.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag was passed to AddStack"), ELogVerbosity::Warning);
		return;
	}
	if (Value <= 0.0f) return;

	for (FYcGameplayTagFloatStack& Stack : Stacks)
	{
		if (Stack.Tag != Tag) continue;

		const float NewValue = Stack.Value + Value;
		Stack.Value = NewValue;
		TagToValueMap[Tag] = NewValue;
		MarkItemDirty(Stack);
		return;
	}

	// Tag不存在，创建新条目
	FYcGameplayTagFloatStack& NewStack = Stacks.Emplace_GetRef(Tag, Value);
	MarkItemDirty(NewStack);
	TagToValueMap.Add(Tag, Value);
}

void FYcGameplayTagFloatStackContainer::RemoveStack(FGameplayTag Tag, float Value)
{
	if (!Tag.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag was passed to RemoveStack"), ELogVerbosity::Warning);
		return;
	}
	if (Value <= 0.0f) return;

	for (auto It = Stacks.CreateIterator(); It; ++It)
	{
		FYcGameplayTagFloatStack& Stack = *It;
		if (Stack.Tag != Tag) continue;

		if (Stack.Value <= Value)
		{
			// 现存值 <= 要移除的值，直接移除整个条目
			It.RemoveCurrent();
			TagToValueMap.Remove(Tag);
			MarkArrayDirty();
		}
		else
		{
			// 移除指定值
			const float NewValue = Stack.Value - Value;
			Stack.Value = NewValue;
			TagToValueMap[Tag] = NewValue;
			MarkItemDirty(Stack);
		}
		return;
	}
}

void FYcGameplayTagFloatStackContainer::SetStack(FGameplayTag Tag, float Value)
{
	if (!Tag.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag was passed to SetStack"), ELogVerbosity::Warning);
		return;
	}

	for (FYcGameplayTagFloatStack& Stack : Stacks)
	{
		if (Stack.Tag != Tag) continue;

		Stack.Value = Value;
		TagToValueMap[Tag] = Value;
		MarkItemDirty(Stack);
		return;
	}

	// Tag不存在，创建新条目
	FYcGameplayTagFloatStack& NewStack = Stacks.Emplace_GetRef(Tag, Value);
	MarkItemDirty(NewStack);
	TagToValueMap.Add(Tag, Value);
}

void FYcGameplayTagFloatStackContainer::ExportStacks(TArray<TPair<FGameplayTag, float>>& OutStacks) const
{
	OutStacks.Reset();
	OutStacks.Reserve(Stacks.Num());
	for (const FYcGameplayTagFloatStack& Stack : Stacks)
	{
		if (!Stack.Tag.IsValid())
		{
			continue;
		}
		OutStacks.Emplace(Stack.Tag, Stack.Value);
	}
}

void FYcGameplayTagFloatStackContainer::ImportStacks(const TArray<TPair<FGameplayTag, float>>& InStacks)
{
	Stacks.Reset();
	TagToValueMap.Reset();

	TMap<FGameplayTag, float> Merged;
	for (const TPair<FGameplayTag, float>& Pair : InStacks)
	{
		if (!Pair.Key.IsValid())
		{
			continue;
		}
		Merged.FindOrAdd(Pair.Key) += Pair.Value;
	}

	Stacks.Reserve(Merged.Num());
	for (const TPair<FGameplayTag, float>& Pair : Merged)
	{
		FYcGameplayTagFloatStack& NewStack = Stacks.Emplace_GetRef(Pair.Key, Pair.Value);
		TagToValueMap.Add(Pair.Key, Pair.Value);
		MarkItemDirty(NewStack);
	}
	MarkArrayDirty();
}

void FYcGameplayTagFloatStackContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (int32 Index : RemovedIndices)
	{
		const FGameplayTag Tag = Stacks[Index].Tag;
		TagToValueMap.Remove(Tag);
	}
}

void FYcGameplayTagFloatStackContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FYcGameplayTagFloatStack& Stack = Stacks[Index];
		TagToValueMap.Add(Stack.Tag, Stack.Value);
	}
}

void FYcGameplayTagFloatStackContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		const FYcGameplayTagFloatStack& Stack = Stacks[Index];
		TagToValueMap[Stack.Tag] = Stack.Value;
	}
}

// FYcGameplayTagFloatStackContainer
//////////////////////////////////////////////////////////////////////
