// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "DataRegistryId.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "YcItemInstanceId.h"
#include "YcInventoryManagerComponent.generated.h"

class UYcInventoryManagerComponent;
struct FYcInventoryItemDefinition;
class UYcInventoryItemInstance;
struct FYcInventoryItemList;
class AActor;

/**
 * 库存重排请求参数。
 * 由外部系统传入实例化负载，库存实现自行解释其含义。
 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcInventoryRelocationRequest
{
	GENERATED_BODY()

	/** 请求负载（推荐使用InstancedStruct承载具体上下文）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	FInstancedStruct Payload;
};

/**
 * 物品锚点驱动的库存重排负载。
 * 由调用方提供一个“锚点物品实例”，库存实现按该锚点解释重排语义。
 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcInventoryRelocationPayload_ItemScope
{
	GENERATED_BODY()

	/** 重排锚点物品实例。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Inventory)
	TObjectPtr<UYcInventoryItemInstance> AnchorItem = nullptr;
};

/**
 * 库存物品变化消息结构体
 * 用于通过GameplayMessageSubsystem广播物品数量变化
 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcInventoryItemChangeMessage
{
	GENERATED_BODY()
	// @TODO: 考虑基于标签的名称和拥有者角色来管理库存，而非直接暴露组件？
	/** 发生变化的ItemInstance所属的库存组件对象 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TObjectPtr<UActorComponent> InventoryOwner = nullptr;

	/** 发生变化的ItemInstance对象 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TObjectPtr<UYcInventoryItemInstance> ItemInstance = nullptr;

	/** 新的数量 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	int32 NewCount = 0;

	/** 变化的差量 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	int32 Delta = 0;
};

/**
 * 库存中的单个ItemInstance的包装结构体
 * 用于FastArray网络同步
 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcInventoryItemEntry : public FFastArraySerializerItem
{
	GENERATED_BODY()

	FYcInventoryItemEntry() {}

	FString GetDebugString() const;

private:
	friend FYcInventoryItemList;
	friend UYcInventoryManagerComponent;

	/** Item实例对象 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UYcInventoryItemInstance> Instance = nullptr;

	/** 物品堆叠数量 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, meta = (AllowPrivateAccess = "true"))
	int32 StackCount = 0;
	
	/** 最后一次修改前的数量（用于计算Delta） */
	UPROPERTY(NotReplicated, BlueprintReadOnly, VisibleInstanceOnly, meta = (AllowPrivateAccess = "true"))
	int32 LastObservedCount = INDEX_NONE;
};

/**
 * 库存Item列表结构体
 * 基于FastArray实现高效的网络同步
 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcInventoryItemList : public FFastArraySerializer
{
	GENERATED_BODY()

	FYcInventoryItemList() : OwnerComponent(nullptr) {}
	FYcInventoryItemList(UActorComponent* InOwnerComponent) : OwnerComponent(InOwnerComponent) {}

public:
	//~ Begin FFastArraySerializer Interface
	void PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize);
	void PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize);
	void PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize);
	//~ End FFastArraySerializer Interface

	bool NetDeltaSerialize(FNetDeltaSerializeInfo& DeltaParms)
	{
		return FFastArraySerializer::FastArrayDeltaSerialize<FYcInventoryItemEntry, FYcInventoryItemList>(Items, DeltaParms, *this);
	}

	/**
	 * 获取所有物品实例
	 * @return 物品实例数组
	 */
	TArray<UYcInventoryItemInstance*> GetAllItemInstance() const;

	/**
	 * 通过DataRegistry ID添加物品
	 * 内部会从DataRegistry获取物品定义并创建ItemInstance
	 * @param ItemRegistryId 物品在DataRegistry中的ID
	 * @param StackCount 堆叠数量
	 * @return 新创建的ItemInstance对象，失败返回nullptr
	 */ 
	UYcInventoryItemInstance* AddItem(const FDataRegistryId& ItemRegistryId, int32 StackCount, FYcItemInstanceId ForcedItemInstId = FYcItemInstanceId());

	/**
	 * 通过ItemInstance实例对象添加物品
	 * @param Instance 要添加的ItemInstance对象
	 * @param StackCount 堆叠数量
	 * @return 是否添加成功
	 */
	bool AddItem(UYcInventoryItemInstance* Instance, int32 StackCount);

	/**
	 * 移除Item实例对象
	 * @param Instance 要移除的对象
	 * @return 是否移除成功
	 */
	bool RemoveItem(UYcInventoryItemInstance* Instance);
	
	/**
	 * 在ItemsMap中通过ItemId快速查找Item数据条目
	 * @param ItemId 要查找的ItemId
	 * @param OutItemEntry 找到的Item数据条目
	 * @return 是否成功找到
	 */
	bool FindItemById(const FYcItemInstanceId& ItemId, FYcInventoryItemEntry& OutItemEntry);

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
	
	/**
	 * 生成新的物品实例唯一ID（GUID）。
	 * 该ID作为库存内索引主键，避免跨Inventory场景冲突。
	 */
	FYcItemInstanceId GenerateNewItemInstanceId(const FYcInventoryItemEntry& Item) const;
	
	friend UYcInventoryManagerComponent;
	
	/** 网络复制的Item列表 */
	UPROPERTY(BlueprintReadOnly, VisibleInstanceOnly, meta = (AllowPrivateAccess = "true"))
	TArray<FYcInventoryItemEntry> Items;
	
	/**
	 * 映射表，用于快速查询
	 * TMap查询速度>TArray，但TMap在UE中不支持网络复制
	 * 在FastArray的三个网络复制辅助函数中手动操作以保证数据一致性
	 */
	TMap<FYcItemInstanceId, FYcInventoryItemEntry*> ItemsMap;
	
	UPROPERTY(NotReplicated)
	TObjectPtr<UActorComponent> OwnerComponent;
};

