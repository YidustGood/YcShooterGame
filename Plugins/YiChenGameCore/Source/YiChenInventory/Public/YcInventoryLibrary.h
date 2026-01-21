// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "YcInventoryItemDefinition.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StructUtils/InstancedStruct.h"
#include "YcInventoryLibrary.generated.h"

struct FYcDataAssetEntry;
struct FGameplayTag;
struct FDataRegistryId;
class UYcInventoryItemInstance;
class UYcInventoryManagerComponent;

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
	static TInstancedStruct<FYcInventoryItemFragment> FindItemFragment(const FYcInventoryItemDefinition& ItemDef, const UScriptStruct* FragmentStructType);
	
	/**
	 * 从Actor上获取库存管理组件
	 * @param Actor 目标Actor
	 * @return 库存管理组件，未找到时返回 nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "Inventory")
	static UYcInventoryManagerComponent* GetInventoryManagerComponent(const AActor* Actor);
	
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
};
