// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "YcInventoryItemDefinition.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StructUtils/InstancedStruct.h"
#include "YcInventoryLibrary.generated.h"

struct FYcInventoryPickup;
struct FYcDataAssetEntry;
struct FGameplayTag;
struct FDataRegistryId;
class UYcInventoryItemInstance;
class UYcInventoryManagerComponent;
class UPrimaryDataAsset;

/**
 * 库存系统蓝图函数库
 * 提供库存相关的工具函数，方便蓝图和C++调用
 */
UCLASS()
class YICHENINVENTORY_API UYcInventoryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * 从物品定义中查找特定类型的Fragment
	 * @param ItemDef 物品定义
	 * @param FragmentStructType Fragment的结构体类型
	 * @return 找到的Fragment实例，未找到时返回空实例
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static FInstancedStruct FindItemFragment(const FYcInventoryItemDefinition& ItemDef, const UScriptStruct* FragmentStructType);
	
	/**
	 * 从物品定义中查找特定类型的Fragment
	 * @param ItemDefId 物品定义的Id
	 * @param FragmentStructType Fragment的结构体类型
	 * @return 找到的Fragment实例，未找到时返回空实例
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	static FInstancedStruct FindItemFragmentById(const FDataRegistryId ItemDefId, const UScriptStruct* FragmentStructType);
	
	/**
	 * 从库存归属Actor上获取库存管理组件
	 * @param InventoryOwnerActor 库存归属Actor
	 * @return 库存管理组件，未找到时返回 nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	static UYcInventoryManagerComponent* GetInventoryManagerComponent(const AActor* InventoryOwnerActor);

	/**
	 * 判断指定库存归属Actor关联的库存中是否拥有某个物品
	 * 支持直接传入 PlayerController、PlayerState、Pawn/Character
	 * @param InventoryOwnerActor 库存归属Actor
	 * @param ItemDataRegistryId 物品的数据注册表ID
	 * @return true 表示拥有该物品，false 表示未拥有或参数无效
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	static bool HasItem(const AActor* InventoryOwnerActor, const FDataRegistryId& ItemDataRegistryId);

	/**
	 * 获取指定库存归属Actor关联的库存中某个物品的总数量
	 * 返回的是所有堆叠的数量总和，而不是物品实例个数
	 * @param InventoryOwnerActor 库存归属Actor
	 * @param ItemDataRegistryId 物品的数据注册表ID
	 * @return 物品总数量，未找到或参数无效时返回0
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	static int32 GetItemCount(const AActor* InventoryOwnerActor, const FDataRegistryId& ItemDataRegistryId);

	/**
	 * 消耗指定库存归属Actor关联库存中的某个物品
	 * 支持直接传入 PlayerController、PlayerState、Pawn/Character
	 * @param InventoryOwnerActor 库存归属Actor
	 * @param ItemDataRegistryId 物品的数据注册表ID
	 * @param Count 要消耗的数量，不可小于0
	 * @return true 表示成功消耗，false 表示库存不足、参数无效或执行失败
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Inventory")
	static bool ConsumeItem(const AActor* InventoryOwnerActor, const FDataRegistryId& ItemDataRegistryId, int32 Count);
	
	/**
	 * 通过DataRegistryId获取物品定义
	 * @param ItemDataRegistryId 物品的数据注册表ID
	 * @param ItemDef 输出参数，获取到的物品定义
	 * @return true 表示成功获取，false 表示获取失败
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	static bool GetItemDefinition(const FDataRegistryId& ItemDataRegistryId, FYcInventoryItemDefinition& ItemDef);
	
	/**
	 * 通过DataRegistryId异步加载物品定义中的所有数据资产
	 * 可用于预加载物品资产，避免运行时卡顿
	 * @param WorldContextObject 世界上下文对象
	 * @param ItemDataRegistryId 物品的数据注册表ID
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory", meta = (WorldContext = "WorldContextObject"))
	static void LoadItemDefDataAssetAsync(UObject* WorldContextObject, const FDataRegistryId& ItemDataRegistryId);

	/**
	 * 按标签异步加载物品定义中的指定数据资产。
	 * 加载完成后会通过 AssetTag 对应的 GameplayMessage 广播 FYcDataAssetLifecycleMessage。
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory", meta = (WorldContext = "WorldContextObject"))
	static bool LoadItemDataAssetByTagAsync(UObject* WorldContextObject, const FDataRegistryId& ItemDataRegistryId, const FGameplayTag& AssetTag);
	
	/**
	 * 通过标签从物品定义中获取已加载的数据资产
	 * @param ItemDef 物品定义
	 * @param AssetTag 资产标签
	 * @return 已加载的数据资产指针，未找到或未加载时返回 nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	static UPrimaryDataAsset* GetYcDataAssetByTag(const FYcInventoryItemDefinition& ItemDef, const FGameplayTag& AssetTag);
	
	/**
	 * 通过标签从物品定义中获取数据资产条目
	 * @param ItemDef 物品定义
	 * @param AssetTag 资产标签
	 * @param OutDataAssetEntry 输出参数，获取到的数据资产条目
	 * @return true 表示成功获取，false 表示未找到
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	static bool GetYcDataAssetEntryByTag(const FYcInventoryItemDefinition& ItemDef, const FGameplayTag& AssetTag, FYcDataAssetEntry& OutDataAssetEntry);

	/** 从拾取库存中提取用于世界展示的主物品。优先实例，其次模板。 */
	UFUNCTION(BlueprintPure, Category = "Inventory|Pickup")
	static bool GetPrimaryPickupItemRegistryId(const FYcInventoryPickup& PickupInventory, FDataRegistryId& OutItemDataRegistryId);
	
	UFUNCTION(BlueprintPure, Category = "Interaction")
	static bool GetPickupInventory(const UObject* Object, FYcInventoryPickup& OutPickup);
};
