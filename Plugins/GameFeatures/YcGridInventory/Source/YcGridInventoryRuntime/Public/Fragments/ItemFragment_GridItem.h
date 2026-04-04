// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "YcInventoryItemDefinition.h"

#include "ItemFragment_GridItem.generated.h"

/**
 * 网格库存物品 Fragment：定义物品在网格中的尺寸、图标表现与搜索时长。
 * Grid-inventory item fragment: defines tile size, icon presentation and search duration.
 *
 * 配置说明 / Configuration Notes:
 * - 本结构体应配置在物品定义（ItemDefinition）的 Fragments 列表中。
 *   This struct should be configured in the item definition Fragments list.
 * - Dimensions 决定占用格子范围；Icon/IconTexture/IconRotated 决定UI表现。
 *   Dimensions controls occupied tiles; Icon/IconTexture/IconRotated drive UI visuals.
 * - SearchDuration 用于容器搜索系统（容器开启搜索功能时生效）。
 *   SearchDuration is used by container-search flow (only when container search is enabled).
 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FItemFragment_GridItem : public FYcInventoryItemFragment
{
	GENERATED_BODY()

	FItemFragment_GridItem(const int32 X, const int32 Y)
		: Dimensions(FIntPoint(X, Y))
		, Icon(nullptr)
		, IconTexture(nullptr)
		, IconRotated(nullptr)
	{
	}

	FItemFragment_GridItem()
		: Dimensions(FIntPoint(1, 1))
		, Icon(nullptr)
		, IconTexture(nullptr)
		, IconRotated(nullptr)
	{
	}

	/**
	 * 物品占用网格尺寸（X=宽，Y=高，单位：格子）。
	 * Item size in grid tiles (X = width, Y = height, unit = tile).
	 *
	 * 示例 / Example:
	 * - 手枪 Pistol: (2,1)
	 * - 步枪 Rifle: (3,1) 或 (2,2)（按你的UI设计）
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FIntPoint Dimensions;

	/**
	 * 物品图标材质（可选）。
	 * Item icon material (optional).
	 *
	 * 说明 / Note:
	 * - 若使用材质驱动图标显示，可在材质中采样 IconTexture。
	 *   If your UI uses material-driven rendering, sample IconTexture in this material.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UMaterialInterface* Icon;

	/**
	 * 物品图标贴图（可选）。
	 * Item icon texture (optional).
	 *
	 * 说明 / Note:
	 * - 可直接用于UI图片，或作为 Icon/IconRotated 材质参数输入。
	 *   Can be used directly by UI image, or as texture input for Icon/IconRotated materials.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UTexture* IconTexture;

	/**
	 * 旋转状态下使用的图标材质（可选）。
	 * Icon material used when item is rendered as rotated (optional).
	 *
	 * 建议 / Recommendation:
	 * - 若旋转前后视觉一致，可与 Icon 复用同一材质。
	 *   Reuse Icon material if rotated and non-rotated visuals are identical.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UMaterialInterface* IconRotated;

	/**
	 * 是否允许在网格中旋转（宽高互换）。
	 * Whether this item can be rotated in grid (swap width/height).
	 *
	 * 建议 / Recommendation:
	 * - 细长物品通常开启（true），固定朝向物品可关闭（false）。
	 *   Usually true for elongated items; set false for fixed-orientation items.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	bool bCanRotate = true;

	/**
	 * 容器搜索基础时长（秒）。
	 * Base search duration in container (seconds).
	 *
	 * 规则 / Rules:
	 * - 当容器启用搜索功能时，玩家看到该物品前需要完成搜索计时。
	 *   When container-search is enabled, player must finish timer before item is revealed.
	 * - <= 0 表示无需搜索，立即可见。
	 *   <= 0 means no search required (revealed immediately).
	 * - 实际耗时通常为：SearchDuration / 搜索速度倍率。
	 *   Effective duration is typically: SearchDuration / SearchSpeedMultiplier.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float SearchDuration = 0.0f;
};

/**
 * 网格绘制辅助线段（通常用于调试或UI绘制缓存）。
 * Grid line segment helper (usually for debug drawing or UI render cache).
 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FGridInventoryLine
{
	GENERATED_BODY()

	/** 线段起点（UI坐标）。 / Line start point (UI space). */
	UPROPERTY(BlueprintReadWrite)
	FVector2D Start;

	/** 线段终点（UI坐标）。 / Line end point (UI space). */
	UPROPERTY(BlueprintReadWrite)
	FVector2D End;
};
