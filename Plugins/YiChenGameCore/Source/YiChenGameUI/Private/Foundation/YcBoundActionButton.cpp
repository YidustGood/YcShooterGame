// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Foundation/YcBoundActionButton.h"

#include "CommonInputSubsystem.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcBoundActionButton)

void UYcBoundActionButton::NativeConstruct()
{
	Super::NativeConstruct();

	// 绑定输入方式发生变化后的回调函数
	if (UCommonInputSubsystem* InputSubsystem = GetInputSubsystem())
	{
		InputSubsystem->OnInputMethodChangedNative.AddUObject(this, &ThisClass::HandleInputMethodChanged);
		HandleInputMethodChanged(InputSubsystem->GetCurrentInputType());
	}
}

void UYcBoundActionButton::HandleInputMethodChanged(const ECommonInputType NewInputMethod)
{
	TSubclassOf<UCommonButtonStyle> NewStyle = nullptr;

	if (NewInputMethod == ECommonInputType::Gamepad)
	{
		NewStyle = GamepadStyle;
	}
	else if (NewInputMethod == ECommonInputType::Touch)
	{
		NewStyle = TouchStyle;
	}
	else
	{
		NewStyle = KeyboardStyle;
	}

	if (NewStyle)
	{
		SetStyle(NewStyle);
	}
}