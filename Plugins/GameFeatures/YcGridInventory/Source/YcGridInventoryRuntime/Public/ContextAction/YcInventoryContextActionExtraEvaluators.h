// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "ContextAction/YcInventoryContextActionEvaluator.h"

#include "YcInventoryContextActionExtraEvaluators.generated.h"

/**
 * 复合评估逻辑运算符。
 * Composite logic operators for child evaluators.
 */
UENUM(BlueprintType)
enum class EYcContextCompositeLogicOp : uint8
{
	All UMETA(DisplayName = "All (AND)"),
	Any UMETA(DisplayName = "Any (OR)")
};

/**
 * 物品自身标签评估器（读取 ItemInstance::GetOwnedGameplayTags）。
 * Evaluates item's own gameplay tags (from ItemInstance::GetOwnedGameplayTags).
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class YCGRIDINVENTORYRUNTIME_API UYcEvaluator_ItemOwnedTagQuery : public UYcInventoryContextActionEvaluator
{
	GENERATED_BODY()

public:
	UYcEvaluator_ItemOwnedTagQuery(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool Evaluate_Implementation(const FYcContextActionEvalContext& Context, FText& OutFailReason) const override;

public:
	/** 标签匹配模式。 / Tag matching mode. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction|Tags")
	EYcContextTagMatchMode MatchMode = EYcContextTagMatchMode::HasTag;

	/** 单标签（用于 HasTag/NotHasTag）。 / Single tag used by HasTag/NotHasTag. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction|Tags")
	FGameplayTag RequiredTag;

	/** 标签容器（用于 HasAll/HasAny/NotHasAll/NotHasAny）。 / Tag container used by set-based modes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction|Tags")
	FGameplayTagContainer RequiredTags;

	/** 高级查询（用于 MatchTagQuery/NotMatchTagQuery）。 / Advanced query used by tag-query modes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction|Tags")
	FGameplayTagQuery RequiredTagQuery;
};

/**
 * 复合评估器：将多个子评估器按 AND/OR 组合。
 * Composite evaluator that combines child evaluators with AND/OR.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class YCGRIDINVENTORYRUNTIME_API UYcEvaluator_Composite : public UYcInventoryContextActionEvaluator
{
	GENERATED_BODY()

public:
	UYcEvaluator_Composite(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool Evaluate_Implementation(const FYcContextActionEvalContext& Context, FText& OutFailReason) const override;

public:
	/** 子评估器组合方式。 / How child evaluators are combined. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction|Composite")
	EYcContextCompositeLogicOp LogicOp = EYcContextCompositeLogicOp::All;

	/** 子评估器列表（按顺序执行）。 / Child evaluator list (evaluated in order). */
	UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category = "ContextAction|Composite")
	TArray<TObjectPtr<UYcInventoryContextActionEvaluator>> Children;

	/** 子列表为空时是否视为通过。 / If true and child list is empty, returns pass. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction|Composite")
	bool bPassWhenNoChildren = true;

	/** 是否短路求值。 / Whether to short-circuit on decisive result. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction|Composite")
	bool bStopOnFirstResult = true;

	/** 空子列表失败时的提示。 / Optional fail reason when empty list fails. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction|Composite")
	FText EmptyChildrenFailReason = FText::GetEmpty();
};
