// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "DataRegistrySubsystem.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "YcDataRegistrySubsystem.generated.h"

class UDataRegistry;
struct FDataRegistryType;
struct FDataRegistryId;

//~=============================================================================
// 辅助结构体
//~=============================================================================

/**
 * DataRegistry 查询结果包装结构体
 * 用于蓝图中获取查询结果和状态
 */
USTRUCT(BlueprintType)
struct FDataRegistryFindResult
{
	GENERATED_BODY()
	
public:
	/** 查询结果状态 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	EDataRegistrySubsystemGetItemResult OutResult = EDataRegistrySubsystemGetItemResult::NotFound;
	
	/** 查询到的数据（基类，蓝图中需要转换为具体类型） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	FTableRowBase OutItem;
};

/**
 * FPrimaryAssetId 的包装结构体
 * 便于在 DataTable 中配置主资产引用，然后由 DataRegistry 统一管理
 */
USTRUCT(BlueprintType, Blueprintable)
struct FYcPrimaryDataAssetIdWrapper : public FTableRowBase
{
	GENERATED_BODY()
	
public:
	/** 主资产 ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FPrimaryAssetId AssetId;
};

//~=============================================================================
// UYcDataRegistrySubsystem
//~=============================================================================

/**
 * DataRegistry 工具子系统
 * 
 * 提供对 UE DataRegistry 系统的便捷访问封装，包括：
 * - 类型安全的缓存数据获取（模板方法）
 * - 蓝图友好的查询接口
 * - 动态注册数据源
 * - 异步数据加载
 * 
 * 使用示例（C++）：
 * @code
 * // 获取缓存的物品定义
 * const FYcInventoryItemDefinition* ItemDef = 
 *     UYcDataRegistrySubsystem::GetCachedItem<FYcInventoryItemDefinition>(ItemRegistryId);
 * 
 * // 检查数据是否在缓存中
 * if (UYcDataRegistrySubsystem::IsItemCached(ItemRegistryId))
 * {
 *     // 安全使用
 * }
 * @endcode
 */
UCLASS()
class YICHENGAMEPLAY_API UYcDataRegistrySubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:
	//~=============================================================================
	// USubsystem Interface
	//~=============================================================================
	
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	//~=============================================================================
	// 核心查询接口（C++ 模板方法）
	//~=============================================================================
	
	/**
	 * 获取缓存中的数据项（类型安全）
	 * 
	 * @tparam T 目标结构体类型
	 * @param ItemId DataRegistry 中的数据 ID
	 * @return 数据指针，如果不在缓存中或类型不匹配则返回 nullptr
	 * 
	 * @note 返回的指针指向 Registry 内部数据，生命周期由 Registry 管理
	 * @note 如果数据不在缓存中，需要先调用 AcquireItem 异步加载
	 */
	template <typename T>
	static const T* GetCachedItem(const FDataRegistryId& ItemId);
	
	/**
	 * 检查数据项是否在缓存中
	 * 
	 * @param ItemId DataRegistry 中的数据 ID
	 * @return 如果数据在缓存中返回 true
	 */
	static bool IsItemCached(const FDataRegistryId& ItemId);
	
	//~=============================================================================
	// 蓝图查询接口
	//~=============================================================================
	
	/**
	 * 获取指定 Registry 类型的所有数据 ID
	 * 
	 * @param RegistryType Registry 类型
	 * @param OutIds 输出的 ID 数组
	 * @return 是否成功获取
	 */
	UFUNCTION(BlueprintCallable, Category = "DataRegistry")
	static bool GetAllRegistryIds(FDataRegistryType RegistryType, TArray<FDataRegistryId>& OutIds);
	
	/**
	 * 蓝图版本：查找缓存中的数据项
	 * 
	 * @param ItemId 数据 ID
	 * @param OutResult 查询结果
	 */
	UFUNCTION(BlueprintCallable, Category = "DataRegistry")
	static void FindCachedItem(FDataRegistryId ItemId, FDataRegistryFindResult& OutResult);
	
