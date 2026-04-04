// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"

#include "YcInventoryContextActionPolicyAsset.generated.h"

class UYcInventoryContextActionEvaluator;

/**
 * 右键动作策略资产：用于配置评估器管线。
 * Context-action policy asset: used to configure evaluator pipeline.
 *
 * 配置说明 / Configuration notes:
 * - 一个动作通常引用一个 Policy 资产。
 *   One action usually references one policy asset.
 * - Evaluators 数组按顺序执行。
 *   Evaluators are executed in array order.
 */
UCLASS(BlueprintType)
class YCGRIDINVENTORYRUNTIME_API UYcInventoryContextActionPolicyAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	/**
	 * 是否在首个失败时短路。
	 * Whether to stop pipeline at the first failed evaluator.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction")
	bool bStopOnFirstFailure = true;

	/**
	 * 是否允许客户端做预评估（仅用于UI展示，最终以服务端为准）。
	 * Whether client-side preview evaluation is allowed (UI-only, server remains authoritative).
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction")
	bool bAllowClientPreview = true;

	/**
	 * 评估器管线（按数组顺序执行）。
	 * Evaluator pipeline (executed in array order).
	 */
	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = "ContextAction")
	TArray<TObjectPtr<UYcInventoryContextActionEvaluator>> Evaluators;
};
