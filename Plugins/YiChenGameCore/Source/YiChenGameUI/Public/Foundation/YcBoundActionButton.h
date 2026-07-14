// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Input/CommonBoundActionButton.h"
#include "YcBoundActionButton.generated.h"

/**
 * 
 */
UCLASS(Abstract, meta = (DisableNativeTick))
class YICHENGAMEUI_API UYcBoundActionButton : public UCommonBoundActionButton
{
	GENERATED_BODY()
protected:
	virtual void NativeConstruct() override;

private:
	
	/**
	 * 根据不同的输入方式设置不同的按钮样式
	 * @param NewInputMethod 新的输入方式
	 */
	void HandleInputMethodChanged(ECommonInputType NewInputMethod);

	UPROPERTY(EditAnywhere, Category = "Styles")
	TSubclassOf<UCommonButtonStyle> KeyboardStyle;

	UPROPERTY(EditAnywhere, Category = "Styles")
	TSubclassOf<UCommonButtonStyle> GamepadStyle;

	UPROPERTY(EditAnywhere, Category = "Styles")
	TSubclassOf<UCommonButtonStyle> TouchStyle;
};
