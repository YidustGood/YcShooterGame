// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "HoverTooltip/YcHoverTooltipWidgetBase.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcHoverTooltipWidgetBase)

bool UYcHoverTooltipWidgetBase::GetHoverTooltipWidgetPayload(FYcHoverTooltipWidgetPayload& OutPayload) const
{
	return ResolveCurrentHoverTooltipPayload(OutPayload);
}

void UYcHoverTooltipWidgetBase::GetHoverTooltipContentPayload(FInstancedStruct& OutPayload) const
{
	FYcHoverTooltipWidgetPayload Payload;
	if (!ResolveCurrentHoverTooltipPayload(Payload))
	{
		OutPayload = FInstancedStruct();
		return;
	}

	OutPayload = Payload.ContentPayload;
}

bool UYcHoverTooltipWidgetBase::ShouldHoverTooltipFollowCursor() const
{
	FYcHoverTooltipWidgetPayload Payload;
	return ResolveCurrentHoverTooltipPayload(Payload) && Payload.bFollowCursor;
}

FVector2D UYcHoverTooltipWidgetBase::ResolveHoverTooltipScreenPosition(FVector2D TooltipSize) const
{
	FYcHoverTooltipWidgetPayload Payload;
	if (!ResolveCurrentHoverTooltipPayload(Payload))
	{
		return FVector2D::ZeroVector;
	}

	FVector2D PixelPosition = FVector2D::ZeroVector;
	FVector2D ViewportPosition = FVector2D::ZeroVector;
	USlateBlueprintLibrary::AbsoluteToViewport(const_cast<UYcHoverTooltipWidgetBase*>(this), Payload.ScreenPosition, PixelPosition, ViewportPosition);

	FVector2D ScreenPosition = ViewportPosition + Payload.ScreenOffset;
	if (!Payload.bClampToViewport)
	{
		return ScreenPosition;
	}

	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(const_cast<UYcHoverTooltipWidgetBase*>(this));
	ScreenPosition.X = FMath::Clamp(ScreenPosition.X, 0.0f, FMath::Max(0.0f, ViewportSize.X - TooltipSize.X));
	ScreenPosition.Y = FMath::Clamp(ScreenPosition.Y, 0.0f, FMath::Max(0.0f, ViewportSize.Y - TooltipSize.Y));
	return ScreenPosition;
}

FVector2D UYcHoverTooltipWidgetBase::ResolveHoverTooltipScreenPositionFromDesiredSize() const
{
	return ResolveHoverTooltipScreenPosition(GetDesiredSize());
}

void UYcHoverTooltipWidgetBase::RefreshHoverTooltipViewportPosition()
{
	ForceLayoutPrepass();

	FVector2D TooltipSize = GetDesiredSize();
	if (TooltipSize.X <= 1.0f || TooltipSize.Y <= 1.0f)
	{
		TooltipSize = FVector2D(180.0f, 48.0f);
	}

	SetAlignmentInViewport(FVector2D::ZeroVector);
	SetPositionInViewport(ResolveHoverTooltipScreenPosition(TooltipSize), false);
}

void UYcHoverTooltipWidgetBase::OnDynamicScreenWidgetPayloadUpdated_Implementation()
{
	OnHoverTooltipPayloadUpdated();
	RefreshHoverTooltipViewportPosition();
}

void UYcHoverTooltipWidgetBase::OnDynamicScreenWidgetPayloadHidden_Implementation()
{
	OnHoverTooltipPayloadHidden();
}

bool UYcHoverTooltipWidgetBase::ResolveCurrentHoverTooltipPayload(FYcHoverTooltipWidgetPayload& OutPayload) const
{
	FInstancedStruct Payload;
	GetDynamicScreenWidgetPayload(Payload);
	if (!Payload.IsValid())
	{
		return false;
	}

	const FYcHoverTooltipWidgetPayload* Value = Payload.GetPtr<FYcHoverTooltipWidgetPayload>();
	if (!Value)
	{
		return false;
	}

	OutPayload = *Value;
	return true;
}
