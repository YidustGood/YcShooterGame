// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CommonActivatableWidget.h"
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HoverTooltip/YcHoverTooltipTypes.h"
#include "YcHoverTooltipProviderWidget.generated.h"

/**
 * 悬停提示 Provider 的通用 Widget 基类。
 *
 * AngelScript/Blueprint Widget 可直接继承它来覆写 Tooltip 数据提供逻辑。
 */
UCLASS(Abstract, Blueprintable)
class YICHENGAMEUI_API UYcHoverTooltipProviderWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hover Tooltip")
	bool CanShowHoverTooltip() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hover Tooltip")
	TSubclassOf<UCommonActivatableWidget> GetHoverTooltipWidgetClass() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hover Tooltip")
	FInstancedStruct BuildHoverTooltipPayload() const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hover Tooltip")
	FYcHoverTooltipDisplayConfig GetHoverTooltipDisplayConfig() const;

protected:
	/** 当前悬停目标默认使用的 Tooltip Widget 类。 */
	UPROPERTY(EditDefaultsOnly, Category = "Hover Tooltip")
	TSubclassOf<UCommonActivatableWidget> HoverTooltipWidgetClass;

	/** 当前悬停目标默认使用的 Tooltip 显示策略。 */
	UPROPERTY(EditDefaultsOnly, Category = "Hover Tooltip")
	FYcHoverTooltipDisplayConfig HoverTooltipDisplayConfig;
};
