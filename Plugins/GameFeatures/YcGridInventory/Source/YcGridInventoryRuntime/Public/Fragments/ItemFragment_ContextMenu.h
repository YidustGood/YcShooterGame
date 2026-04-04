// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YcInventoryItemDefinition.h"

#include "ItemFragment_ContextMenu.generated.h"

class UTexture2D;

/**
 * 物品右键菜单中的单个动作定义（数据驱动）。
 * Single context-menu action definition for an item (data-driven).
 *
 * 配置说明 / Configuration Notes:
 * - 每个动作建议使用唯一的 ActionTag，避免同一物品出现重复动作。
 *   Use a unique ActionTag per action to avoid duplicated entries on the same item.
 * - 执行流程由 ExecutorTag + EventTag 决定：
 *   Execution route is determined by ExecutorTag + EventTag:
 *   1) Message 执行器：广播消息给玩法层处理。
 *      Message executor: broadcast a gameplay message for gameplay-layer handling.
 *   2) GAS 执行器：向玩家ASC发送GameplayEvent，由能力系统处理。
 *      GAS executor: send GameplayEvent to player's ASC for ability handling.
 * - UI展示名称与图标都来自这里，逻辑本身不在该结构体实现。
 *   UI label/icon come from this struct; gameplay logic is implemented elsewhere.
 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FGridItemContextMenuAction
{
	GENERATED_BODY()

	/**
	 * 动作唯一标识（GameplayTag），用于服务端校验与路由。
	 * Unique action identifier (GameplayTag), used for server validation and routing.
	 *
	 * 示例 / Example: Item.Action.Use, Item.Action.Inspect
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (Categories = "Item.Action"))
	FGameplayTag ActionTag;

	/**
	 * 菜单显示文本（支持本地化）。
	 * Display text in the context menu (localized text supported).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FText DisplayName = FText::GetEmpty();

	/**
	 * 菜单图标（可选）。
	 * Optional icon shown in the menu entry.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TObjectPtr<UTexture2D> Icon = nullptr;

	/**
	 * 菜单排序值（升序，越小越靠前）。
	 * Sort order in ascending order (smaller value appears first).
	 *
	 * 示例 / Example:
	 * - Use = 0
	 * - Inspect = 10
	 * - Drop = 100
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int32 Order = 0;

	/**
	 * 执行器类型标签，用于选择动作请求的执行通道。
	 * Executor type tag that selects the action execution channel.
	 *
	 * 常见值 / Common values (recommended):
	 * - 走GameplayMessage消息广播: Yc.Inventory.ContextAction.Executor.Message
	 * - 走GAS的GameplayEvent事件触发: Yc.Inventory.ContextAction.Executor.GAS
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, meta = (Categories = "Yc.Inventory.ContextAction.Executor"))
	FGameplayTag ExecutorTag;

	/**
	 * 动作事件标签，传给执行器作为玩法层分发键。
	 * Action event tag passed into executor as gameplay dispatch key.
	 *
	 * 示例 / Example:
	 * - Item.Action.Event.Heal
	 * - Item.Action.Event.Inspect
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FGameplayTag EventTag;
	
	/**
	 * 点击后是否自动关闭菜单（默认 true）。
	 * Whether to close menu automatically after click (default true).
	 *
	 * 建议 / Recommendation:
	 * - 一次性动作（使用/检视）通常设为 true。
	 *   One-shot actions (use/inspect) usually set to true.
	 * - 需要连续点击的场景可设为 false。
	 *   Set to false if repeated actions are needed without reopening menu.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bCloseMenuOnExecute = true;
};

/**
 * 物品右键菜单 Fragment：挂在物品定义上，声明该物品可用的右键动作列表。
 * Item context-menu fragment: attached to item definition to declare available actions.
 *
 * 配置方式 / How to configure:
 * 1) 在物品定义 Fragments 中添加本结构体。
 *    Add this fragment to the item's Fragments list.
 * 2) 在 Actions 数组中填入一个或多个 FGridItemContextMenuAction。
 *    Fill one or more FGridItemContextMenuAction entries in Actions.
 * 3) 若 Actions 为空，右键菜单不会弹出。
 *    If Actions is empty, no context menu will be opened.
 *
 * 典型案例 / Typical example:
 * - 医疗物品：Use -> Message Executor + Item.Action.Event.Heal
 * - 收藏品：Inspect -> Message/GAS Executor + Item.Action.Event.Inspect
 *
 * - Medical item: Use -> Message Executor + Item.Action.Event.Heal
 * - Collectible item: Inspect -> Message/GAS Executor + Item.Action.Event.Inspect
 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FItemFragment_ContextMenu : public FYcInventoryItemFragment
{
	GENERATED_BODY()

	/**
	 * 动作定义数组；每个元素对应菜单中的一项。
	 * Action definition array; each element corresponds to one menu entry.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TArray<FGridItemContextMenuAction> Actions;
};
