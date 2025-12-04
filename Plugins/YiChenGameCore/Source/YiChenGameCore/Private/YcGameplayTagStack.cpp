// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcGameplayTagStack.h"

//////////////////////////////////////////////////////////////////////
// FYcGameplayTagStack

FString FYcGameplayTagStack::GetDebugString() const
{
	return FString::Printf(TEXT("%sx%d"), *Tag.ToString(), StackCount);
}

// FYcGameplayTagStack
//////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////
// FYcGameplayTagStackContainer

void FYcGameplayTagStackContainer::AddStack(FGameplayTag Tag, int32 StackCount)
{
	if (!Tag.IsValid()) // 检查标签有效性
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag was passed to AddStack"), ELogVerbosity::Warning);
		return;
	}
	if (StackCount <= 0) return; // 检查添加的数量有效性

	for (FYcGameplayTagStack& Stack : Stacks) // 遍历列表查找出对应Tag结构体，并修改值
	{
		if (Stack.Tag != Tag) continue;

		const int32 NewCount = Stack.StackCount + StackCount; // 计算新增后的数量
		Stack.StackCount = NewCount; // 设置新数量
		TagToCountMap[Tag] = NewCount; // 将新数量同步设置到TMap中，以便查询
		MarkItemDirty(Stack); // 标记为脏，让新数据可以进行网络同步
		return;
	}

	//如果这个Tag还不存在于列表中，那么根据参数在末尾构造一个元素
	FYcGameplayTagStack& NewStack = Stacks.Emplace_GetRef(Tag, StackCount);
	MarkItemDirty(NewStack);
	TagToCountMap.Add(Tag, StackCount); //同步到TMap
}

void FYcGameplayTagStackContainer::RemoveStack(FGameplayTag Tag, int32 StackCount)
{
	if (!Tag.IsValid())
	{
		FFrame::KismetExecutionMessage(TEXT("An invalid tag was passed to RemoveStack"), ELogVerbosity::Warning);
		return;
	}
	//@TODO: Should we error if you try to remove a stack that doesn't exist or has a smaller count?
	// 如果我们尝试删除一个不存在的Stack或者现存StackCount小于要删除的StackCount,我们是否需要抛出错误提示或者通知
	if (StackCount <= 0) return;

	for (auto It = Stacks.CreateIterator(); It; ++It)
	{
		FYcGameplayTagStack& Stack = *It;
		if (Stack.Tag != Tag) continue;

		if (Stack.StackCount <= StackCount)
		{
			// 现存数量<要删除的数量，所以直接把这个Tag从容器中移除
			It.RemoveCurrent();
			TagToCountMap.Remove(Tag);	// TMap同步移除
			MarkArrayDirty();	// 列表长度发生变化需要调用MarkArrayDirty()，才能触发网络同步
		}
		else
		{
			// 删除指定数量
			const int32 NewCount = Stack.StackCount - StackCount;	
			Stack.StackCount = NewCount;
			TagToCountMap[Tag] = NewCount;
			MarkItemDirty(Stack);
		}
		return;
	}
}

void FYcGameplayTagStackContainer::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	// 由于TMap不可复制，所以移除Tag的时候，TMap中映射的Tag需要我们收到预删除通知时手动进行清理，以保证与服务器达成同步，后面的添加修改同理，在收到相应同步后，从Stacks列表中取得数据并同步设置到TMap中
	for (int32 Index : RemovedIndices)
	{
		const FGameplayTag Tag = Stacks[Index].Tag;
		TagToCountMap.Remove(Tag);
	}
}

void FYcGameplayTagStackContainer::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FYcGameplayTagStack& Stack = Stacks[Index];
		TagToCountMap.Add(Stack.Tag, Stack.StackCount);
	}
}

void FYcGameplayTagStackContainer::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		const FYcGameplayTagStack& Stack = Stacks[Index];
		TagToCountMap[Stack.Tag] = Stack.StackCount;
	}
}

// FYcGameplayTagStackContainer
//////////////////////////////////////////////////////////////////////