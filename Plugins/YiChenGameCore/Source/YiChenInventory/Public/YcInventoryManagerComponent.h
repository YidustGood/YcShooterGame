// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "YcInventoryManagerComponent.generated.h"

struct FYcInventoryItemDefinition;
class UYcInventoryManagerComponent;
class UYcInventoryItemInstance;
struct FYcInventoryItemList;

USTRUCT(BlueprintType)
struct FYcInventoryItemChangeMessage
{
	GENERATED_BODY()

	// @TODO: 考虑基于标签的名称和拥有者角色来管理库存，而非直接暴露组件？
	// 发生变化的ItemInstance所属的库存组件对象
	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	TObjectPtr<UActorComponent> InventoryOwner = nullptr;

	// 发生变化的ItemInstance对象
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TObjectPtr<UYcInventoryItemInstance> ItemInstance = nullptr;

	// 新的数量
	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	int32 NewCount = 0;

	// 变化的差量
	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	int32 Delta = 0;
};

/** 库存中的单个ItemInstance的包装结构体，FastArray的用法*/
USTRUCT(BlueprintType)
struct FYcInventoryItemEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FYcInventoryItemEntry()
	{
	}

	FString GetDebugString() const;

private:
	// 开启友元类，这样他们就可以访问private成员
	friend FYcInventoryItemList;
	friend UYcInventoryManagerComponent;

	/** Item实例对象 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UYcInventoryItemInstance> Instance = nullptr;

	/** 物品堆叠数量 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, meta=(AllowPrivateAccess = "true"))
	int32 StackCount = 0;
	
	/** 最后一次修改前的数量 */
	UPROPERTY(NotReplicated, BlueprintReadOnly, VisibleInstanceOnly, meta=(AllowPrivateAccess = "true"))
	int32 LastObservedCount = INDEX_NONE;
};

/** 库存Item列表结构体 */
USTRUCT(BlueprintType)
struct FYcInventoryItemList : public FFastArraySerializer
{
	GENERATED_BODY()

	FYcInventoryItemList()
		: OwnerComponent(nullptr)
	{
	}

	FYcInventoryItemList(UActorComponent* InOwnerComponent)
		: OwnerComponent(InOwnerComponent)
	{
	}

public:
	//~FFastArraySerializer contract
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~End of FFastArraySerializer contract

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FYcInventoryItemEntry, FYcInventoryItemList>(Items, DeltaParms, *this);
	}

	/**
	 * 获取所有物品
	 * @return 物品数组
	 */
	TArray<UYcInventoryItemInstance*> GetAllItemInstance() const;

	/**
	 * 通过ItemDef添加物品, 内部会判断创建ItemInst
	 * @param ItemDef Item的定义类型
	 * @param StackCount 堆叠数量
	 * @return 新增加的ItemInstance对象
	 */
	UYcInventoryItemInstance* AddItem(const FYcInventoryItemDefinition& ItemDef, const int32 StackCount);

	/**
	 * 通过ItemInst实例对象添加物品
	 * @param Instance 要添加的对象
	* @param StackCount 堆叠数量
	 */
	bool AddItem(UYcInventoryItemInstance* Instance, const int32 StackCount);


	/**
	 * 移除Item实例对象
	 * @param Instance 要移除的对象
	 */
	bool RemoveItem(UYcInventoryItemInstance* Instance);

private:
	/**
	 * 广播Item发生变化的消息
	 * @param ItemEntry 发生变化的ItemEntry
	 * @param OldCount 旧的数量
	 * @param NewCount 新的数量
	 */
	void BroadcastChangeMessage(const FYcInventoryItemEntry& ItemEntry, int32 OldCount, int32 NewCount) const;

	void AddItemToMap_Internal(FYcInventoryItemEntry& Item);
	void ChangeItemToMap_Internal(FYcInventoryItemEntry& Item);
	void RemoveItemToMap_Internal(const FYcInventoryItemEntry& Item);
	/** 同一个物品的第一份会占用ItemId,如果超过了堆叠数量,需要新建另一份来存储,此时就不能再延用ItemId了,所以可以使用这个函数来生成下一个不重复带后缀的ItemId作为使用*/
	FName GenerateNextItemId(const FYcInventoryItemEntry& Item) const;
	
	friend UYcInventoryManagerComponent;
	
	// 网络复制的Item列表
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, meta=(AllowPrivateAccess = "true"))
	TArray<FYcInventoryItemEntry> Items;
	
	// 映射Stacks，用于快速查询的TMap，TMap查询速度>TArray，但是TMap在UE中目前不支持网络复制，可以在FastArray的三个网络复制辅助函数中进行手动操作，以保证数据一致性
	TMap<FName, FYcInventoryItemEntry*> ItemsMap; // @TODO 考虑是否换成TMap<FName, TArray<FYcInventoryItemEntry*>>,以更优雅的加速处理同一ItemDef的Item
	
	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

