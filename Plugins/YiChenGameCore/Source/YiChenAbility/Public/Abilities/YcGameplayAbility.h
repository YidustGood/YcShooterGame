// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Abilities/GameplayAbility.h"
#include "YcGameplayAbility.generated.h"

/**
 * 插件提供的技能基类, 与插件提供的YcAbilitySystemComponent配合使用
 */
UCLASS(Abstract, HideCategories = Input, Meta = (ShortTooltip = "The base gameplay ability class used by this project."))
class YICHENABILITY_API UYcGameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	friend class UYcAbilitySystemComponent;
	
public:
	UYcGameplayAbility(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Ability")
	UYcAbilitySystemComponent* GetYcAbilitySystemComponentFromActorInfo() const;
	
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Ability")
	AController* GetControllerFromActorInfo() const;
};