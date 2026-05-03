// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "HoverTooltip/YcHoverTooltipTypes.h"
#include "YcHoverTooltip.generated.h"

class APlayerController;
class UUserWidget;

/**
 * 悬停提示辅助函数库。
 *
 * 负责把 Widget 的 hover 事件接到玩家控制器上的 Hover Tooltip 本地组件。
 */
UCLASS()
class YICHENGAMEUI_API UYcHoverTooltipLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Hover Tooltip")
	static void NotifyHoverEnter(UUserWidget* SourceWidget, FVector2D ScreenPosition);

	UFUNCTION(BlueprintCallable, Category = "Hover Tooltip")
	static void NotifyHoverMove(UUserWidget* SourceWidget, FVector2D ScreenPosition);

	UFUNCTION(BlueprintCallable, Category = "Hover Tooltip")
	static void NotifyHoverLeave(UUserWidget* SourceWidget);

	UFUNCTION(BlueprintCallable, Category = "Hover Tooltip")
	static void CancelHoverTooltipForWidget(UUserWidget* SourceWidget);

	UFUNCTION(BlueprintCallable, Category = "Hover Tooltip")
	static void CancelHoverTooltipForPlayer(APlayerController* PlayerController);

private:
	static bool BuildHoverTooltipRequest(UObject* SourceObject, FYcHoverTooltipRequest& OutRequest);
};
