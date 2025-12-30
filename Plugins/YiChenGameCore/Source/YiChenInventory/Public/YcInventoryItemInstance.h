// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "DataRegistryId.h"
#include "YcInventoryItemDefinition.h"
#include "YiChenGameCore/Public/YcReplicableObject.h"
#include "YcGameplayTagStack.h"
#include "YcInventoryItemInstance.generated.h"

class UYcInventoryManagerComponent;

/**
 * 根据ItemDefinition创建的Item的实例对象
 * 
 * 支持网络复制，由InventoryManagerComponent持有（InventoryItemList，使用FastArray进行网络同步）
 * 基于FastArray提供了高效的网络同步TagsStack，可以自由的以GameplayTag为单位存储StackCount
 * 
 * 物品定义通过 FDataRegistryId 引用，支持从多个数据源（DataTable）聚合查询
 */
UCLASS(BlueprintType)
class YICHENINVENTORY_API UYcInventoryItemInstance : public UYcReplicableObject
{
	GENERATED_BODY()
	
public:
	UYcInventoryItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	//~ Begin UObject Interface
	virtual bool IsSupportedForNetworking() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UObject Interface
	
	//~=============================================================================
	// 物品定义查询
	
	/**
	 * 获取物品定义
	 * 优先返回缓存的指针，如果缓存无效则从DataRegistry重新获取
	 * @return 物品定义指针，如果不存在则返回nullptr
	 */
	const FYcInventoryItemDefinition* GetItemDef();
	
	/**
	 * 蓝图版本：获取物品定义
	 * @param OutItemDef 输出的物品定义副本
	 * @return 是否成功获取
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory", DisplayName = "GetItemDef")
	bool K2_GetItemDef(FYcInventoryItemDefinition& OutItemDef);
	
	/**
	 * 获取物品的DataRegistry ID
	 * @return 物品在DataRegistry中的唯一标识
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	const FDataRegistryId& GetItemRegistryId() const { return ItemRegistryId; }
	
	/**
	 * 获取物品实例的唯一ID
	 * 同一物品定义可能存在多个实例，每个实例有唯一的ItemInstId
	 * @return 物品实例ID
	 */
	FORCEINLINE const FName& GetItemInstId() const { return ItemInstId; }
	
	//~=============================================================================
	// Fragment查询
	
	/**
	 * 在Instance所属的ItemDef上查找指定类型的Fragment
	 * @param FragmentStructType 目标Fragment结构类型
	 * @return 查找到的Fragment结构体数据，如果不存在则返回空结构体
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Inventory")
	TInstancedStruct<FYcInventoryItemFragment> FindItemFragment(const UScriptStruct* FragmentStructType);
	
	/**
	 * C++版本：获取特定类型的Fragment
	 * 效率最高，推荐在C++中使用
	 * @tparam FragmentType Fragment类型
	 * @return Fragment指针，如果不存在则返回nullptr
	 */
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
	
	//~=============================================================================
	// 标签堆叠系统
	
	/**
	 * 向指定Tag添加堆叠数量
	 * @param Tag 目标GameplayTag
	 * @param StackCount 要添加的数量（如果<0，将不执行任何操作）
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	/**
	 * 从指定Tag移除堆叠数量
	 * @param Tag 目标GameplayTag
	 * @param StackCount 要移除的数量（如果<0，将不执行任何操作）
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	/**
	 * 获得指定Tag的堆叠数量
	 * @param Tag 目标GameplayTag
	 * @return 堆叠数量（Tag不存在返回0）
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	/**
	 * 检查是否拥有指定Tag
	 * @param Tag 目标GameplayTag
	 * @return 如果Tag的StackCount>0则返回true
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool HasStatTag(FGameplayTag Tag) const;
	
	//~=============================================================================
	// 其他
	
	/**
	 * 获取该物品实例所属的InventoryManagerComponent
	 * @return 库存管理组件，如果不存在则返回nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	UYcInventoryManagerComponent* GetInventoryManager() const;

protected:
	/**
	 * 当ItemRegistryId值到达客户端后的回调
	 * 会清除缓存并尝试重新获取物品定义
	 */
	UFUNCTION()
	void OnRep_ItemRegistryId();
	
private:
	friend struct FYcInventoryItemFragment;
	friend struct FYcInventoryItemList;
	
	/**
	 * 设置物品的DataRegistry ID
	 * 通常在通过ItemDef创建出Instance后立刻调用
	 * @param InItemRegistryId 物品在DataRegistry中的ID
	 */
	void SetItemRegistryId(const FDataRegistryId& InItemRegistryId);
	
	/**
	 * 设置物品实例的唯一ID
	 * @param NewId 新的实例ID
	 */
	void SetItemInstId(const FName NewId);
	
	/**
	 * 从DataRegistry获取并缓存物品定义
	 * @return 是否成功获取
	 */
	bool CacheItemDefFromRegistry();

	//~=============================================================================
	// 内部数据
	
	/**
	 * 物品在DataRegistry中的唯一标识
	 * 用于从DataRegistry获取物品定义
	 */
	UPROPERTY(ReplicatedUsing = OnRep_ItemRegistryId, BlueprintReadOnly, VisibleInstanceOnly, meta = (AllowPrivateAccess = "true"))
	FDataRegistryId ItemRegistryId;
	
	/**
	 * 物品定义指针缓存
	 * 在SetItemRegistryId或OnRep_ItemRegistryId时从DataRegistry获取并缓存
	 * 指向DataRegistry内部数据，生命周期由Registry管理
	 */
	const FYcInventoryItemDefinition* ItemDef = nullptr;
	
	/**
	 * 物品实例的唯一ID
	 * 同一物品定义可能存在多个实例，只有第一份实例能直接使用ItemDef.ItemId
	 * 后续实例会自动生成带后缀的唯一ID
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, VisibleInstanceOnly, meta = (AllowPrivateAccess = "true"))
	FName ItemInstId;
	
	/**
	 * 标签堆叠容器
	 * 基于FastArray进行网络同步，可以自由的以GameplayTag为单位存储数值
	 */
	UPROPERTY(Replicated, BlueprintReadOnly, VisibleInstanceOnly, meta = (AllowPrivateAccess = "true"))
	FYcGameplayTagStackContainer TagsStack;
};
