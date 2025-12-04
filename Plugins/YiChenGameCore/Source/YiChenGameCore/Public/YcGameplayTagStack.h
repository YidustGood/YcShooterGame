// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "YcGameplayTagStack.generated.h"

// 基于GameplayTag的数量栈FastArray实现

struct FYcGameplayTagStackContainer;
struct FNetDeltaSerializeInfo;

/**
 * 单条记录的结构体-(GameplayTag,StackCount)
 */
USTRUCT(BlueprintType)
struct YICHENGAMECORE_API FYcGameplayTagStack : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FYcGameplayTagStack()
	{
	}

	FYcGameplayTagStack(FGameplayTag InTag, int32 InStackCount)
		: Tag(InTag)
		  , StackCount(InStackCount)
	{
	}

	FString GetDebugString() const;

private:
	friend FYcGameplayTagStackContainer;

	// 标签
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, meta=(AllowPrivateAccess = "true"))
	FGameplayTag Tag;

	// 数量
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, meta=(AllowPrivateAccess = "true"))
	int32 StackCount = 0;
};
/** FGameplayTagStack的容器 */
USTRUCT(BlueprintType)
struct YICHENGAMECORE_API FYcGameplayTagStackContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	FYcGameplayTagStackContainer()
	{
	}
	
public:
	// 向指定Tag添加堆叠数量(如果StackCount<0，将不执行任何操作)
	void AddStack(FGameplayTag Tag, int32 StackCount);

	// 从指定Tag移除堆叠数量(如果StackCount<0，将不执行任何操作)
	void RemoveStack(FGameplayTag Tag, int32 StackCount);

	// 获得指定Tag的堆叠数量 (Tag不存在返回0)
	int32 GetStackCount(FGameplayTag Tag) const
	{
		return TagToCountMap.FindRef(Tag);
	}

	// 如果指定Tag存在于容器中则返回True
	bool ContainsTag(FGameplayTag Tag) const
	{
		return TagToCountMap.Contains(Tag);
	}

	//~FFastArraySerializer contract 这三个函数都只会在收到同步的客户端上被调用
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize); //客户端收到删除同步，应用同步数据前所调用的函数，用于在删除前进行一些数据处理，参数是即将被删除的元素下标列表
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize); //客户端收到添加数据同步并完成后调用的函数，参数是新增的元素下表列表
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize); //客户端收到修改数据同步并完成后调用的函数，参数是发生改变的元素下表列表
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FYcGameplayTagStack, FYcGameplayTagStackContainer>(Stacks, DeltaParms, *this);
	}

private:
	// 网络复制的数量栈数组
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, meta=(AllowPrivateAccess = "true"))
	TArray<FYcGameplayTagStack> Stacks;
	
	// 映射Stacks，用于加快查询，TMap查询速度>TArray,但是TMap在UE中目前不支持网络复制,所以在FastArray的三个网络复制辅助函数中进行手动操作保证数据一致性
	UPROPERTY(NotReplicated, BlueprintReadOnly, VisibleInstanceOnly, meta=(AllowPrivateAccess = "true"))
	TMap<FGameplayTag, int32> TagToCountMap;
};

template <>
struct TStructOpsTypeTraits<FYcGameplayTagStackContainer> : public TStructOpsTypeTraitsBase2<FYcGameplayTagStackContainer>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
