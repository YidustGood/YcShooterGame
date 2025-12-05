// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "AbilitySystemComponent.h"
#include "YcAbilitySystemComponent.generated.h"

/**
 * 插件提供的技能组件基类
 */
UCLASS(ClassGroup=(YiChenGameCore), meta=(BlueprintSpawnableComponent))
class YICHENABILITY_API UYcAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
public:
	UYcAbilitySystemComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	/* 获取该类型ASC组件的静态函数 **/
	static UYcAbilitySystemComponent* GetAbilitySystemComponentFromActor(const AActor* Actor, bool LookForComponent = false);
	
	/* 尝试通过技能类型取消技能 **/
	UFUNCTION(BlueprintCallable, Category="AbilitySystem")
	bool TryCancelAbilityByClass(TSubclassOf<UGameplayAbility> InAbilityToCancel);
};
