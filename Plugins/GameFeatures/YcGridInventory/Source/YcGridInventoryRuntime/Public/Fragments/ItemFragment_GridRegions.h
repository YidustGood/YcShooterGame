// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "YcInventoryItemDefinition.h"
#include "ItemFragment_GridRegions.generated.h"

UENUM(BlueprintType)
enum class EGridRegionTagConstraintPolicy : uint8
{
	/** 不做标签限制。 / No tag constraint. */
	None = 0,
	/** 仅允许命中白名单标签。 / Allow only when allow-list matches. */
	AllowList = 1,
	/** 命中黑名单标签时禁止。 / Deny when deny-list matches. */
	DenyList = 2,
};

UENUM(BlueprintType)
enum class EGridRegionTagConstraintMatchMode : uint8
{
	/** 任意一个标签命中即视为匹配。 / Match succeeds if any tag matches. */
	Any = 0,
	/** 所有标签都命中才视为匹配。 / Match succeeds only when all tags match. */
	All = 1,
};

/**
 * 区域/口袋的物品标签约束配置。
 * Item tag constraint config for a region/pocket.
 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FGridRegionTagConstraint
{
	GENERATED_BODY()

	/** 约束策略（无/白名单/黑名单）。 / Constraint policy (none/allow/deny). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGridRegionTagConstraintPolicy Policy = EGridRegionTagConstraintPolicy::None;

	/** 标签匹配模式（任意或全部）。 / Tag match mode (any or all). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EGridRegionTagConstraintMatchMode MatchMode = EGridRegionTagConstraintMatchMode::Any;

	/** 参与匹配的标签集合。 / Tags used for matching. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGameplayTag> Tags;
};

/**
 * 区域局部可用形状单元（相对坐标）。
 * Region-local available shape cell (relative coordinate).
 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FGridRegionShapeCell
{
	GENERATED_BODY()

	/** 单元X坐标。 / Cell X coordinate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 X = 0;

	/** 单元Y坐标。 / Cell Y coordinate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Y = 0;

	/** 转换为FIntPoint。 / Convert to FIntPoint. */
	FIntPoint ToPoint() const
	{
		return FIntPoint(X, Y);
	}
};

/**
 * 单个区域下的口袋定义（可用于多分页/分槽布局）。
 * Pocket definition under one region (for multi-page or sub-slot layouts).
 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FGridInventoryPocketDefinition
{
	GENERATED_BODY()

	/** 口袋显示名。 / Pocket display name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName = FText::GetEmpty();

	/** 口袋排序优先级（越小越靠前）。 / Pocket sort priority (smaller first). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Priority = 0;

	/** 口袋基础尺寸（格子数）。 / Pocket base dimensions (in tiles). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint Dimensions = FIntPoint(1, 1);

	/** 可用形状单元；为空时通常按矩形Dimensions处理。 / Available shape cells; empty usually means rectangular Dimensions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGridRegionShapeCell> ShapeCells;

	/** UI布局偏移（格子坐标）。 / UI layout offset (tile coordinate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint LayoutOffset = FIntPoint::ZeroValue;
};

/**
 * 单个库存区域定义，可由装备在运行时提供。
 * One inventory region definition, can be granted by equipment at runtime.
 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FGridInventoryRegionDefinition
{
	GENERATED_BODY()

	/** 区域唯一标签ID。 / Unique gameplay-tag id of this region. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGameplayTag RegionId;

	/** 区域显示名。 / Region display name. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FText DisplayName = FText::GetEmpty();

	/** 区域排序优先级（越小越靠前）。 / Region sort priority (smaller first). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Priority = 100;

	/** 区域基础尺寸（格子数）。 / Region base dimensions (in tiles). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint Dimensions = FIntPoint(1, 1);

	/** 区域可用形状单元；为空时通常按矩形Dimensions处理。 / Available shape cells; empty usually means rectangular Dimensions. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGridRegionShapeCell> ShapeCells;

	/** 区域在UI中的布局偏移（格子坐标）。 / Region layout offset in UI (tile coordinate). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FIntPoint LayoutOffset = FIntPoint::ZeroValue;

	/**
	 * 区域级物品放置标签约束（当口袋未覆盖时生效）。
	 * Per-region item placement tag constraint (used when pocket has no override).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FGridRegionTagConstraint ItemTagConstraint;

	/** 区域下的口袋列表。 / Pocket definitions under this region. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGridInventoryPocketDefinition> Pockets;
};

/**
 * 装备类物品片段：为持有者提供运行时网格区域定义。
 * Equipment fragment that grants runtime grid-region definitions.
 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FItemFragment_GridRegions : public FYcInventoryItemFragment
{
	GENERATED_BODY()

	/** 由该物品提供的区域定义集合。 / Region definitions provided by this item. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FGridInventoryRegionDefinition> Regions;
};
