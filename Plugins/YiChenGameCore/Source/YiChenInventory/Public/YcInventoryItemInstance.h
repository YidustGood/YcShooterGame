// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcInventoryItemDefinition.h"
#include "YiChenGameCore/Public/YcReplicableObject.h"
#include "YcGameplayTagStack.h"
#include "YcInventoryItemInstance.generated.h"

class UYcInventoryManagerComponent;

/**
 * 根据ItemDefinition创建的Item的实例对象,支持网络复制,由InventoryManagerComponent持有(InventoryItemList,使用FastArray进行网络同步)
 * 基于FastArray提供了高效的网络同步TagsStack,可以自由的以GameplayTag为单位存储StackCount
 */
UCLASS()
class YICHENINVENTORY_API UYcInventoryItemInstance : public UYcReplicableObject
{
	GENERATED_BODY()
public:
	UYcInventoryItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual bool IsSupportedForNetworking() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	/**
	 * 在Instance所属的ItemDef上查找指定类型的Fragment
	 * @param FragmentStructType 目标Fragment结构类型
	 * @return 查找到的Fragment结构体数据
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	TInstancedStruct<FYcInventoryItemFragment> FindItemFragment(const UScriptStruct* FragmentStructType);
	
	// 向指定Tag添加堆叠数量(如果StackCount<0，将不执行任何操作)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	// 向指定Tag移除堆叠数量(如果StackCount<0，将不执行任何操作)
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	// 获得指定Tag的堆叠数量 (Tag不存在返回0)
	UFUNCTION(BlueprintCallable, Category=Inventory)
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	// 如果指定Tag StackCount>0,则返回true
	UFUNCTION(BlueprintCallable, Category=Inventory)
	bool HasStatTag(FGameplayTag Tag) const;
	
	// 从内部ItemDef指针获取ItemDef数据
	UFUNCTION(BlueprintCallable, Category=Inventory, DisplayName="GetItemDef")
	bool K2_GetItemDef(FYcInventoryItemDefinition& OutItemDef);
	
	// 获取该物品实例所属的InventoryManagerComponent
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UYcInventoryManagerComponent* GetInventoryManager() const;

protected:
	/** 当ItemDefRowHandle值到达客户端后会解析并缓存ItemDef指针 */
	UFUNCTION()
	void OnRep_ItemDefRowHandle(FDataTableRowHandle LastItemDefRowHandle);
	
private:
	friend struct FYcInventoryItemFragment;
	friend struct FYcInventoryItemList;
	
	/* 设置Instance所属的ItemDef, 通常是在通过ItemDef创建出Instance后立刻进行设置, 设置后会立刻解析并缓存ItemDef指针 **/
	void SetItemDefRowHandle(const FDataTableRowHandle& InItemDefRowHandle);
	
	void SetItemInstId(const FName NewId);

	/* 这个ItemInstance所属的物品定义类型 **/
	UPROPERTY(ReplicatedUsing=OnRep_ItemDefRowHandle, BlueprintReadOnly, VisibleInstanceOnly, meta = (AllowPrivateAccess = "true", RowType=FYcInventoryItemDefinition))
	FDataTableRowHandle ItemDefRowHandle;
	
	/** 物品定义指针, 在SetItemDefRowHandle函数调用时自动通过ItemDefRowHandle进行设置, 该指针指向的就是DataTable中的ItemDef */
	FYcInventoryItemDefinition* ItemDef;
	
	/* 这个ItemInstance专属的Id, 因为同一个物品存在多个不同实例,只有第一份实例能直接使用ItemDef.ItemId **/
	UPROPERTY(Replicated, BlueprintReadOnly, VisibleInstanceOnly, meta = (ExcludeBaseStruct, AllowPrivateAccess = "true"))
	FName ItemInstId;
	
	// 标签堆叠容器对象-基于FastArray进行网络同步
	UPROPERTY(Replicated, BlueprintReadOnly, VisibleInstanceOnly, meta=(AllowPrivateAccess = "true"))
	FYcGameplayTagStackContainer TagsStack;

public:
	const FYcInventoryItemDefinition* GetItemDef();
	FORCEINLINE const FName& GetItemInstId() const { return ItemInstId; }
	
	/** 获取特定类型的Fragment, 只能在c++使用, 同时c++都应该使用这个方式效率最高*/
	template <typename FragmentType>
	const FragmentType* GetTypedFragment()
	{
		if (!GetItemDef()) return nullptr;
		for (const auto& Fragment : ItemDef->Fragments)
		{
			const FragmentType* TypedFragment = Fragment.GetPtr<FragmentType>();
			if (TypedFragment != nullptr) return TypedFragment;
		}
		return nullptr;
	}
};