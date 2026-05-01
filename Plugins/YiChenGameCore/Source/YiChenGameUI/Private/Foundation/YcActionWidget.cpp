// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Foundation/YcActionWidget.h"

#include "CommonInputBaseTypes.h"
#include "CommonInputSubsystem.h"
#include "EnhancedInputSubsystems.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcActionWidget)

FSlateBrush UYcActionWidget::GetIcon() const
{
	// 如果当前控件关联了 Enhanced Input Action，
	// 则优先查找该动作实际绑定的按键，并显示对应图标，而不是使用默认数据表中的配置。
	// 这样可以覆盖玩家重绑定按键后的显示场景。
	const UEnhancedInputLocalPlayerSubsystem* EnhancedInputSubsystem = GetEnhancedInputSubsystem();
	if (!AssociatedInputAction || !EnhancedInputSubsystem) return Super::GetIcon();

	// 获取当前映射到该输入动作的按键列表。
	TArray<FKey> BoundKeys = EnhancedInputSubsystem->QueryKeysMappedToAction(AssociatedInputAction);
	FSlateBrush SlateBrush;

	const UCommonInputSubsystem* CommonInputSubsystem = GetInputSubsystem();
	if (!BoundKeys.IsEmpty() && CommonInputSubsystem && UCommonInputPlatformSettings::Get()->TryGetInputBrush(SlateBrush, BoundKeys[0], CommonInputSubsystem->GetCurrentInputType(), CommonInputSubsystem->GetCurrentGamepadName()))
	{
		return SlateBrush;
	}
	
	return Super::GetIcon();
}

UEnhancedInputLocalPlayerSubsystem* UYcActionWidget::GetEnhancedInputSubsystem() const
{
	const UWidget* BoundWidget = DisplayedBindingHandle.GetBoundWidget();
	if (const ULocalPlayer* BindingOwner = BoundWidget ? BoundWidget->GetOwningLocalPlayer() : GetOwningLocalPlayer())
	{
		return BindingOwner->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
	}
	return nullptr;
}
