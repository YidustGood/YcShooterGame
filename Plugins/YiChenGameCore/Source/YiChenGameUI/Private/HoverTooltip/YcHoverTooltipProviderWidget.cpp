// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "HoverTooltip/YcHoverTooltipProviderWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcHoverTooltipProviderWidget)

bool UYcHoverTooltipProviderWidget::CanShowHoverTooltip_Implementation() const
{
	return HoverTooltipWidgetClass != nullptr;
}

TSubclassOf<UCommonActivatableWidget> UYcHoverTooltipProviderWidget::GetHoverTooltipWidgetClass_Implementation() const
{
	return HoverTooltipWidgetClass;
}

FInstancedStruct UYcHoverTooltipProviderWidget::BuildHoverTooltipPayload_Implementation() const
{
	return FInstancedStruct();
}

FYcHoverTooltipDisplayConfig UYcHoverTooltipProviderWidget::GetHoverTooltipDisplayConfig_Implementation() const
{
	return HoverTooltipDisplayConfig;
}
