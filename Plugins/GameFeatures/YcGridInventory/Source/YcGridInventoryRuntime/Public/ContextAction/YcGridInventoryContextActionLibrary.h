// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "ContextAction/YcInventoryContextActionEvaluator.h"
#include "Fragments/ItemFragment_ContextMenu.h"

#include "YcGridInventoryContextActionLibrary.generated.h"

class AActor;
class UYcInventoryItemInstance;
class UYcInventoryManagerComponent;
class UYcInventoryContextActionPolicyAsset;

/**
 * 网格库存右键动作评估工具库。
 * Utility helpers for evaluating grid-inventory context actions.
 */
UCLASS()
class YCGRIDINVENTORYRUNTIME_API UYcGridInventoryContextActionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 校验执行器标签是否属于允许命名空间：`Yc.Inventory.ContextAction.Executor.*`。
	 * Validate whether executor tag is under allowed namespace: `Yc.Inventory.ContextAction.Executor.*`.
	 */
	UFUNCTION(BlueprintCallable, Category = "Yc|Inventory|ContextAction")
	static bool IsExecutorTagAllowed(const FGameplayTag& ExecutorTag);

	/**
	 * 评估单个动作策略（仅执行策略管线，不包含上层业务门禁）。
	 * Evaluate one action policy (policy pipeline only, without high-level gameplay gates).
	 *
	 * 常见调用约定 / Typical usage:
	 * - 客户端：用于右键菜单灰显与失败原因预览。
	 *   Client: preview disabled-state and fail reason in UI.
	 * - 服务端：执行请求前复验，作为最终权威判定。
	 *   Server: re-check before execution as the final authority.
	 */
	UFUNCTION(BlueprintCallable, Category = "Yc|Inventory|ContextAction")
	static bool EvaluateContextActionPolicyForItem(
		AActor* Player,
		UYcInventoryManagerComponent* PlayerInventory,
		UYcInventoryItemInstance* ItemInst,
		const FGridItemContextMenuAction& ActionDef,
		bool bItemOperable,
		FYcContextActionEvalResult& OutResult);

private:
	/** Internal pipeline runner used by public evaluate helpers. */
	static bool EvaluatePolicyInternal(
		const FYcContextActionEvalContext& Context,
		const UYcInventoryContextActionPolicyAsset* Policy,
		FYcContextActionEvalResult& OutResult);
};
