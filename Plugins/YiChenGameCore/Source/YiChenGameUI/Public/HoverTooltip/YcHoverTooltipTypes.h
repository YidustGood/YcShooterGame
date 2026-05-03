// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CommonActivatableWidget.h"
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "YcHoverTooltipTypes.generated.h"

/**
 * 悬停提示的显示策略。
 *
 * 该配置只描述通用表现策略，不关心具体展示内容。
 */
USTRUCT(BlueprintType)
struct YICHENGAMEUI_API FYcHoverTooltipDisplayConfig
{
	GENERATED_BODY()

	FYcHoverTooltipDisplayConfig()
		: WidgetLayer(FGameplayTag::RequestGameplayTag(TEXT("UI.Layer.Game"), false))
	{
	}

	/** 悬停多久后才显示提示框。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Tooltip")
	float DelaySeconds = 0.5f;

	/** 提示框显示后是否持续跟随鼠标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Tooltip")
	bool bFollowCursor = true;

	/** 鼠标锚点到提示框的额外偏移。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Tooltip")
	FVector2D ScreenOffset = FVector2D(16.0f, 16.0f);

	/** 是否在视口内进行位置钳制。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Tooltip")
	bool bClampToViewport = true;

	/** 兼容保留字段。Hover Tooltip 当前使用本地屏幕浮层显示，不再参与 CommonUI Layer 栈。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Tooltip")
	FGameplayTag WidgetLayer;
};

/**
 * Provider 产出的通用悬停提示请求。
 *
 * 业务层只负责提供 WidgetClass + Payload + 通用显示策略。
 */
USTRUCT(BlueprintType)
struct YICHENGAMEUI_API FYcHoverTooltipRequest
{
	GENERATED_BODY()

	/** 当前悬停目标要显示的 Tooltip Widget 类。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Tooltip")
	TSubclassOf<UCommonActivatableWidget> WidgetClass;

	/** 具体业务内容载荷。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Tooltip")
	FInstancedStruct Payload;

	/** 通用显示策略。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Tooltip")
	FYcHoverTooltipDisplayConfig DisplayConfig;
};

/**
 * 真正送给 Tooltip 动态 Widget 的统一载荷。
 *
 * 其中包含框架层的位置/策略信息，以及业务内容 payload。
 */
USTRUCT(BlueprintType)
struct YICHENGAMEUI_API FYcHoverTooltipWidgetPayload
{
	GENERATED_BODY()

	/** 当前 tooltip 的鼠标锚点屏幕坐标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Tooltip")
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	/** 锚点到 tooltip 的偏移量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Tooltip")
	FVector2D ScreenOffset = FVector2D(16.0f, 16.0f);

	/** 是否持续跟随鼠标。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Tooltip")
	bool bFollowCursor = true;

	/** 是否在视口中钳制位置。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Tooltip")
	bool bClampToViewport = true;

	/** 业务层真正关心的数据。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hover Tooltip")
	FInstancedStruct ContentPayload;
};
