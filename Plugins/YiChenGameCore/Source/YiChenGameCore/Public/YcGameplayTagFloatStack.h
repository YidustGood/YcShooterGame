// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "YcGameplayTagFloatStack.generated.h"

/**
 * 基于GameplayTag的浮点数栈FastArray实现
 * 用于存储物品实例的动态浮点数数据（如护甲耐久、武器充能等）
 */

struct FYcGameplayTagFloatStackContainer;
struct FNetDeltaSerializeInfo;

/**
 * 单条浮点数记录结构体 - (GameplayTag, FloatValue)
 */
USTRUCT(BlueprintType)
struct YICHENGAMECORE_API FYcGameplayTagFloatStack : public FFastArraySerializerItem
{
    GENERATED_BODY()

    FYcGameplayTagFloatStack() {}

    FYcGameplayTagFloatStack(FGameplayTag InTag, float InValue)
        : Tag(InTag), Value(InValue)
    {
    }

    FString GetDebugString() const;

    /** 获取标签 */
    FGameplayTag GetTag() const { return Tag; }
    
    /** 获取浮点数值 */
    float GetValue() const { return Value; }

private:
    friend FYcGameplayTagFloatStackContainer;

    /** 标签 */
    UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, meta = (AllowPrivateAccess = "true"))
    FGameplayTag Tag;

    /** 浮点数值 */
    UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, meta = (AllowPrivateAccess = "true"))
    float Value = 0.0f;
};

/**
 * 浮点数TagStack容器
 * 支持网络复制，用于存储浮点数类型的动态物品数据
 */
USTRUCT(BlueprintType)
struct YICHENGAMECORE_API FYcGameplayTagFloatStackContainer : public FFastArraySerializer
{
    GENERATED_BODY()

    FYcGameplayTagFloatStackContainer() {}

public:
    /** 
     * 向指定Tag添加浮点数值
     * @param Tag 标签
     * @param Value 要添加的值（如果Value <= 0，将不执行任何操作）
     */
    void AddStack(FGameplayTag Tag, float Value);

    /**
     * 从指定Tag移除浮点数值
     * @param Tag 标签
     * @param Value 要移除的值（如果Value <= 0，将不执行任何操作）
     */
    void RemoveStack(FGameplayTag Tag, float Value);

    /**
     * 直接设置指定Tag的浮点数值
     * @param Tag 标签
     * @param Value 要设置的值
     */
    void SetStack(FGameplayTag Tag, float Value);

    /**
     * 获取指定Tag的浮点数值
     * @param Tag 标签
     * @return 标签对应的值，如果Tag不存在返回0.0f
     */
    float GetStackValue(FGameplayTag Tag) const
    {
        return TagToValueMap.FindRef(Tag);
    }

    /**
     * 检查指定Tag是否存在于容器中
     * @param Tag 标签
     * @return 如果存在返回true
     */
    bool ContainsTag(FGameplayTag Tag) const
    {
        return TagToValueMap.Contains(Tag);
    }

    //~FFastArraySerializer contract 这三个函数都只会在收到同步的客户端上被调用
    void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
    void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
    //~End of FFastArraySerializer contract

    bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
    {
        return FFastArraySerializer::FastArrayDeltaSerialize<FYcGameplayTagFloatStack, FYcGameplayTagFloatStackContainer>(Stacks, DeltaParms, *this);
    }

private:
    /** 网络复制的浮点数栈数组 */
    UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, meta = (AllowPrivateAccess = "true"))
    TArray<FYcGameplayTagFloatStack> Stacks;

    /** 
     * 映射Stacks，用于加快查询
     * TMap查询速度>TArray，但是TMap在UE中目前不支持网络复制
     * 所以在FastArray的三个网络复制辅助函数中进行手动操作保证数据一致性
     */
    UPROPERTY(NotReplicated)
    TMap<FGameplayTag, float> TagToValueMap;
};

template <>
struct TStructOpsTypeTraits<FYcGameplayTagFloatStackContainer> : public TStructOpsTypeTraitsBase2<FYcGameplayTagFloatStackContainer>
{
    enum
    {
        WithNetDeltaSerializer = true,
    };
};
