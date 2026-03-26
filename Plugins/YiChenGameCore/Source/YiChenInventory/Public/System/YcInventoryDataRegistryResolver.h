// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "System/YcDataRegistryAssetResolver.h"

struct FItemFragment_DataAsset;

/**
 * 库存物品 DataRegistry 解析器
 * 
 * 实现 IYcDataRegistryAssetResolver 接口，从 FItemFragment_DataAsset 中提取资产信息。
 * 这是针对 Inventory 模块的具体实现案例。
 * 
 * 工作流程：
 * 1. 通过 FDataRegistryId 获取 FYcInventoryItemDefinition
 * 2. 从 ItemDefinition 中获取 FItemFragment_DataAsset
 * 3. 遍历 DataAssetMap 提取所有 PrimaryAssetId
 * 4. 返回默认的 Bundle 名称
 * 
 * 使用示例：
 * @code
 * // 注册解析器
 * UYcAssetManager& AssetManager = UYcAssetManager::Get();
 * AssetManager.RegisterDataRegistryResolver(MakeShared<FYcInventoryDataRegistryResolver>());
 * 
 * // 解析资产
 * TArray<FPrimaryAssetId> AssetIds;
 * Resolver->ResolveAssets(FDataRegistryId("Weapon", "AK47"), AssetIds);
 * @endcode
 */
class FYcInventoryDataRegistryResolver : public IYcDataRegistryAssetResolver
{
public:
	//~ Begin IYcDataRegistryAssetResolver interface
	/**
	 * 从 DataRegistryId 解析出所有需要加载的 PrimaryAssetId
	 * 
	 * 实现逻辑：
	 * 1. 通过 UYcDataRegistrySubsystem 获取 ItemDefinition
	 * 2. 查找 FItemFragment_DataAsset Fragment
	 * 3. 遍历 DataAssetMap 提取所有 DataAssetId
	 * 
	 * @param InDataRegistryId 数据注册表ID（如 "Weapon:AK47"）
	 * @param OutAssetIds 输出的资产ID列表
	 * @return 是否解析成功
	 */
	virtual bool ResolveAssets(const FDataRegistryId& InDataRegistryId, TArray<FPrimaryAssetId>& OutAssetIds) const override;
	
	
	/**
	 * 获取要加载的 Bundle 名称列表
	 * 
	 * 返回该类型数据默认需要加载的 Bundle 名称列表, 从FItemFragment_DataAsset中收集
	 * 
	 * @param InDataRegistryId 数据注册表ID
	 * @param OutBundleNames 输出的 Bundle 名称列表
	 */
	virtual void GetBundleNames(const FDataRegistryId& InDataRegistryId, TArray<FName>& OutBundleNames) const override;
	//~ End IYcDataRegistryAssetResolver interface
	
	const FItemFragment_DataAsset* TryGetDataAssetFragment(const FDataRegistryId& InDataRegistryId) const;
};
