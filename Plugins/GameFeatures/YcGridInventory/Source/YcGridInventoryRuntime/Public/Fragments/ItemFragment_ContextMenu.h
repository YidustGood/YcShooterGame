// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YcInventoryItemDefinition.h"

#include "ItemFragment_ContextMenu.generated.h"

class UYcInventoryContextActionPolicyAsset;
class UTexture2D;

/**
 * 物品右键菜单中的单个动作定义（数据驱动）。
 * Single context-menu action definition for an item (data-driven).
 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FGridItemContextMenuAction
{
	GENERATED_BODY()

	/**
	 * 动作唯一标识（GameplayTag），用于服务端校验与路由。
	 * Unique action identifier (GameplayTag), used for server validation and routing.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (Categories = "Item.Action"))
	FGameplayTag ActionTag;

	/** 菜单显示文本（本地化）。 / Display text in menu (localized). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FText DisplayName = FText::GetEmpty();

	/** 菜单图标（可选）。 / Optional icon shown in menu. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UTexture2D> Icon = nullptr;

	/**
	 * 排序值（升序，越小越靠前）。
	 * Sort order in ascending order (smaller value appears first).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int32 Order = 0;

	/**
	 * 执行器类型标签。
	 * Executor type tag.
	 *
	 * 推荐值 / Recommended values:
	 * - Yc.Inventory.ContextAction.Executor.Message
	 * - Yc.Inventory.ContextAction.Executor.GAS
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (Categories = "Yc.Inventory.ContextAction.Executor"))
	FGameplayTag ExecutorTag;

	/** 动作事件标签。 / Event tag passed to executor. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FGameplayTag EventTag;

	/**
	 * 动作策略资产（可选）：内部定义评估器管线。
	 * Optional policy asset that defines the evaluator pipeline.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TSoftObjectPtr<UYcInventoryContextActionPolicyAsset> Policy;

	/**
	 * 不可执行时是否仍显示（灰显）。
	 * Keep entry visible as disabled when action is not executable.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bShowWhenDisabled = true;

	/**
	 * 默认不可执行原因（可选）。
	 * Optional default disabled reason.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FText DisabledReason = FText::GetEmpty();

	/** 点击后是否自动关闭菜单。 / Whether to close menu automatically after click. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bCloseMenuOnExecute = true;

	/** 运行时：是否可执行。 / Runtime-only executable flag. */
	UPROPERTY(Transient, BlueprintReadWrite)
	bool bRuntimeCanExecute = true;

	/** 运行时：不可执行原因。 / Runtime-only disabled reason. */
	UPROPERTY(Transient, BlueprintReadWrite)
	FText RuntimeDisabledReason = FText::GetEmpty();
};

/**
 * 物品右键菜单 Fragment：挂在物品定义上，声明该物品可用动作列表。
 * Item context-menu fragment: attach to item definition to declare available actions.
 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FItemFragment_ContextMenu : public FYcInventoryItemFragment
{
	GENERATED_BODY()

	/** 动作定义数组。 / Action definition array. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TArray<FGridItemContextMenuAction> Actions;
};
