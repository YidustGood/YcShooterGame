// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "YcAbilitySystemGlobals.generated.h"

/**
 * 技能系统全局配置类
 * 
 * 自定义UAbilitySystemGlobals，用于创建自定义的FYcGameplayEffectContext。
 * 需要在DefaultGame.ini中进行配置指定：
 * 
 * [/Script/GameplayAbilities.AbilitySystemGlobals]
 * AbilitySystemGlobalsClassName=/Script/YiChenAbility.UYcAbilitySystemGlobals
 */
UCLASS(Config=Game)
class YICHENABILITY_API UYcAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_UCLASS_BODY()

	//~UAbilitySystemGlobals interface
	/** 创建自定义的FYcGameplayEffectContext实例 */
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
	//~End of UAbilitySystemGlobals interface
};