template <>
struct TStructOpsTypeTraits<FYcInventoryItemList> : public TStructOpsTypeTraitsBase2<FYcInventoryItemList>
{
	enum { WithNetDeltaSerializer = true };
};


UCLASS(ClassGroup=(YiChenGameCore), meta=(BlueprintSpawnableComponent), Blueprintable)
class YICHENINVENTORY_API UYcInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UYcInventoryManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	//~UObject interface
	// 复制子UObject对象函数,5.1后使用新方法AddReplicatedSubObject, 无需重写ReplicateSubobjects
	virtual void ReadyForReplication() override;
	//~End of UObject interface
	
	/**
	 * 检查是否可以添加指定类型的ItemDef
	 * @param ItemDef Item的定义类型
	 * @param StackCount 数量
	 * @return 是否添加成功
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category=Inventory)
	bool CanAddItemDefinition(const FYcInventoryItemDefinition& ItemDef, int32 StackCount = 1);
	
	/**
	 * 根据ItemDef向InventoryItemList中添加物品
	 * @param ItemDef ItemDefinition
	 * @param StackCount 要添加的数量
	 * @return 
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	UYcInventoryItemInstance* AddItemByDefinition(const FYcInventoryItemDefinition& ItemDef, int32 StackCount = 1);
	
	/**
	 * 通过物品实例对象添加物品
	 * @param ItemInstance 物品实例对象
	 * @param StackCount 数量
	 * @return 是否添加成功
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	bool AddItemInstance(UYcInventoryItemInstance* ItemInstance, const int32 StackCount);
	
	/**
	 * 移除从InventoryItemList 移除 ItemInstance对象
	 * @param ItemInstance 要移除的目标对象
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Inventory)
	bool RemoveItemInstance(UYcInventoryItemInstance* ItemInstance);
	
	/**
	 * 获取当前库存中所有的ItemInstance对象
	 * @return 当前库存中ItemInstance对象列表
	 */
	UFUNCTION(BlueprintCallable, Category=Inventory, BlueprintPure=false)
	TArray<UYcInventoryItemInstance*> GetAllItemInstance() const;
	
	/**
	 * 获取第一个匹配目标ItemDef的ItemInstance对象
	 * @param ItemDef 需要查找匹配的ItemDef
	 * @return 找到的首个匹配ItemInstance对象
	 */
	UFUNCTION(BlueprintCallable, Category=Inventory, BlueprintPure)
	UYcInventoryItemInstance* FindFirstItemInstByDefinition(const FYcInventoryItemDefinition& ItemDef) const;
	
	/**
	 * 查找某个物品的类型在库存中有多少个ItemInstance(查找InventoryItemList中有多少个对象是与目标ItemDefClass关联的)
	 * @param ItemDef 目标ItemDef
	 * @return 与这个ItemDef关联的对象数量, 注意这不是所有的StackCount
	 */
	UFUNCTION(BlueprintCallable, Category=Inventory, BlueprintPure)
	int32 GetTotalItemCountByDefinition(const FYcInventoryItemDefinition& ItemDef) const;
	
	/**
	 * 消费库存物品(按照内存中的先后顺序挨个从库存中移除, 目前并非小号物品的StackCount数量)
	 * @param ItemDef 库存物品的类型
	 * @param NumToConsume 
	 * @return 
	 */
	bool ConsumeItemsByDefinition(const FYcInventoryItemDefinition& ItemDef, int32 NumToConsume);
private:
	// 基于FastArray进行网络复制的库存物品列表
	UPROPERTY(VisibleInstanceOnly, Replicated, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	FYcInventoryItemList ItemList;
};
