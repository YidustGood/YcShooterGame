// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayAbilitySpec.h"
#include "YcAbilityCost.generated.h"

class UYcGameplayAbility;

/**
 * 插件提供的自定义技能成本基类
 * 定义技能激活时需要支付的额外成本（如弹药、充能等）
 * 支持蓝图实现，可通过K2_CheckCost和K2_ApplyCost在蓝图中自定义成本检查和应用逻辑
 */
UCLASS(Blueprintable, BlueprintType, DefaultToInstanced, EditInlineNew, Abstract)
class YICHENABILITY_API UYcAbilityCost : public UObject
{
	GENERATED_BODY()
public:
	UYcAbilityCost();

	/**
	 * 检查是否可以支付此成本
	 * 在技能激活前调用，用于验证是否有足够的资源支付成本
	 * 如果成本不足，可以向OptionalRelevantTags添加失败原因标签，用于在其他地方查询并提供用户反馈
	 * （例如：武器弹药不足时播放点击声）
	 * 
	 * 注意：Ability和ActorInfo在调用时保证非空，但OptionalRelevantTags可能为nullptr
	 * 
	 * @param Ability 要检查成本的技能
	 * @param Handle 技能规格句柄
	 * @param ActorInfo Actor信息
	 * @param OptionalRelevantTags 可选的输出相关标签容器，可用于添加失败原因标签
	 * @return 如果可以支付成本则返回true，否则返回false
	 */
	virtual bool CheckCost(const UYcGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle,
	                       const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags);
	
	/**
	 * 应用技能的成本
	 * 在技能激活后调用，扣除相应的资源（如弹药、充能等）
	 * 
	 * 注意：
	 * - 实现时无需检查ShouldOnlyApplyCostOnHit()，调用者会处理该逻辑
	 * - Ability和ActorInfo在调用时保证非空
	 * 
	 * @param Ability 要应用成本的技能
	 * @param Handle 技能规格句柄
	 * @param ActorInfo Actor信息
	 * @param ActivationInfo 技能激活信息
	 */
	virtual void ApplyCost(const UYcGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
						   const FGameplayAbilityActivationInfo ActivationInfo);

	/**
	 * 检查是否只在命中时应用成本
	 * @return 如果只在命中时应用成本则返回true，否则返回false
	 */
	bool ShouldOnlyApplyCostOnHit() const { return bOnlyApplyCostOnHit; }
	
	/**
	 * 蓝图可实现的成本检查函数
	 * 在蓝图中重写此函数以实现自定义的成本检查逻辑
	 * @param Ability 要检查成本的技能
	 * @param Handle 技能规格句柄
	 * @param ActorInfo Actor信息
	 * @return 如果可以支付成本则返回true，否则返回false
	 */
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "CheckCost")
	bool K2_CheckCost(const UYcGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo& ActorInfo);

	/**
	 * 蓝图可实现的成本应用函数
	 * 在蓝图中重写此函数以实现自定义的成本应用逻辑
	 * @param Ability 要应用成本的技能
	 * @param Handle 技能规格句柄
	 * @param ActorInfo Actor信息
	 * @param ActivationInfo 技能激活信息
	 */
	UFUNCTION(BlueprintImplementableEvent, DisplayName = "ApplyCost")
	void K2_ApplyCost(const UYcGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo& ActorInfo,
						   const FGameplayAbilityActivationInfo ActivationInfo);

protected:
	/** 如果为true，则此成本只在技能成功命中目标时应用（例如：只有命中时才消耗弹药） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Costs)
	bool bOnlyApplyCostOnHit = false;
};