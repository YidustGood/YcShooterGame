// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Abilities/YcGameplayAbility.h"
#include "YcGameplayAbility_DeathBase.generated.h"

/**
 * CombatCore 的死亡序列技能基类。
 *
 * 职责：
 * - 监听 GameplayEvent.Death 并激活技能
 * - 取消非生存类技能并切换到阻塞激活组
 * - 协调 StartDeath ->（技能激活期）-> FinishDeath
 *
 * 游戏侧可直接继承本类，或通过兼容包装类继承。
 */
UCLASS(Abstract)
class YICHENCOMBATCORE_API UYcGameplayAbility_DeathBase : public UYcGameplayAbility
{
	GENERATED_BODY()

public:
	UYcGameplayAbility_DeathBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/** 在 Avatar 的 HealthComponent 上开始死亡序列。 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Ability")
	virtual void StartDeath();

	/** 在 Avatar 的 HealthComponent 上完成死亡序列。 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Ability")
	virtual void FinishDeath();

	/** 死亡序列进入“死亡中”窗口时触发（蓝图扩展点）。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "YcGameCore|Death", DisplayName = "OnDeathSequenceStarted")
	void K2_OnDeathSequenceStarted();

	/** 即将结束死亡序列前触发（蓝图扩展点）。 */
	UFUNCTION(BlueprintImplementableEvent, Category = "YcGameCore|Death", DisplayName = "OnDeathSequenceEnding")
	void K2_OnDeathSequenceEnding();

protected:
	/** 是否在激活技能时自动调用 StartDeath。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YcGameCore|Death")
	bool bAutoStartDeath = true;
};
