// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcAssetManagerStartupJob.h"
#include "Engine/AssetManager.h"
#include "YcAssetManager.generated.h"

class UYcGameData;
class UYcPawnData;

struct FYcBundles
{
	static const FName Equipped;
};

/**
 * 插件提供的游戏资产管理器, 项目需要以此类作为资产管理类的基类，你可以继承这个类做更多的定制化逻辑
 * 
 * 自定义的资产管理器实现，提供游戏特定的资产加载逻辑和管理功能。
 * 包括同步/异步加载、资产追踪、启动工作管理等功能。
 * 
 * 使用前的配置说明：
 * 1. 在DefaultGame.ini中配置默认资产路径：
 *    [/Script/YiChenGameplay.YcAssetManager]
 *    YcGameDataPath=/Game/DefaultGameData.DefaultGameData
 *    DefaultPawnData=/Game/Characters/Heroes/DefaultPawnData.DefaultPawnData
 * 
 * 2. 在DefaultEngine.ini中指定使用此资产管理器：
 *    [/Script/Engine.Engine]
 *    AssetManagerClassName=/Script/YiChenGameplay.YcAssetManager
 * 
 * 3. 命令行参数 -LogAssetLoads 可启用资产加载日志输出
 */
UCLASS(Config = Game)
class YICHENGAMEPLAY_API UYcAssetManager : public UAssetManager
{
	GENERATED_BODY()
public:
	/** 构造函数 */
	UYcAssetManager();
	
	/**
	 * 获取资产管理器单例
	 * @return 资产管理器引用
	 */
	static UYcAssetManager& Get();
	
	/**
	 * 获取由TSoftObjectPtr指定的资产, 如果未加载则会进行同步加载
	 * @param AssetPointer 软资产指针
	 * @param bKeepInMemory 是否保持资产在内存中(如果为true, YcAssetManager会保存该资产到LoadedAssets列表中)
	 * @return 加载的资产指针，加载失败返回nullptr
	 */
	template <typename AssetType>
	static AssetType* GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);
	
	/**
	 * 获取由TSoftClassPtr指定的资产类, 如果未加载则会进行同步加载
	 * @param AssetPointer 软类指针
	 * @param bKeepInMemory 是否保持资产在内存中(如果为true, YcAssetManager会保存该资产到LoadedAssets列表中)
	 * @return 加载的资产类，加载失败返回nullptr
	 */
	template <typename AssetType>
	static TSubclassOf<AssetType> GetSubclass(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory = true);
	
	/**
	 * 输出所有已加载的资产
	 * 在日志中显示当前由当前资产管理器追踪的所有已加载资产
	 */
	static void DumpLoadedAssets();
	
	/**
	 * 获取全局游戏数据
	 * @return 全局游戏数据引用
	 */
	const UYcGameData& GetGameData();
	
	/**
	 * 获取默认Pawn配置
	 * @return 默认Pawn配置指针，未配置返回nullptr
	 */
	const UYcPawnData* GetDefaultPawnData() const;
	
protected:
	/**
	 * 获取或加载指定类型的主数据资产
	 * 如果已加载则直接返回，否则进行同步加载
	 * @param DataPath 主数据资产的软指针
	 * @return 加载的主数据资产引用
	 */
	template <typename PrimaryDataAssetClass>
	const PrimaryDataAssetClass& GetOrLoadTypedGamePrimaryDataAsset(const TSoftObjectPtr<PrimaryDataAssetClass>& DataPath)
	{
		// 如已加载则直接返回
		if (TObjectPtr<UPrimaryDataAsset> const* pResult = GamePrimaryDataAssetMap.Find(PrimaryDataAssetClass::StaticClass()))
		{
			return *CastChecked<PrimaryDataAssetClass>(*pResult);
		}

		// 进行同步加载
		return *CastChecked<const PrimaryDataAssetClass>(LoadGamePrimaryDataAssetOfClass(PrimaryDataAssetClass::StaticClass(), DataPath, PrimaryDataAssetClass::StaticClass()->GetFName()));
	}
	
	/** 同步加载资产 */
	static UObject* SynchronousLoadAsset(const FSoftObjectPath& AssetPath);
	
	/** 判断是否应该输出资产加载日志 */
	static bool ShouldLogAssetLoads();

	/** 线程安全地添加已加载的资产到内存追踪列表 */
	void AddLoadedAsset(const UObject* Asset);

	//~UAssetManager interface
	/** 启动初始加载过程 */
	virtual void StartInitialLoading() override;
