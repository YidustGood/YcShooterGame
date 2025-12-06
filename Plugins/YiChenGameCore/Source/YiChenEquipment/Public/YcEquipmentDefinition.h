// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "StructUtils/InstancedStruct.h"
#include "YcEquipmentDefinition.generated.h"

class UYcEquipmentInstance;
class UYcAbilitySet;
class UYcAbilitySystemComponent;

/**
 * 装备片段基类
 * 为装备定义提供扩展功能，支持灵活组合不同的装备特性
 * 
 * 使用方式：
 * 1. 创建继承自 FYcEquipmentFragment 的结构体
 * 2. 在装备定义的 Fragments 数组中添加
 * 3. 在装备实例中通过 GetTypedFragment<T>() 查询
 */
USTRUCT(BlueprintType)
struct YICHENEQUIPMENT_API FYcEquipmentFragment
{
	GENERATED_BODY()
	
	virtual ~FYcEquipmentFragment() = default;
	
	/**
	 * 装备实例创建时的回调
	 * 用于初始化装备实例的运行时数据
	 * 
	 * @param Instance 新创建的装备实例
	 */
	virtual void OnEquipmentInstanceCreated(UYcEquipmentInstance* Instance) const {}
	
	/**
	 * 装备时的回调
	 * 在装备被装备到角色身上时调用
	 * 
	 * @param Instance 装备实例
	 */
	virtual void OnEquipped(UYcEquipmentInstance* Instance) const {}
	
	/**
	 * 卸下时的回调
	 * 在装备被卸下时调用
	 * 
	 * @param Instance 装备实例
	 */
	virtual void OnUnequipped(UYcEquipmentInstance* Instance) const {}
	
	/**
	 * 获取调试字符串
	 * 用于打印调试信息
	 */
	virtual FString GetDebugString() const
	{
		return FString(TEXT("FYcEquipmentFragment"));
	}
};

/** 用于生成并附加到玩家角色身上的Actor信息结构体，包含了要生成的Actor类和附加的Socket以及附加的Transform等信息 */
USTRUCT(Blueprintable)
struct YICHENEQUIPMENT_API FYcEquipmentActorToSpawn
{
	GENERATED_BODY()

	FYcEquipmentActorToSpawn() : bReplicateActor(true)
	{
	}

	// 指针和引用类型放在前面（8字节对齐）, 通过合理的声明顺序降低内存对齐填充带来的内存浪费
	/* 要附加到玩家角色身上的Actor类(需开启网络复制才能让所有人看到) */
	UPROPERTY(EditAnywhere, Category=Equipment, BlueprintReadWrite)
	TSoftClassPtr<AActor> ActorToSpawn;

	// 较大结构体放在中间
	/* 附加变换 */
	UPROPERTY(EditAnywhere, Category=Equipment, BlueprintReadWrite)
	FTransform AttachTransform;

	// 数组类型
	/* 这个Actor的Tags，方便查找 */
	UPROPERTY(EditAnywhere, Category=Equipment, BlueprintReadWrite)
	TArray<FName> ActorTags;

	// 较小类型分组排列
	/* 父组件Tag，会根据这个Tag去查找父组件对象 */
	UPROPERTY(EditAnywhere, Category=Equipment, BlueprintReadWrite)
	FName ParentComponentTag;

	/* 附加的Socket名称 */
	UPROPERTY(EditAnywhere, Category=Equipment, BlueprintReadWrite)
	FName AttachSocket;

	// 布尔标志放在最后（1字节）
	/* 是否为复制的Actor，如是则在服务器上生成，否则在本地生成 */
	UPROPERTY(EditAnywhere, Category=Equipment, BlueprintReadWrite)
	uint8 bReplicateActor : 1;
};

/**
 * 装备定义, 例如可以用来定义附加到角色身上的武器、护甲、头盔等类似的装备
 * 
 * Fragments: 支持 Fragment 扩展系统，可以灵活组合不同的装备特性
 * ActorsToSpawn: 支持Actor生成附加功能
 * AbilitySetsToGrant: 支持装备技能功能
 */
USTRUCT(BlueprintType)
struct YICHENEQUIPMENT_API FYcEquipmentDefinition : public FTableRowBase
{
	GENERATED_BODY()
	
	// 装备的实例类
	UPROPERTY(EditDefaultsOnly, Category=Equipment, BlueprintReadOnly)
	TSubclassOf<UYcEquipmentInstance> InstanceType;
	
	// 当装备被装备时,需要生成并附加到玩家角色身上的Actor列表, 例如: 装备了一把武器, 会在生成武器并附加到角色手上
	UPROPERTY(EditDefaultsOnly, Category=Equipment, BlueprintReadOnly, meta=(TitleProperty="{ActorToSpawn} -> {ParentComponentTag}::{AttachSocket}"))
	TArray<FYcEquipmentActorToSpawn> ActorsToSpawn;
	
	// 当这个装备被装备时所需要授予相关玩家角色的技能集合,包含GA GE AS
	UPROPERTY(EditDefaultsOnly, Category=Equipment, BlueprintReadOnly)
	TArray<TObjectPtr<const UYcAbilitySet>> AbilitySetsToGrant;
	
	// 装备片段，用于扩展装备的功能和属性
	// 支持灵活组合不同的装备特性，无需继承链
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FYcEquipmentFragment>> Fragments;
	
	/**
	 * 获取特定类型的 Fragment
	 * 只能在 C++ 中使用，效率最高
	 * 
	 * @return 找到的 Fragment 指针，如果不存在则返回 nullptr
	 * 
	 * 使用示例：
	 * const FEquipmentFragment_WeaponStats* WeaponStats = 
	 *     EquipmentDef.GetTypedFragment<FEquipmentFragment_WeaponStats>();
	 */
	template <typename FragmentType>
	const FragmentType* GetTypedFragment() const
	{
		for (const auto& Fragment : Fragments)
		{
			const FragmentType* TypedFragment = Fragment.GetPtr<FragmentType>();
			if (TypedFragment != nullptr) return TypedFragment;
		}
		return nullptr;
	}
};