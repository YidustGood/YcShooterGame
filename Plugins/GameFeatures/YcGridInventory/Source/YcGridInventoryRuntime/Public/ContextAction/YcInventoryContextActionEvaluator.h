// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"
#include "UObject/Object.h"

#include "YcInventoryContextActionEvaluator.generated.h"

class AActor;
class UYcInventoryItemInstance;
class UYcInventoryManagerComponent;
struct FGridItemContextMenuAction;

/**
 * 属性比较运算符（用于 GAS 属性评估器）。
 * Attribute comparison operators (used by GAS-attribute evaluator).
 */
UENUM(BlueprintType)
enum class EYcContextAttributeComparisonOp : uint8
{
	Greater UMETA(DisplayName = ">"),
	GreaterOrEqual UMETA(DisplayName = ">="),
	Less UMETA(DisplayName = "<"),
	LessOrEqual UMETA(DisplayName = "<="),
	Equal UMETA(DisplayName = "=="),
	NotEqual UMETA(DisplayName = "!=")
};

/**
 * 标签匹配模式（用于 ASC/Item 标签评估器）。
 * Tag match modes (used by ASC/item-tag evaluators).
 */
UENUM(BlueprintType)
enum class EYcContextTagMatchMode : uint8
{
	HasTag UMETA(DisplayName = "HasTag"),
	NotHasTag UMETA(DisplayName = "NotHasTag"),
	HasAll UMETA(DisplayName = "HasAll"),
	HasAny UMETA(DisplayName = "HasAny"),
	NotHasAll UMETA(DisplayName = "NotHasAll"),
	NotHasAny UMETA(DisplayName = "NotHasAny"),
	MatchTagQuery UMETA(DisplayName = "MatchTagQuery"),
	NotMatchTagQuery UMETA(DisplayName = "NotMatchTagQuery")
};

/**
 * 右键动作评估上下文。
 * Runtime context passed into each context-action evaluator.
 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FYcContextActionEvalContext
{
	GENERATED_BODY()

	/** 发起右键动作请求的玩家Actor。 / Actor who requests the context action. */
	UPROPERTY(BlueprintReadWrite, Category = "ContextAction")
	TObjectPtr<AActor> Player = nullptr;

	/** 请求者库存组件（通常是玩家背包）。 / Requester's inventory component (usually player backpack). */
	UPROPERTY(BlueprintReadWrite, Category = "ContextAction")
	TObjectPtr<UYcInventoryManagerComponent> PlayerInventory = nullptr;

	/** 目标物品实例。 / Target item instance. */
	UPROPERTY(BlueprintReadWrite, Category = "ContextAction")
	TObjectPtr<UYcInventoryItemInstance> ItemInstance = nullptr;

	/** 物品当前所属库存组件。 / Inventory component that currently owns the item instance. */
	UPROPERTY(BlueprintReadWrite, Category = "ContextAction")
	TObjectPtr<UYcInventoryManagerComponent> ItemInventory = nullptr;

	/** 当前评估动作标签。 / Action tag being evaluated. */
	UPROPERTY(BlueprintReadWrite, Category = "ContextAction")
	FGameplayTag ActionTag;

	/** 当前评估执行器标签。 / Executor tag being evaluated. */
	UPROPERTY(BlueprintReadWrite, Category = "ContextAction")
	FGameplayTag ExecutorTag;

	/** 当前评估事件标签。 / Event tag being evaluated. */
	UPROPERTY(BlueprintReadWrite, Category = "ContextAction")
	FGameplayTag EventTag;

	/** 该玩家当前是否可操作此物品（如容器搜索已揭示）。 / Whether this item is currently operable for this player. */
	UPROPERTY(BlueprintReadWrite, Category = "ContextAction")
	bool bItemOperable = true;
};

/**
 * 右键动作评估结果（整条管线）。
 * Evaluation result of the whole policy pipeline.
 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FYcContextActionEvalResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "ContextAction")
	bool bPassed = true;

	UPROPERTY(BlueprintReadWrite, Category = "ContextAction")
	FText FailReason = FText::GetEmpty();

	UPROPERTY(BlueprintReadWrite, Category = "ContextAction", meta = (Categories = "Yc.Inventory.ContextAction.Evaluator"))
	FGameplayTag FailedEvaluatorTag;

	UPROPERTY(BlueprintReadWrite, Category = "ContextAction")
	FText FailedEvaluatorName = FText::GetEmpty();
};

/**
 * 右键动作评估器基类。
 * Base class for context-action evaluators.
 *
 * 设计约定 / Design notes:
 * - 子类只做“条件判断”，不做实际动作执行。
 *   Subclasses should only evaluate conditions, not execute gameplay actions.
 * - 客户端可用于预览，服务端必须复验。
 *   Client can preview; server must re-evaluate before execution.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class YCGRIDINVENTORYRUNTIME_API UYcInventoryContextActionEvaluator : public UObject
{
	GENERATED_BODY()

public:
	UYcInventoryContextActionEvaluator(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 执行当前评估器并返回是否通过。 / Evaluate this node and return pass/fail. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "ContextAction")
	bool Evaluate(const FYcContextActionEvalContext& Context, FText& OutFailReason) const;
	virtual bool Evaluate_Implementation(const FYcContextActionEvalContext& Context, FText& OutFailReason) const;

	/** 解析失败文案（优先使用传入文案，否则回退默认文案）。 / Resolve fail reason with fallback behavior. */
	UFUNCTION(BlueprintCallable, Category = "ContextAction")
	FText ResolveFailReason(const FText& InFailReason) const;

	/** 解析翻转失败文案（用于“原本通过但翻转后失败”的场景）。 / Resolve fail reason for inverted-failure case. */
	UFUNCTION(BlueprintCallable, Category = "ContextAction")
	FText ResolveInvertedFailReason() const;

