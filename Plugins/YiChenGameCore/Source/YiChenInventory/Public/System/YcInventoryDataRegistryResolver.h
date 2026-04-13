// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "System/YcDataRegistryAssetResolver.h"

struct FItemFragment_DataAsset;

/**
 * Inventory 模块的数据注册表资源解析器。
 *
 * 实现 IYcDataRegistryAssetResolver 接口，负责把 DataRegistryId 解析为
 * 需要预加载的 PrimaryAssetId 与 Bundle 名称。
 */
class YICHENINVENTORY_API FYcInventoryDataRegistryResolver : public IYcDataRegistryAssetResolver
{
public:
	//~ Begin IYcDataRegistryAssetResolver interface
	/**
	 * 解析 DataRegistryId 对应的资源列表（PrimaryAssetId）。
	 * @param InDataRegistryId 数据注册表ID。
	 * @param OutAssetIds 输出的资源ID列表。
	 * @return 是否解析成功。
	 */
	virtual bool ResolveAssets(const FDataRegistryId& InDataRegistryId, TArray<FPrimaryAssetId>& OutAssetIds) const override;

	/**
	 * 获取需要预加载的 Bundle 名称列表。
	 * @param InDataRegistryId 数据注册表ID。
	 * @param OutBundleNames 输出的 Bundle 名称列表。
	 */
	virtual void GetBundleNames(const FDataRegistryId& InDataRegistryId, TArray<FName>& OutBundleNames) const override;
	//~ End IYcDataRegistryAssetResolver interface

	/** 尝试从物品定义中提取 DataAsset Fragment。 */
	const FItemFragment_DataAsset* TryGetDataAssetFragment(const FDataRegistryId& InDataRegistryId) const;
};