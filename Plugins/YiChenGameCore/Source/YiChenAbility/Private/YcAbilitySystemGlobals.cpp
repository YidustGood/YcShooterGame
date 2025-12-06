// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcAbilitySystemGlobals.h"

#include "YcGameplayEffectContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAbilitySystemGlobals)


UYcAbilitySystemGlobals::UYcAbilitySystemGlobals(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

FGameplayEffectContext* UYcAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FYcGameplayEffectContext();
}