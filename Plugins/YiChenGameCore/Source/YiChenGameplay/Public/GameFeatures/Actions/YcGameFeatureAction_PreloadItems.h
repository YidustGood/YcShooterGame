// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "DataRegistryId.h"
#include "YcGameFeatureAction_WorldActionBase.h"
#include "YcGameFeatureAction_PreloadItems.generated.h"

class UYcAssetManager;
struct FStreamableHandle;

/**
 * 预加载条目配置
 * 
 * 定义单个预加载请求的配置，包括要预加载的 DataRegistryId 和可选的 Bundle 名称。
 */
USTRUCT(BlueprintType)
struct FYcPreloadItemEntry
{
	GENERATED_BODY()
	
	/** 要预加载的数据注册表ID（如 "Weapon:AK47"） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preload")
	FDataRegistryId DataRegistryId;
	
	/** 
	 * 要加载的 Bundle 名称列表
	 * 如果为空，则使用解析器返回的默认 Bundle
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Preload")
	TArray<FName> BundleNames;
};

/**
 * GameFeatureAction：预加载物品资产
 * 
 * 该 Action 在 GameFeature 激活时预加载指定的资产，确保资产在需要时已经就绪。
 * 支持可选的阻塞模式，可以阻塞 Experience 加载直到资产预加载完成，
 * 同时不会阻塞主线程，保持 UI 响应。
 * 
 * 使用场景：
 * - 在关卡加载时预加载武器、角色等资产
 * - 通过 GameFeature 插件配置模块化的资产包
 * - 确保核心资产在游戏开始前已就绪
 * 
 * 配置示例：
 * - ItemsToPreload: ["Weapon:AK47", "Weapon:M4A1"]
 * - BundleNames: ["Equipped", "EquipmentVisual"]
 * - bBlockActivation: true（阻塞直到加载完成）
 */
UCLASS(MinimalAPI, meta = (DisplayName = "Preload Items"))
class UYcGameFeatureAction_PreloadItems : public UYcGameFeatureAction_WorldActionBase
{
	GENERATED_BODY()
	
public:
	//~ Begin UGameFeatureAction interface
	/** GameFeature 停用时调用，清理所有活跃的加载句柄和上下文数据 */
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;
	//~ End UGameFeatureAction interface

	//~ Begin UGameFeatureAction_WorldActionBase interface
	/**
	 * 将预加载功能添加到指定的 World 中
	 * 
	 * 开始预加载配置的资产。如果 bBlockActivation 为 true，
	 * 会注册到 LoadingScreenManager 保持加载屏幕显示直到加载完成。
	 */
	virtual void AddToWorld(const FWorldContext& WorldContext, const FGameFeatureStateChangeContext& ChangeContext) override;
	//~ End UGameFeatureAction_WorldActionBase
	
	//~ Begin UObject interface
#if WITH_EDITOR
	/**
	 * 编辑器中验证配置数据的有效性
	 * 
	 * 检查 ItemsToPreload 是否有效，BundleNames 是否配置正确。
	 * 
	 * @param Context 数据验证上下文
	 * @return 验证结果
	 */
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
	//~ End UObject interface
	
	/** 要预加载的条目列表 */
	UPROPERTY(EditAnywhere, Category = "Preload", meta = (TitleProperty = "DataRegistryId", ShowOnlyInnerProperties))
	TArray<FYcPreloadItemEntry> ItemsToPreload;
	
	/** 
	 * 默认的 Bundle 名称列表
	 * 如果条目中没有指定 BundleNames，则使用此默认值
	 */
	UPROPERTY(EditAnywhere, Category = "Preload")
	TArray<FName> DefaultBundleNames;
	
	/**
	 * 是否阻塞 GameFeature 激活完成
	 * 
	 * - true: Experience 加载会等待资产预加载完成，但不会阻塞主线程, 加载 UI 可以正常显示进度并响应
	 * - false: 后台异步加载，这个Action的异步资产加载不影响 Experience 加载进度
	 * 
	 * 建议对核心资产（如首选武器）设置为 true，
	 * 对可选资产设置为 false。
	 */
	UPROPERTY(EditAnywhere, Category = "Preload")
	bool bBlockActivation = false;

private:
	/** 单个 GameFeature 上下文的加载数据 */
	struct FPerContextData
	{
		/** 活跃的加载句柄列表 */
		TArray<TSharedPtr<FStreamableHandle>> ActiveHandles;
		
		/** 是否已完成 */
		bool bLoadComplete = false;
		
		/** 加载是否成功 */
		bool bLoadSuccess = false;
		
		/** 注册的 Experience 管理器（用于取消阻塞） */
		TWeakObjectPtr<class UYcExperienceManagerComponent> RegisteredExperienceManager;
	};
	
	/** 按 GameFeature 上下文存储的数据映射 */
	TMap<FGameFeatureStateChangeContext, FPerContextData> ContextData;
	
	/**
	 * 开始预加载资产
	 * 
	 * 解析 DataRegistryId 到 PrimaryAssetId，然后通过 AssetManager 异步加载。
	 * 如果 bBlockActivation = true，会注册到 ExperienceManager 阻塞 Experience 加载。
	 * 
	 * @param Context GameFeature 状态变更上下文
	 * @param WorldContext 世界上下文（用于获取 GameState 和 ExperienceManager）
	 */
	void StartPreload(FGameFeatureStateChangeContext Context, const FWorldContext& WorldContext);
	
	/**
	 * 尝试注册 Experience 阻塞器
	 * 
	 * 如果 bBlockActivation = true，会尝试从 WorldContext 获取 ExperienceManager 并注册阻塞器。
	 * 
	 * @param WorldContext 世界上下文
	 * @param Context GameFeature 状态变更上下文
	 */
	void TryRegisterActionPauser(const FWorldContext& WorldContext, FGameFeatureStateChangeContext Context);
	
	/**
	 * 收集所有需要预加载的 PrimaryAssetId
	 * 
	 * @param OutAssetIds 输出的资产ID列表
	 * @param OutBundleNames 输出的 Bundle 名称列表
	 * @return 是否成功收集
	 */
	bool CollectAssetsToPreload(TArray<FPrimaryAssetId>& OutAssetIds, TArray<FName>& OutBundleNames);
	
	/**
	 * 清理指定上下文的数据
	 * 
	 * @param ContextData 要清理的上下文数据
	 */
	void Reset(FPerContextData& ContextData);
};
