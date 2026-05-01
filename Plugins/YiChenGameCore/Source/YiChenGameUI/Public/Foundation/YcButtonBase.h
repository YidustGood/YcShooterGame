// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonButtonBase.h"
#include "YcButtonBase.generated.h"

/**
 * 通用按钮基类。
 * 
 * 提供按钮文本刷新、输入方式切换后的样式更新，以及与输入动作提示联动的基础能力，
 * 便于在蓝图中派生统一风格的交互按钮。
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class YICHENGAMEUI_API UYcButtonBase : public UCommonButtonBase
{
	GENERATED_BODY()
public:
	/** 设置按钮显示文本。 */
	UFUNCTION(BlueprintCallable)
	void SetButtonText(const FText& InText);
	
protected:
	/** 预构建阶段刷新按钮样式与文本。 */
	virtual void NativePreConstruct() override;

	/** 输入动作控件更新后同步刷新按钮外观。 */
	virtual void UpdateInputActionWidget() override;
	/** 输入设备类型切换时刷新按钮样式。 */
	virtual void OnInputMethodChanged(ECommonInputType CurrentInputType) override;

	/** 根据覆盖文本和输入动作显示文本刷新按钮文案。 */
	void RefreshButtonText();
	
	/** 由蓝图实现具体的按钮文本更新逻辑。 */
	UFUNCTION(BlueprintImplementableEvent)
	void UpdateButtonText(const FText& InText);

	/** 由蓝图实现具体的按钮样式更新逻辑。 */
	UFUNCTION(BlueprintImplementableEvent)
	void UpdateButtonStyle();
	
private:
	/** 是否使用自定义按钮文本。 */
	UPROPERTY(EditAnywhere, Category="Button", meta=(InlineEditConditionToggle))
	uint8 bOverride_ButtonText : 1;
	
	/** 自定义按钮文本内容。 */
	UPROPERTY(EditAnywhere, Category="Button", meta=( editcondition="bOverride_ButtonText" ))
	FText ButtonText;
};
