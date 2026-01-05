// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcAbilitySet.h"
#include "YcEquipmentDefinition.h"
#include "YcEquipmentInstance.h"
#include "Components/PawnComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "YcEquipmentManagerComponent.generated.h"

struct FYcEquipmentList;
class UYcEquipmentManagerComponent;
class UYcInventoryItemInstance;

// ============================================================================
// FYcEquipmentEntry - 装备条目
// ============================================================================

/** 
 * 装备条目结构体
 * 表示一个已创建的装备实例，包含装备实例、所属物品、已授予的技能句柄
 */
USTRUCT(BlueprintType)
struct FYcEquipmentEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FYcEquipmentEntry() = default;

	FString GetDebugString() const;

private:
	friend FYcEquipmentList;
	friend UYcEquipmentManagerComponent;
	
	/** 装备实例对象 */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UYcEquipmentInstance> Instance;
	
	/** 装备所属的库存物品实例 */
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UYcInventoryItemInstance> OwnerItemInstance;

	/** 
	 * 已授予的技能集句柄信息
	 * 不参与网络复制，只在服务器上存在，因为只有服务器能掌管技能的授予/收回
	 */
	UPROPERTY(NotReplicated)
	FYcAbilitySet_GrantedHandles GrantedHandles;
};

// ============================================================================
// FYcEquipmentList - 装备列表
// ============================================================================

/** 
 * 装备列表
 * 使用 FastArraySerializer 实现高效的网络同步
 * 
 * 语义：存储所有已创建的装备实例（包括已装备和未装备的）
 */
USTRUCT(BlueprintType)
struct FYcEquipmentList : public FFastArraySerializer
{
	GENERATED_BODY()

	FYcEquipmentList() : OwnerComponent(nullptr) {}
	FYcEquipmentList(UActorComponent* InOwnerComponent) : OwnerComponent(InOwnerComponent) {}
	
	// ========================================================================
	// FFastArraySerializer 接口
	// ========================================================================
	
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FYcEquipmentEntry, FYcEquipmentList>(Entries, DeltaParms, *this);
	}

	// ========================================================================
	// 装备管理接口（仅服务器）
	// ========================================================================
	
	/**
	 * [Server Only] 创建装备实例
	 * 创建装备实例并生成Actors（隐藏状态），但不装备（不授予GA）
	 * 
	 * @param EquipmentDef 装备定义
	 * @param ItemInstance 所属的库存物品实例
	 * @return 创建的装备实例
	 */
	UYcEquipmentInstance* CreateEntry(const FYcEquipmentDefinition& EquipmentDef, UYcInventoryItemInstance* ItemInstance);
	
	/**
	 * [Server Only] 装备
	 * 授予GA，显示Actors，设置状态为Equipped
	 * 
	 * @param Instance 要装备的装备实例
	 */
	void EquipEntry(UYcEquipmentInstance* Instance);
	
	/**
	 * [Server Only] 卸下装备
	 * 移除GA，隐藏Actors，设置状态为Unequipped
	 * 
	 * @param Instance 要卸下的装备实例
	 */
	void UnequipEntry(UYcEquipmentInstance* Instance);
	
	/**
	 * [Server Only] 销毁装备实例
	 * 销毁Actors，移除GA（如果有），从列表中移除
	 * 
	 * @param Instance 要销毁的装备实例
	 */
	void DestroyEntry(UYcEquipmentInstance* Instance);

private:
	UYcAbilitySystemComponent* GetAbilitySystemComponent() const;
	FYcEquipmentEntry* FindEntry(UYcEquipmentInstance* Instance);

	friend UYcEquipmentManagerComponent;
	
	/** 装备条目列表 */
	UPROPERTY(VisibleAnywhere)
	TArray<FYcEquipmentEntry> Entries;

	UPROPERTY(NotReplicated, VisibleAnywhere)
	TObjectPtr<UActorComponent> OwnerComponent;
};

template <>
struct TStructOpsTypeTraits<FYcEquipmentList> : public TStructOpsTypeTraitsBase2<FYcEquipmentList>
{
	enum { WithNetDeltaSerializer = true };
};

// ============================================================================
// UYcEquipmentManagerComponent - 装备管理组件
// ============================================================================

