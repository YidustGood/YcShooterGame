// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "UIExtensionSystem.h"
#include "YiChenGameplay/Public/GameFeatures/Actions/YcGameFeatureAction_WorldActionBase.h"
#include "YcGameFeatureAction_AddWidgets.generated.h"

struct FUIExtensionHandle;
struct FComponentRequestHandle;
class UCommonActivatableWidget;

/** HUD 布局请求的数据结构体 */
USTRUCT()
struct FYcHUDLayoutRequest
{
	GENERATED_BODY()

	/** 要生成的布局 Widget */
	UPROPERTY(EditAnywhere, Category=UI, meta=(AssetBundles="Client"))
	TSoftClassPtr<UCommonActivatableWidget> LayoutClass;

	/** 要将 Widget 插入的层级 */
	UPROPERTY(EditAnywhere, Category=UI, meta=(Categories="UI.Layer"))
	FGameplayTag LayerID;
};

/** 要添加到 HUD 的单个 Widget 的数据结构体 */
USTRUCT()
struct FYcHUDElementEntry
{
	GENERATED_BODY()

	/** 要生成的 Widget */
	UPROPERTY(EditAnywhere, Category=UI, meta=(AssetBundles="Client"))
	TSoftClassPtr<UUserWidget> WidgetClass;

	/** 我们应该放置此 Widget 的插槽 ID */
	UPROPERTY(EditAnywhere, Category = UI)
	FGameplayTag SlotID;
};

/**
 * @class UYcGameFeatureAction_AddWidgets
 * @brief GameFeatureAction，负责为指定类型的 Actor 添加基于 CommonUI 的 Widgets。
 * 这个 GFA 依赖于 UIExtensionSystem 来动态地将 UI 布局和元素添加到游戏中。
 */
UCLASS(meta = (DisplayName = "Add Widgets"))
class YICHENGAMEUI_API UYcGameFeatureAction_AddWidgets : public UYcGameFeatureAction_WorldActionBase
{
	GENERATED_BODY()
public:
	//~ Begin UGameFeatureAction interface
	/**
	 * @brief 当 Game Feature 被禁用时调用。
	 * @param Context 停用过程的上下文信息。
	 */
	virtual void OnGameFeatureDeactivating(FGameFeatureDeactivatingContext& Context) override;

#if WITH_EDITORONLY_DATA
	/**
	 * @brief 在编辑器中，将此 Action 依赖的额外资源包数据添加到 AssetBundleData 中。
	 * @param AssetBundleData 用于收集资源包数据的对象。
	 */
	virtual void AddAdditionalAssetBundleData(FAssetBundleData& AssetBundleData) override;
#endif
	//~ End UGameFeatureAction interface
	
	//~ Begin UObject interface
#if WITH_EDITOR
	/**
	 * @brief 在编辑器中验证数据是否有效。
	 * @param Context 数据验证上下文。
	 * @return 返回数据验证结果。
	 */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	//~ End UObject interface
	
private:
	/** 要添加到 HUD 的布局列表 */
	UPROPERTY(EditAnywhere, Category=UI, meta=(TitleProperty="{LayerID} -> {LayoutClass}"))
	TArray<FYcHUDLayoutRequest> Layout;

	/** 要添加到 HUD 的 Widget 列表 */
	UPROPERTY(EditAnywhere, Category=UI, meta=(TitleProperty="{SlotID} -> {WidgetClass}"))
	TArray<FYcHUDElementEntry> Widgets;

	/** 每个 Actor 的数据 */
	struct FPerActorData
	{
		/** 已添加的布局 Widget */
		TArray<TWeakObjectPtr<UCommonActivatableWidget>> LayoutsAdded;
		/** UI 扩展句柄 */
		TArray<FUIExtensionHandle> ExtensionHandles;
	};

	/** 每个上下文的数据 */
	struct FPerContextData
	{
		/** 组件请求句柄 */
		TArray<TSharedPtr<FComponentRequestHandle>> ComponentRequests;
		/** 每个 Actor 的数据映射 */
		TMap<FObjectKey, FPerActorData> ActorData;
	};

	/** 上下文数据映射 */
	TMap<FGameFeatureStateChangeContext, FPerContextData> ContextData;

	//~ Begin UGameFeatureAction_WorldActionBase interface
	/**
	 * @brief 当此 Action 应用于世界时调用。
	 * @param WorldContext 世界上下文。
	 * @param ChangeContext 状态变更上下文。
	 */
	virtual void AddToWorld(const FWorldContext& WorldContext, const FGameFeatureStateChangeContext& ChangeContext) override;
	//~ End UGameFeatureAction_WorldActionBase interface

	/** 处理 Actor 扩展事件 */
	void HandleActorExtension(AActor* Actor, FName EventName, FGameFeatureStateChangeContext ChangeContext);
	/** 为指定的 Actor 添加 Widgets */
	void AddWidgets(AActor* Actor, FPerContextData& ActiveData);
	/** 从指定的 Actor 移除 Widgets */
	void RemoveWidgets(AActor* Actor, FPerContextData& ActiveData);
	/** 重置指定上下文的数据 */
	void Reset(FPerContextData& ActiveData);
};