	/**
	 * 蓝图版本：检查数据项是否在缓存中
	 * 
	 * @param ItemId 数据 ID
	 * @return 如果数据在缓存中返回 true
	 */
	UFUNCTION(BlueprintPure, Category = "DataRegistry", DisplayName = "IsItemCached")
	static bool IsItemCachedBP(FDataRegistryId ItemId);
	
	/**
	 * 从 DataRegistry 获取主资产 ID
	 * 用于配置在 DataTable 中的资产引用
	 * 
	 * @param ItemId 数据 ID
	 * @param OutDataAssetId 输出的主资产 ID
	 * @return 是否成功获取
	 */
	UFUNCTION(BlueprintCallable, Category = "DataRegistry")
	static bool FindCachedPrimaryDataAssetId(FDataRegistryId ItemId, FYcPrimaryDataAssetIdWrapper& OutDataAssetId);
	
	//~=============================================================================
	// 异步加载接口
	//~=============================================================================
	
	/**
	 * 异步请求加载数据项
	 * 
	 * @param ItemId 数据 ID
	 * @param AcquireCallback 加载完成回调
	 * @return 是否成功发起请求
	 * 
	 * @note 如果数据已在缓存中，回调会立即触发
	 */
	UFUNCTION(BlueprintCallable, Category = "DataRegistry")
	static bool AcquireItem(FDataRegistryId ItemId, FDataRegistryItemAcquiredBPCallback AcquireCallback);
	
	/**
	 * 批量异步请求加载数据项
	 * 
	 * @param ItemIds 数据 ID 数组
	 * @param AcquireCallback 每个数据加载完成时的回调
	 * @return 成功发起请求的数量
	 */
	UFUNCTION(BlueprintCallable, Category = "DataRegistry")
	static int32 AcquireItems(const TArray<FDataRegistryId>& ItemIds, FDataRegistryItemAcquiredBPCallback AcquireCallback);
	
	//~=============================================================================
	// 动态注册接口
	//~=============================================================================
	
	/**
	 * 将 DataTable 动态注册到指定的 Registry
	 * 
	 * @param RegistryType Registry 类型
	 * @param DataTable 要注册的 DataTable 软引用
	 * 
	 * @note 适用于运行时动态添加数据源（如 DLC、Mod）
	 */
	UFUNCTION(BlueprintCallable, Category = "DataRegistry")
	static void RegisterDataTableToRegistry(FDataRegistryType RegistryType, TSoftObjectPtr<UDataTable> DataTable);
	
	/**
	 * 加载指定的 DataRegistry 资产
	 * 
	 * @param RegistryPath Registry 资产路径
	 * @return 是否成功加载
	 */
	static bool LoadDataRegistry(const TSoftObjectPtr<UDataRegistry>& RegistryPath);
	
	//~=============================================================================
	// 工具方法
	//~=============================================================================
	
	/**
	 * 获取指定类型的 DataRegistry 对象
	 * 
	 * @param RegistryType Registry 类型
	 * @return Registry 对象指针，如果不存在返回 nullptr
	 */
	static const UDataRegistry* GetRegistry(FDataRegistryType RegistryType);
	
	/**
	 * 获取引擎的 DataRegistrySubsystem
	 * 
	 * @return DataRegistrySubsystem 指针，如果不可用返回 nullptr
	 */
	static UDataRegistrySubsystem* GetEngineSubsystem();
};

//~=============================================================================
// 模板方法实现
//~=============================================================================

template <typename T>
const T* UYcDataRegistrySubsystem::GetCachedItem(const FDataRegistryId& ItemId)
{
	const UDataRegistrySubsystem* Subsystem = GetEngineSubsystem();
	if (!Subsystem)
	{
		return nullptr;
	}
	return Subsystem->GetCachedItem<T>(ItemId);
}