public:
	/** 评估器显示名（用于UI/日志）。 / Display name for UI/logging. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction")
	FText EvaluatorName = FText::GetEmpty();

	/** 评估器标签（用于日志与诊断）。 / Evaluator tag used for logs and diagnostics. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction", meta = (Categories = "Yc.Inventory.ContextAction.Evaluator"))
	FGameplayTag EvaluatorTag;

	/** 默认失败原因（当评估器未给出具体失败文案时使用）。 / Default fail reason used when evaluator returns empty reason text. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction")
	FText DefaultFailReason = FText::GetEmpty();

	/** 结果翻转开关（通过变失败，失败变通过）。 / Invert final evaluator result (pass->fail, fail->pass). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction")
	bool bInvertResult = false;

	/** 翻转后失败文案（原始通过但翻转失败时使用）。 / Fail reason used when inversion turns a pass into a failure. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction")
	FText InvertedFailReason = FText::GetEmpty();
};

/** 物品是否可操作（如容器搜索已揭示）的基础评估器。 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class YCGRIDINVENTORYRUNTIME_API UYcEvaluator_ItemRevealed : public UYcInventoryContextActionEvaluator
{
	GENERATED_BODY()

public:
	UYcEvaluator_ItemRevealed(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool Evaluate_Implementation(const FYcContextActionEvalContext& Context, FText& OutFailReason) const override;
};

/** 玩家是否处于可操作状态（通常要求存活）。 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class YCGRIDINVENTORYRUNTIME_API UYcEvaluator_PlayerAlive : public UYcInventoryContextActionEvaluator
{
	GENERATED_BODY()

public:
	UYcEvaluator_PlayerAlive(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool Evaluate_Implementation(const FYcContextActionEvalContext& Context, FText& OutFailReason) const override;
};

/** 物品堆叠数量阈值评估器。 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class YCGRIDINVENTORYRUNTIME_API UYcEvaluator_HasStackCount : public UYcInventoryContextActionEvaluator
{
	GENERATED_BODY()

public:
	UYcEvaluator_HasStackCount(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool Evaluate_Implementation(const FYcContextActionEvalContext& Context, FText& OutFailReason) const override;

public:
	/** 通过该评估器所需的最小堆叠数量。 / Minimum stack count required to pass this evaluator. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction", meta = (ClampMin = "1"))
	int32 RequiredStackCount = 1;
};

/** 限制动作可用范围（玩家背包/外部容器）。 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class YCGRIDINVENTORYRUNTIME_API UYcEvaluator_InAllowedInventoryScope : public UYcInventoryContextActionEvaluator
{
	GENERATED_BODY()

public:
	UYcEvaluator_InAllowedInventoryScope(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool Evaluate_Implementation(const FYcContextActionEvalContext& Context, FText& OutFailReason) const override;

public:
	/** 来源为玩家背包时是否允许该动作。 / Whether action is allowed when source inventory is player inventory. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction")
	bool bAllowPlayerInventory = true;

	/** 来源为外部容器时是否允许该动作。 / Whether action is allowed when source inventory is container inventory. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction")
	bool bAllowContainerInventory = true;
};

/**
 * 通用 GAS 属性比较评估器。
 * Generic GAS-attribute comparison evaluator.
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class YCGRIDINVENTORYRUNTIME_API UYcEvaluator_GASAttributeCompare : public UYcInventoryContextActionEvaluator
{
	GENERATED_BODY()

public:
	UYcEvaluator_GASAttributeCompare(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual bool Evaluate_Implementation(const FYcContextActionEvalContext& Context, FText& OutFailReason) const override;

public:
	/** 要从玩家 ASC 读取的 GAS 属性。 / GAS attribute to read from player's ASC. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction|GAS")
	FGameplayAttribute Attribute;

	/** 比较运算符。 / Compare operator. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction|GAS")
	EYcContextAttributeComparisonOp ComparisonOp = EYcContextAttributeComparisonOp::GreaterOrEqual;

	/** 比较阈值。 / Threshold value for comparison. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction|GAS")
	float ThresholdValue = 0.0f;

	/** 数值容差（用于 Equal/NotEqual）。 / Numeric tolerance used by Equal/NotEqual comparisons. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction|GAS", meta = (ClampMin = "0.0"))
	float Tolerance = KINDA_SMALL_NUMBER;

	/**
	 * 启用后将比较 `(Attribute / MaxAttribute)` 与阈值。
	 * 常用于生命百分比判定（例如 `HealthPercent >= 0.4`）。
	 *
	 * When enabled, compare `(Attribute / MaxAttribute)` against threshold.
	 * Useful for percent checks like `HealthPercent >= 0.4`.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction|GAS")
	bool bUseNormalizedComparison = false;

	/** 归一化模式下使用的“最大值属性”。 / Max attribute used when normalized comparison is enabled. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction|GAS")
	FGameplayAttribute MaxAttribute;
};

/**
 * ASC GameplayTag 条件评估器（支持单标签、容器关系和 TagQuery）。
 * ASC gameplay-tag evaluator (supports single-tag, container relations, and TagQuery).
 */
UCLASS(BlueprintType, EditInlineNew, DefaultToInstanced)
class YCGRIDINVENTORYRUNTIME_API UYcEvaluator_ASCGameplayTagQuery : public UYcInventoryContextActionEvaluator
{
	GENERATED_BODY()

public:
	UYcEvaluator_ASCGameplayTagQuery(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

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

	/** 高级标签查询（用于 MatchTagQuery/NotMatchTagQuery）。 / Advanced tag query used by tag-query modes. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ContextAction|Tags")
	FGameplayTagQuery RequiredTagQuery;
};
