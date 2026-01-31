// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "UObject/Object.h"
#include "YcAbilitySystemComponentMixinLibrary.generated.h"

/**
 * 
 */
UCLASS(Meta = (ScriptMixin = "UAbilitySystemComponent"))
class YCANGELSCRIPTMIXIN_API UYcAbilitySystemComponentMixinLibrary : public UObject
{
	GENERATED_BODY()
public:
	// 从自生的CDO对象复制一份新的实例对象
	UFUNCTION(ScriptCallable)
	static bool HasMatchingGameplayTag(const UAbilitySystemComponent* ASC,const FGameplayTag& Tag)
	{
		if (ASC == nullptr) return false;
		return ASC->HasMatchingGameplayTag(Tag);
	}
};
