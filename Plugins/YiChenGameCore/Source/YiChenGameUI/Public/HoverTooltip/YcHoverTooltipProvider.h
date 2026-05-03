// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "HoverTooltip/YcHoverTooltipTypes.h"
#include "YcHoverTooltipProvider.generated.h"

class UCommonActivatableWidget;

/**
 * 通用悬停提示 Provider 接口。
 *
 * 用于让任意对象按需提供 Tooltip WidgetClass / Payload / 显示策略。
 */
UINTERFACE(BlueprintType)
class YICHENGAMEUI_API UYcHoverTooltipProvider : public UInterface
{
	GENERATED_BODY()
};

class YICHENGAMEUI_API IYcHoverTooltipProvider
{
	GENERATED_BODY()

public:
	/** 当前是否允许显示 Tooltip。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hover Tooltip")
	bool CanShowHoverTooltip() const;

	/** 当前悬停目标希望显示的 Tooltip Widget 类。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hover Tooltip")
	TSubclassOf<UCommonActivatableWidget> GetHoverTooltipWidgetClass() const;

	/** 由业务层构造具体展示内容。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hover Tooltip")
	FInstancedStruct BuildHoverTooltipPayload() const;

	/** 获取 Tooltip 的通用显示策略。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Hover Tooltip")
	FYcHoverTooltipDisplayConfig GetHoverTooltipDisplayConfig() const;
};