template <>
struct TStructOpsTypeTraits<FYcInventoryItemList> : public TStructOpsTypeTraitsBase2<FYcInventoryItemList>
{
	enum { WithNetDeltaSerializer = true };
};


/**
 * 库存管理组件
 * 
 * 管理玩家或其他Actor的物品库存，提供添加、移除、查询物品的功能。
 * 基于FastArray实现高效的网络同步。
 * 
 * 物品定义通过DataRegistry管理，支持从多个数据源聚合查询。
 */
UCLASS(ClassGroup = (YiChenInventory), meta = (BlueprintSpawnableComponent), Blueprintable)
class YICHENINVENTORY_API UYcInventoryManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UYcInventoryManagerComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	//~ Begin UActorComponent Interface
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void ReadyForReplication() override;
	//~ End UActorComponent Interface
	
	//~=============================================================================
	// 物品添加
	
	/**
	 * 检查是否可以添加指定类型的物品
	 * 可在子类或蓝图中重写以实现自定义限制逻辑
	 * @param ItemDef 物品定义
	 * @param StackCount 数量
	 * @return 是否可以添加
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category = Inventory)
	bool CanAddItemDefinition(const FYcInventoryItemDefinition& ItemDef, int32 StackCount = 1);
	
	/**
	 * 通过DataRegistry ID添加物品
	 * @param ItemRegistryId 物品在DataRegistry中的ID
	 * @param StackCount 要添加的数量
	 * @return 新创建的ItemInstance对象，失败返回nullptr
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Inventory)
	UYcInventoryItemInstance* AddItem(const FDataRegistryId& ItemRegistryId, int32 StackCount = 1);

	/**
	 * 通过DataRegistry ID添加物品，并指定恢复用的ItemInstId
	 * 主要用于本地化存档恢复，常规运行请调用AddItem
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Inventory)
	UYcInventoryItemInstance* AddItemWithInstanceId(const FDataRegistryId& ItemRegistryId, FYcItemInstanceId ForcedItemInstId, int32 StackCount = 1);
	
	/**
	 * 通过物品实例对象添加物品
	 * @param ItemInstance 物品实例对象
	 * @param StackCount 数量
	 * @return 是否添加成功
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Inventory)
	bool AddItemInstance(UYcInventoryItemInstance* ItemInstance, int32 StackCount);
	
	//~=============================================================================
	// 物品移除
	
	/**
	 * 移除ItemInstance对象
	 * @param ItemInstance 要移除的目标对象
	 * @return 是否移除成功
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Inventory)
	bool RemoveItemInstance(UYcInventoryItemInstance* ItemInstance);

	/**
	 * 消费指定ItemInstance的堆叠数量
	 * @param ItemInstance 目标物品实例
	 * @param StackCount 要消费的数量（<=0视为失败）
	 * @return 是否成功消费
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Inventory)
	bool ConsumeItemInstance(UYcInventoryItemInstance* ItemInstance, int32 StackCount = 1);
	
	/**
	 * 消费库存物品
	 * 按照内存中的先后顺序挨个从库存中移除
	 * @param ItemDef 库存物品的定义
	 * @param NumToConsume 要消费的数量
	 * @return 是否成功消费指定数量
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Inventory)
	bool ConsumeItemsByDefinition(const FYcInventoryItemDefinition& ItemDef, int32 NumToConsume);
	
	//~=============================================================================
	// 物品查询

	/**
	 * 从Actor查找库存管理组件
	 * 查找顺序：Actor自身 -> Pawn/Controller互查 -> PlayerState
	 *
	 * @param Actor 要查找的Actor对象
	 * @return 找到的库存管理组件，如果不存在则返回nullptr
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Inventory)
	static UYcInventoryManagerComponent* FindInventoryManager(const AActor* Actor);

	/**
	 * 从物品实例反查其所属库存管理组件
	 * 通过 ItemInstance 的 Outer Actor 定位库存组件
	 *
	 * @param ItemInstance 物品实例
	 * @return 所属库存管理组件，如果不存在则返回nullptr
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = Inventory)
	static UYcInventoryManagerComponent* FindInventoryManagerByItem(const UYcInventoryItemInstance* ItemInstance);
	
	/**
	 * 获取当前库存中所有的ItemInstance对象
	 * @return ItemInstance对象列表
	 */
	UFUNCTION(BlueprintCallable, Category = Inventory, BlueprintPure = false)
	TArray<UYcInventoryItemInstance*> GetAllItemInstance() const;
	
	/**
	 * 获取第一个匹配目标ItemDef的ItemInstance对象
	 * @param ItemDef 需要查找匹配的ItemDef
	 * @return 找到的首个匹配ItemInstance对象，未找到返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = Inventory, BlueprintPure)
	UYcInventoryItemInstance* FindFirstItemInstByDefinition(const FYcInventoryItemDefinition& ItemDef) const;
	
	/**
	 * 查找某个物品定义在库存中有多少个ItemInstance
	 * 注意：这是实例数量，不是所有的StackCount总和
	 * @param ItemDef 目标ItemDef
	 * @return 与这个ItemDef关联的实例数量
	 */
	UFUNCTION(BlueprintCallable, Category = Inventory, BlueprintPure)
	int32 GetTotalItemCountByDefinition(const FYcInventoryItemDefinition& ItemDef);
	
	/**
	 * 获取ItemInstance的StackCount
	 * @param ItemInstance 目标ItemInstance
	 * @return ItemInstance StackCount
	 */
	UFUNCTION(BlueprintCallable, Category=Inventory, BlueprintPure)
	int32 GetStackCountByItemInstance(const UYcInventoryItemInstance* ItemInstance) const;
	
	/**
	 * 在ItemsMap中通过ItemId快速查找Item数据条目
	 * @param ItemId 要查找的ItemId
	 * @param OutItemEntry 找到的Item数据条目
	 * @return 是否成功找到
	 */
	UFUNCTION(BlueprintCallable, Category = Inventory, BlueprintPure)
	bool FindItemById(const FYcItemInstanceId& ItemId, FYcInventoryItemEntry& OutItemEntry);

	/**
	 * 校验库存是否可接收“回归”物品（如QuickBar移除、系统归还）。
	 * 默认实现返回true，具体库存类型可重写（如网格库存容量校验）。
	 */
	UFUNCTION(BlueprintNativeEvent, Category = Inventory)
	bool CanAcceptItemForReturn(UYcInventoryItemInstance* ItemInstance, FString& OutReason);

	/**
	 * 库存重排可行性校验。
	 * 默认实现返回true，具体库存类型可重写。
	 */
	UFUNCTION(BlueprintNativeEvent, Category = Inventory)
	bool CanApplyInventoryRelocation(const FYcInventoryRelocationRequest& Request, FString& OutReason);

	/**
	 * 执行库存重排准备。
	 * 默认实现返回true，具体库存类型可重写。
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category = Inventory)
	bool ApplyInventoryRelocation(const FYcInventoryRelocationRequest& Request, FString& OutReason);
	
private:
	/** 基于FastArray进行网络复制的库存物品列表 */
	UPROPERTY(VisibleInstanceOnly, Replicated, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
	FYcInventoryItemList ItemList;
};