#if WITH_EDITOR
	/** PIE前预加载资产 */
	virtual void PreBeginPIE(bool bStartSimulate) override;
#endif
	//~End of UAssetManager interface
	
	/**
	 * 加载指定类型的主数据资产
	 * 这是一个同步加载过程，会阻塞线程直到加载完成
	 * @param DataClass 主数据资产类
	 * @param DataClassPath 主数据资产路径
	 * @param PrimaryAssetType 主资产类型
	 * @return 加载的主数据资产指针
	 */
	UPrimaryDataAsset* LoadGamePrimaryDataAssetOfClass(TSubclassOf<UPrimaryDataAsset> DataClass, const TSoftObjectPtr<UPrimaryDataAsset>& DataClassPath, FPrimaryAssetType PrimaryAssetType);
	
	/**
	 * 全局游戏数据资产路径
	 * 在DefaultGame.ini中配置
	 */
	UPROPERTY(Config)
	TSoftObjectPtr<UYcGameData> YcGameDataPath;
	
	/**
	 * 已加载的主数据资产映射表
	 * 缓存已加载的主数据资产，避免重复加载
	 */
	UPROPERTY(Transient)
	TMap<TObjectPtr<UClass>, TObjectPtr<UPrimaryDataAsset>> GamePrimaryDataAssetMap;
	
	/**
	 * 默认Pawn配置
	 * 在生成玩家Pawn时，如果玩家状态中未设置则使用此默认配置
	 */
	UPROPERTY(Config)
	TSoftObjectPtr<UYcPawnData> DefaultPawnData;
	
private:
	/** 执行所有启动工作任务 */
	void DoAllStartupJobs();
	
	/** 初始化GameplayCue管理器 */
	void InitializeGameplayCueManager();
	
	/** 更新初始游戏内容加载进度 */
	void UpdateInitialGameContentLoadPercent(float GameContentPercent);
	
	/** 启动时要执行的工作任务列表 */
	TArray<FYcAssetManagerStartupJob> StartupJobs;
	
	/** 由资产管理器追踪的已加载资产集合 */
	UPROPERTY()
	TSet<TObjectPtr<const UObject>> LoadedAssets;

	/** 修改已加载资产列表时的临界区锁 */
	FCriticalSection LoadedAssetsCritical;
};

template <typename AssetType>
AssetType* UYcAssetManager::GetAsset(const TSoftObjectPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	AssetType* LoadedAsset = nullptr;

	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedAsset = AssetPointer.Get();
		if (!LoadedAsset)
		{
			LoadedAsset = Cast<AssetType>(SynchronousLoadAsset(AssetPath));
			ensureAlwaysMsgf(LoadedAsset, TEXT("Failed to load asset [%s]"), *AssetPointer.ToString());
		}

		if (LoadedAsset && bKeepInMemory)
		{
			// Added to loaded asset list.
			Get().AddLoadedAsset(Cast<UObject>(LoadedAsset));
		}
	}

	return LoadedAsset;
}

template <typename AssetType>
TSubclassOf<AssetType> UYcAssetManager::GetSubclass(const TSoftClassPtr<AssetType>& AssetPointer, bool bKeepInMemory)
{
	TSubclassOf<AssetType> LoadedSubclass;

	const FSoftObjectPath& AssetPath = AssetPointer.ToSoftObjectPath();

	if (AssetPath.IsValid())
	{
		LoadedSubclass = AssetPointer.Get();
		if (!LoadedSubclass)
		{
			LoadedSubclass = Cast<UClass>(SynchronousLoadAsset(AssetPath));
			ensureAlwaysMsgf(LoadedSubclass, TEXT("Failed to load asset class [%s]"), *AssetPointer.ToString());
		}

		if (LoadedSubclass && bKeepInMemory)
		{
			// Added to loaded asset list.
			Get().AddLoadedAsset(Cast<UObject>(LoadedSubclass));
		}
	}

	return LoadedSubclass;
}