/**
 * ============================================================================
 * 装备管理组件 (Equipment Manager Component)
 * ============================================================================
 * 
 * 管理角色的装备实例，提供创建、装备、卸下、销毁装备的能力。
 * 通常挂载到玩家控制的角色上。
 * 
 * ============================================================================
 * 核心设计：职责分离
 * ============================================================================
 * 
 * 本组件采用"职责分离"设计，将装备的生命周期管理与状态管理分开：
 * 
 * 1. 生命周期管理：CreateEquipment / DestroyEquipment
 *    - 控制装备实例的创建和销毁
 *    - 控制Actors的生成和销毁
 * 
 * 2. 状态管理：EquipItem / UnequipItem
 *    - 控制装备的装备/卸下状态
 *    - 控制GA的授予/移除
 *    - 控制Actors的显示/隐藏
 * 
 * 这种设计的优势：
 * - 支持预创建：物品加入QuickBar时就创建装备实例，消除首次装备延迟
 * - 网络开销小：状态切换只需同步一个枚举值，而不是整个Entry的增删
 * - 逻辑清晰：创建/销毁 与 装备/卸下 是两个独立的概念
 * 
 * ============================================================================
 * 典型使用流程
 * ============================================================================
 * 
 * 1. 物品加入QuickBar时：
 *    CreateEquipment() → 创建实例，生成Actors（隐藏），State=Unequipped
 * 
 * 2. 玩家装备武器时：
 *    EquipItem() → 授予GA，显示Actors，State=Equipped
 * 
 * 3. 玩家切换武器时：
 *    UnequipItem(旧) → 移除GA，隐藏Actors，State=Unequipped
 *    EquipItem(新) → 授予GA，显示Actors，State=Equipped
 * 
 * 4. 物品从QuickBar移除时：
 *    DestroyEquipment() → 销毁Actors，销毁实例
 * 
 * ============================================================================
 */
UCLASS(ClassGroup=(YiChenGameCore), meta=(BlueprintSpawnableComponent), Blueprintable, BlueprintType)
class YICHENEQUIPMENT_API UYcEquipmentManagerComponent : public UPawnComponent
{
	GENERATED_BODY()
	
public:
	UYcEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void InitializeComponent() override;
	virtual void UninitializeComponent() override;
	virtual void ReadyForReplication() override;
	
	// ========================================================================
	// 主要接口（仅服务器）
	// ========================================================================
	
	/**
	 * [Server Only] 创建装备实例
	 * 创建装备实例并生成Actors（隐藏状态），但不装备
	 * 
	 * @param EquipmentDef 装备定义
	 * @param ItemInstance 所属的库存物品实例
	 * @return 创建的装备实例，失败返回nullptr
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	UYcEquipmentInstance* CreateEquipment(const FYcEquipmentDefinition& EquipmentDef, UYcInventoryItemInstance* ItemInstance);
	
	/**
	 * [Server Only] 装备
	 * 授予GA，显示Actors，设置状态为Equipped
	 * 
	 * @param EquipmentInstance 要装备的装备实例
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	void EquipItem(UYcEquipmentInstance* EquipmentInstance);
	
	/**
	 * [Server Only] 卸下装备
	 * 移除GA，隐藏Actors，设置状态为Unequipped
	 * 
	 * @param EquipmentInstance 要卸下的装备实例
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	void UnequipItem(UYcEquipmentInstance* EquipmentInstance);
	
	/**
	 * [Server Only] 销毁装备实例
	 * 销毁Actors，从列表中移除
	 * 
	 * @param EquipmentInstance 要销毁的装备实例
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	void DestroyEquipment(UYcEquipmentInstance* EquipmentInstance);
	
	// ========================================================================
	// 查询接口
	// ========================================================================
	
	/**
	 * 根据物品实例查找装备实例
	 * @param ItemInstance 库存物品实例
	 * @return 对应的装备实例，如果不存在则返回nullptr
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	UYcEquipmentInstance* FindEquipmentByItem(UYcInventoryItemInstance* ItemInstance) const;
	
	/**
	 * 获取当前已装备的装备实例
	 * @return 当前已装备的装备实例，如果没有则返回nullptr
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	UYcEquipmentInstance* GetEquippedItem() const;
	
	/** 
	 * 返回给定装备类型的第一个实例
	 * @param InstanceType 装备实例类型
	 * @return 找到的装备实例，如果没有则返回nullptr
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	UYcEquipmentInstance* GetFirstInstanceOfType(TSubclassOf<UYcEquipmentInstance> InstanceType) const;

	/**
	 * 返回给定装备类型的所有装备实例
	 * @param InstanceType 装备实例类型
	 * @return 找到的装备实例数组
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	TArray<UYcEquipmentInstance*> GetEquipmentInstancesOfType(TSubclassOf<UYcEquipmentInstance> InstanceType) const;

	/** 模板版本：获取指定类型的第一个装备实例 */
	template <typename T>
	T* GetFirstInstanceOfType()
	{
		return Cast<T>(GetFirstInstanceOfType(T::StaticClass()));
	}
	
private:
	friend struct FYcEquipmentList;
	
	/** 装备列表 - 存储所有已创建的装备实例 */
	UPROPERTY(Replicated, VisibleAnywhere)
	FYcEquipmentList EquipmentList;
};
