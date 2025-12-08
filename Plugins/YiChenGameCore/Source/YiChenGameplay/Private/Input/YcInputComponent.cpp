// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Input/YcInputComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcInputComponent)

UYcInputComponent::UYcInputComponent(const FObjectInitializer& ObjectInitializer)
{
}

void UYcInputComponent::AddInputMappings(const UYcInputConfig* InputConfig, const UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

	// 此处可处理自定义逻辑，用于在需要时从输入配置中添加特定内容
}

void UYcInputComponent::RemoveInputMappings(const UYcInputConfig* InputConfig, const UEnhancedInputLocalPlayerSubsystem* InputSubsystem) const
{
	check(InputConfig);
	check(InputSubsystem);

	// 此处可处理自定义逻辑，用于移除先前添加的输入映射
}

void UYcInputComponent::RemoveBinds(TArray<uint32>& BindHandles)
{
	for (const uint32 Handle : BindHandles)
	{
		RemoveBindingByHandle(Handle);
	}
	BindHandles.Reset();
}