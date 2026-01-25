// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Abilities/YcGameplayAbility.h"
#include "YcGameplayAbility_Death.generated.h"

/**
 * 死亡技能
 * 用于处理角色死亡的技能，实际逻辑是转发到Avatar actor的HealthComponent->StartDeath以及FinishDeath进行处理
 * 技能通过"GameplayEvent.Death"能力触发标签自动激活
 * 玩家血量归零后UYcShooterHealthComponent::HandleOutOfHealth()中会发送一个"GameplayEvent.Death"事件来触发这个死亡技能
 */
UCLASS(Abstract)
class YICHENGAMEPLAY_API UYcGameplayAbility_Death : public UYcGameplayAbility
{
	GENERATED_BODY()
public:
	/**
	 * 构造函数
	 * @param ObjectInitializer 对象初始化器
	 */
	UYcGameplayAbility_Death(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
protected:

	/**
	 * 激活技能
	 * 取消其他技能，设置为阻塞性排他技能，如果启用自动开始死亡则调用StartDeath
	 * @param Handle 技能规格句柄
	 * @param ActorInfo Actor信息
	 * @param ActivationInfo 激活信息
	 * @param TriggerEventData 触发事件数据
	 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	/**
	 * 结束技能
	 * 总是尝试完成死亡序列（如果死亡已开始）
	 * @param Handle 技能规格句柄
	 * @param ActorInfo Actor信息
	 * @param ActivationInfo 激活信息
	 * @param bReplicateEndAbility 是否复制结束技能
	 * @param bWasCancelled 是否被取消
	 */
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	/**
	 * 开始死亡序列
	 * 调用HealthComponent的StartDeath方法
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Ability")
	void StartDeath();

	/**
	 * 完成死亡序列
	 * 调用HealthComponent的FinishDeath方法
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Ability")
	void FinishDeath();

protected:
	/** 如果启用，技能将自动调用StartDeath。如果死亡已开始，FinishDeath总是在技能结束时被调用 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "YcGameCore|Death")
	bool bAutoStartDeath;
};