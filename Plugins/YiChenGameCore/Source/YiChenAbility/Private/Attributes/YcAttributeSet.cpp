// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Attributes/YcAttributeSet.h"

#include "YcAbilitySystemComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAttributeSet)

UYcAttributeSet::UYcAttributeSet()
{
}

UWorld* UYcAttributeSet::GetWorld() const
{
	// 通过外部对象（通常是AbilitySystemComponent）获取世界对象
	const UObject* Outer = GetOuter();
	check(Outer);

	return Outer->GetWorld();
}

UYcAbilitySystemComponent* UYcAttributeSet::GetYcAbilitySystemComponent() const
{
	// 将父类的AbilitySystemComponent转换为UYcAbilitySystemComponent类型
	return Cast<UYcAbilitySystemComponent>(GetOwningAbilitySystemComponent());
}