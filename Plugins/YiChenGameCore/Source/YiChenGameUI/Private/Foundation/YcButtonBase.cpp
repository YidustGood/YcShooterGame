// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Foundation/YcButtonBase.h"

#include "CommonActionWidget.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcButtonBase)

void UYcButtonBase::SetButtonText(const FText& InText)
{
	bOverride_ButtonText = InText.IsEmpty();
	ButtonText = InText;
	RefreshButtonText();
}

void UYcButtonBase::NativePreConstruct()
{
	Super::NativePreConstruct();

	UpdateButtonStyle();
	RefreshButtonText();
}

void UYcButtonBase::UpdateInputActionWidget()
{
	Super::UpdateInputActionWidget();
    
	UpdateButtonStyle();
	RefreshButtonText();
}

void UYcButtonBase::OnInputMethodChanged(ECommonInputType CurrentInputType)
{
	Super::OnInputMethodChanged(CurrentInputType);

	UpdateButtonStyle();
}

void UYcButtonBase::RefreshButtonText()
{
	if (bOverride_ButtonText || ButtonText.IsEmpty())
	{
		if (InputActionWidget)
		{
			const FText ActionDisplayText = InputActionWidget->GetDisplayText();	
			if (!ActionDisplayText.IsEmpty())
			{
				UpdateButtonText(ActionDisplayText);
				return;
			}
		}
	}
	
	UpdateButtonText(ButtonText);
}