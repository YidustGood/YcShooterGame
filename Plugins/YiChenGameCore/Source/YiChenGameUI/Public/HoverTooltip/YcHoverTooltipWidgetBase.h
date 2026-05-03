// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DynamicScreenUI/YcDynamicScreenWidgetBase.h"
#include "HoverTooltip/YcHoverTooltipTypes.h"
#include "YcHoverTooltipWidgetBase.generated.h"

/**
 * Tooltip 动态 Widget 的通用基类。
 *
 * 它负责暴露统一 wrapper payload，并提供屏幕位置解析/钳制工具函数。
 * 具体内容展示仍由业务 Tooltip Widget 自己处理。
 */
UCLASS(Abstract, Blueprintable)
class YICHENGAMEUI_API UYcHoverTooltipWidgetBase : public UYcDynamicScreenWidgetBase
{
	GENERATED_BODY()

public:
	/** 获取当前 tooltip 的统一 wrapper payload。 */
	UFUNCTION(BlueprintCallable, Category = "Hover Tooltip")
	bool GetHoverTooltipWidgetPayload(FYcHoverTooltipWidgetPayload& OutPayload) const;

	/** 获取当前 tooltip 的业务内容 payload。 */
	UFUNCTION(BlueprintCallable, Category = "Hover Tooltip")
	void GetHoverTooltipContentPayload(FInstancedStruct& OutPayload) const;

	/** 是否配置为跟随鼠标。 */
	UFUNCTION(BlueprintPure, Category = "Hover Tooltip")
	bool ShouldHoverTooltipFollowCursor() const;

	/** 使用传入尺寸计算钳制后的最终屏幕位置。 */
	UFUNCTION(BlueprintPure, Category = "Hover Tooltip")
	FVector2D ResolveHoverTooltipScreenPosition(FVector2D TooltipSize) const;

	/** 使用当前 Widget 的 DesiredSize 计算钳制后的最终屏幕位置。 */
	UFUNCTION(BlueprintPure, Category = "Hover Tooltip")
	FVector2D ResolveHoverTooltipScreenPositionFromDesiredSize() const;

	/** 直接将 Tooltip Widget 本身放置到最终的 viewport 位置。 */
	UFUNCTION(BlueprintCallable, Category = "Hover Tooltip")
	void RefreshHoverTooltipViewportPosition();

protected:
	virtual void OnDynamicScreenWidgetPayloadUpdated_Implementation() override;
	virtual void OnDynamicScreenWidgetPayloadHidden_Implementation() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Hover Tooltip")
	void OnHoverTooltipPayloadUpdated();

	UFUNCTION(BlueprintImplementableEvent, Category = "Hover Tooltip")
	void OnHoverTooltipPayloadHidden();

private:
	bool ResolveCurrentHoverTooltipPayload(FYcHoverTooltipWidgetPayload& OutPayload) const;
};
