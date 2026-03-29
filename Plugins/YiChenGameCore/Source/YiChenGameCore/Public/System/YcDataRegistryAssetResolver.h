// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

struct FDataRegistryId;
struct FPrimaryAssetId;

/**
 * DataRegistry 资产解析器接口
 * 
 * 用于从 FDataRegistryId 解析出需要预加载的 PrimaryAssetId。
 * 由具体业务模块（如 Inventory）实现，AssetManager 只依赖接口。
 * 
 * 设计目的：
 * - 解耦 Gameplay 与 Inventory 模块
 * - 支持不同模块实现自己的解析逻辑
 * - 通用接口，不限于特定数据类型
 * 
 * 使用示例：
 * @code
 * // 实现示例（Inventory模块）
 * class FYcInventoryDataRegistryResolver : public IYcDataRegistryAssetResolver
 * {
 * public:
 *     virtual bool ResolveAssets(const FDataRegistryId& InDataRegistryId, TArray<FPrimaryAssetId>& OutAssetIds) const override
 *     {
 *         // 从 ItemDefinition 的 Fragment 中提取 PrimaryAssetId
 *         // ...
 *         return true;
 *     }
 * };
 * @endcode
 */
class YICHENGAMECORE_API IYcDataRegistryAssetResolver
{
public:
	/** 默认构造函数 */
	IYcDataRegistryAssetResolver();
	
	/** 虚析构函数 */
	virtual ~IYcDataRegistryAssetResolver();
	
	/**
	 * 从 DataRegistryId 解析出所有需要加载的 PrimaryAssetId
	 * 
	 * 该方法是核心解析逻辑，由具体业务模块实现。
	 * 例如 Inventory 模块会从 FItemFragment_DataAsset 中提取资产ID。
	 * 
	 * @param InDataRegistryId 数据注册表ID（如 "Weapon:AK47"）
	 * @param OutAssetIds 输出的资产ID列表（如 EquipmentVisualData、WeaponVisualData 等）
	 * @return 是否解析成功，false 表示找不到对应的数据或解析失败
	 */
	virtual bool ResolveAssets(const FDataRegistryId& InDataRegistryId, TArray<FPrimaryAssetId>& OutAssetIds) const = 0;
	
	/**
	 * 获取要加载的 Bundle 名称列表
	 * 
	 * 返回该类型数据默认需要加载的 Bundle 名称列表。
	 * 例如 Inventory 物品可以加载 "Equipped" 和 "EquipmentVisual" Bundle。
	 * 
	 * @param InDataRegistryId 数据注册表ID
	 * @param OutBundleNames 输出的 Bundle 名称列表
	 */
	virtual void GetBundleNames(const FDataRegistryId& InDataRegistryId, TArray<FName>& OutBundleNames) const = 0;
};
