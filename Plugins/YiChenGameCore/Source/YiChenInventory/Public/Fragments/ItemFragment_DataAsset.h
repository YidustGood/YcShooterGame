// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "YcInventoryItemDefinition.h"
#include "Engine/StreamableManager.h"
#include "ItemFragment_DataAsset.generated.h"

/**
 * 数据资产生命周期消息
 * 用于广播数据资产的加载/卸载事件
 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcDataAssetLifecycleMessage
{
	GENERATED_BODY()
	
	/** 已加载的数据资产指针 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="DataAsset")
	TObjectPtr<UPrimaryDataAsset> LoadedDataAsset = nullptr;
	
	/** 相关对象（通常是触发加载的物品实例） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="DataAsset")
	UObject* RelatedObject = nullptr;
	
	/** 资产标签，用于标识资产类型 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="DataAsset")
	FGameplayTag AssetTag;
	
	/** true 表示加载，false 表示卸载 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="DataAsset")
	bool bIsLoaded = true;
};

/**
 * 数据资产条目
 * 封装了数据资产的加载、卸载和查询功能
 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcDataAssetEntry
{
	GENERATED_BODY()
	
	/** 数据资产的主资产ID */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DataAsset")
	FPrimaryAssetId DataAssetId;
	
	/** 
	 * 要加载的资产包名称列表, 一定要和资产项的meta = (AssetBundles = "XXX") 匹配上, 配置错了不会自动加载相关资产, 详情了解AssetBundles机制
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "DataAsset")
	TArray<FName> AssetBundleNames;
	
	/**
	 * 异步加载资产数据
	 * @param OnLoadCompleted 加载完成回调
	 * @param bExecuteIfAlreadyLoaded 如果资产已加载，是否立即执行回调（默认为 true）
	 * @return 加载句柄，可用于监控进度或取消加载；如果已加载则返回 nullptr
	 */
	TSharedPtr<struct FStreamableHandle> LoadDataAssetAsync(const FStreamableDelegate& OnLoadCompleted = FStreamableDelegate(), bool bExecuteIfAlreadyLoaded = true) const;
	
	/**
	 * 卸载资产数据
	 * @param InRelatedObject 相关对象，用于广播卸载消息
	 * @param AssetTag 资产标签，用于消息广播
	 */
	void UnloadDataAsset(UObject* InRelatedObject = nullptr, FGameplayTag AssetTag = FGameplayTag()) const;
	
	/**
	 * 获取已加载的数据资产（基类指针）
	 * @return 已加载的数据指针，未加载时返回 nullptr
	 */
	UPrimaryDataAsset* GetLoadedDataAsset() const;

	/**
	 * 获取已加载的数据资产（模板版本，自动转换类型）
	 * @return 已加载的数据指针（指定类型），未加载或类型不匹配时返回 nullptr
	 */
	template<typename T>
	T* GetLoadedDataAssetAs() const
	{
		return Cast<T>(GetLoadedDataAsset());
	}
	
	/**
	 * 检查数据是否已加载
	 * @return true 表示已加载，false 表示未加载
	 */
	bool IsDataAssetLoaded() const;

	/**
	 * 获取调试字符串
	 * @return 包含资产ID和加载状态的调试信息
	 */
	FString GetDebugString() const
	{
		return FString::Printf(TEXT("DataAsset(Id=%s, Loaded=%s)"), 
			*DataAssetId.ToString(), IsDataAssetLoaded() ? TEXT("Yes") : TEXT("No"));
	}
};


/**
 * 物品数据资产Fragment
 * 用于管理物品关联的数据资产（如视觉资源、配置数据等）
 * 支持自动加载和按需加载
 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FItemFragment_DataAsset : public FYcInventoryItemFragment
{
	GENERATED_BODY()
	
	/** 数据资产映射表，使用GameplayTag作为键来标识不同类型的资产 */
	UPROPERTY(EditDefaultsOnly, Category="DataAsset", meta=(Categories = "Asset", ForceInlineRow))
	TMap<FGameplayTag, FYcDataAssetEntry> DataAssetMap;
	
	/**
	 * 物品实例创建时的回调
	 * 会自动触发标记为自动加载的数据资产的加载
	 */
	virtual void OnInstanceCreated(UYcInventoryItemInstance* Instance) const override;
	
	/**
	 * 异步加载所有标记为自动加载的数据资产
	 * @param InRelatedObject 相关对象，用于消息广播
	 */
	virtual void LoadAllDataAssetAsync(UObject* InRelatedObject) const;

	/**
	 * 通过标签获取数据资产条目
	 * @param Tag 资产标签
	 * @return 资产条目指针，未找到时返回 nullptr
	 */
	const FYcDataAssetEntry* GetDataAssetByTag(FGameplayTag Tag) const;
};
