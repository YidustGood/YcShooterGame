// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Abilities/YcAbilityCost.h"
#include "YcAbilityCost_ItemTagStack.generated.h"

/**
 * 物品标签堆栈成本
 * 表示需要消耗关联物品实例上指定标签堆栈数量的成本
 * 仅适用于从装备物品授予的技能（UYcGameplayAbility_FromEquipment）
 * 常用于消耗物品的耐久度、充能次数等标签堆栈
 */
UCLASS(meta=(DisplayName="Item Tag Stack"))
class YICHENEQUIPMENT_API UYcAbilityCost_ItemTagStack : public UYcAbilityCost
{
	GENERATED_BODY()
public:
	UYcAbilityCost_ItemTagStack();

	//~UYcAbilityCost interface
	
	/**
	 * 检查是否可以支付此成本（覆盖父类）
	 * 检查关联物品实例上指定标签的堆栈数量是否足够
	 * 如果成本不足，会添加FailureTag到OptionalRelevantTags中
	 * @param Ability 要检查成本的技能（必须是UYcGameplayAbility_FromEquipment类型）
	 * @param Handle 技能规格句柄
	 * @param ActorInfo Actor信息
	 * @param OptionalRelevantTags 可选的输出相关标签容器，可用于添加失败原因标签
	 * @return 如果可以支付成本则返回true，否则返回false
	 */
	virtual bool CheckCost(const UYcGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) override;
	/**
	 * 应用技能的成本（覆盖父类）
	 * 从关联物品实例上移除指定数量的标签堆栈
	 * 仅在服务器端执行
	 * @param Ability 要应用成本的技能（必须是UYcGameplayAbility_FromEquipment类型）
	 * @param Handle 技能规格句柄
	 * @param ActorInfo Actor信息
	 * @param ActivationInfo 技能激活信息
	 */
	virtual void ApplyCost(const UYcGameplayAbility* Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) override;
	//~End of UYcAbilityCost interface

protected:
	/** 需要消耗的标签堆栈数量（根据技能等级缩放） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Costs)
	FScalableFloat Quantity;

	/** 要消耗的标签 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Costs)
	FGameplayTag Tag;

	/** 如果此成本无法应用时，作为响应发送的标签（用于提供用户反馈） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Costs)
	FGameplayTag FailureTag;
};