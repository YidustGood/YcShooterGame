// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "YcGamePhaseAbility.h"
#include "YcGamePhaseAbility_Playing.generated.h"

/**
 * 案例代码, 用于展示使用方式, 无实际意义
 * 游戏阶段技能 - 游戏中(对局的正式对战阶段)
 */
UCLASS()
class YICHENGAMEPHASE_API UYcGamePhaseAbility_Playing : public UYcGamePhaseAbility
{
	GENERATED_BODY()
public:
	UYcGamePhaseAbility_Playing();
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
};
