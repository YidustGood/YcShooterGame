// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "YcResistanceContainer.generated.h"

struct FYcResistanceContainer;
struct FNetDeltaSerializeInfo;

/**
 * 单条抗性记录
 * 存储 (ResistanceTag, Value) 映射
 * 
 * 示例：
 *   Tag: Resistance.Fire
 *   Value: 0.3 (表示 30% 火焰抗性)
 */
USTRUCT(BlueprintType)
struct YICHENDAMAGE_API FYcResistanceEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FYcResistanceEntry()
	{
	}

	FYcResistanceEntry(FGameplayTag InTag, float InValue)
		: Tag(InTag)
		, Value(InValue)
	{
	}

	FString GetDebugString() const;

private:
	friend FYcResistanceContainer;

	/** 抗性标签（如 Resistance.Fire） */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, meta = (AllowPrivateAccess = "true"))
	FGameplayTag Tag;

	/** 抗性值（0.0 ~ 1.0，表示 0% ~ 100% 减伤） */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, meta = (AllowPrivateAccess = "true"))
	float Value = 0.0f;
};

/**
 * 抗性容器
 * 
 * 基于 FastArray 实现的高效抗性存储：
 * - FastArray 支持增量网络复制
 * - TMap 缓存提供 O(1) 查询速度
 * 
 * 使用方式：
 *   container.SetResistance(FGameplayTag::RequestGameplayTag("Resistance.Fire"), 0.3f);
 *   float fireRes = container.GetResistance(FGameplayTag::RequestGameplayTag("Resistance.Fire"));
 */
USTRUCT(BlueprintType)
struct YICHENDAMAGE_API FYcResistanceContainer : public FFastArraySerializer
{
	GENERATED_BODY()

	FYcResistanceContainer()
	{
	}

public:
	// -------------------------------------------------------------------
	// 抗性操作接口
	// -------------------------------------------------------------------

	/**
	 * 设置指定标签的抗性值
	 * @param Tag 抗性标签
	 * @param InValue 抗性值（自动限制在 0.0 ~ MaxResistance）
	 */
	void SetResistance(FGameplayTag Tag, float InValue);

	/**
	 * 增加指定标签的抗性值
	 * @param Tag 抗性标签
	 * @param Delta 增量值（可为负数）
	 */
	void AddResistance(FGameplayTag Tag, float Delta);

	/**
	 * 移除指定标签的抗性
	 * @param Tag 抗性标签
	 */
	void RemoveResistance(FGameplayTag Tag);

	/**
	 * 获取指定标签的抗性值
	 * @param Tag 抗性标签
	 * @return 抗性值，不存在返回 0.0
	 */
	float GetResistance(FGameplayTag Tag) const
	{
		return TagToValueMap.FindRef(Tag);
	}

	/**
	 * 检查是否存在指定标签的抗性
	 */
	bool ContainsResistance(FGameplayTag Tag) const
	{
		return TagToValueMap.Contains(Tag);
	}

	/**
	 * 获取所有抗性标签
	 */
	TArray<FGameplayTag> GetAllResistanceTags() const;

	/**
	 * 清空所有抗性
	 */
	void ClearAllResistances();

	// -------------------------------------------------------------------
	// 配置
	// -------------------------------------------------------------------

	/** 最大抗性值（默认 0.8，避免完全免疫） */
	float MaxResistance = 0.8f;

	/** 最小抗性值（默认 0.0） */
	float MinResistance = 0.0f;

	// -------------------------------------------------------------------
	// FFastArraySerializer 接口
	// -------------------------------------------------------------------

	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FYcResistanceEntry, FYcResistanceContainer>(Entries, DeltaParms, *this);
	}

private:
	/** 网络复制的抗性数组 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, meta = (AllowPrivateAccess = "true"))
	TArray<FYcResistanceEntry> Entries;

	/** TMap 缓存，用于 O(1) 查询（不同步） */
	UPROPERTY(NotReplicated)
	TMap<FGameplayTag, float> TagToValueMap;
};

/** 启用网络增量序列化 */
template <>
struct TStructOpsTypeTraits<FYcResistanceContainer> : public TStructOpsTypeTraitsBase2<FYcResistanceContainer>
{
	enum
	{
		WithNetDeltaSerializer = true,
	};
};
