// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_GrantNearbyInteraction.generated.h"

/**
 * 从Task发起者的位置进行指定范围大小的重叠碰撞测试，然后将重叠范围内的可交互物体的Option中的InteractionAbilityToGrant技能授予玩家ASC组件，
 * 并将FGameplayAbilitySpec(GA实例)设置到这个交互物体的Option中(TargetInteractionAbilityHandle),同时缓存到Task的InteractionAbilityCache中
 * 然后玩家交互查询射线查询到交互物体后，会检查玩家的ASC组件是否拥有交互物体的InteractionAbilityToGrant的技能实例，如果有，
 * 那么就会将这个交互物通过IndicatorDescriptor进行包装，并添加到IndicatorManagerComponent中，有IMC组件对象进行管理
 * 玩家则可以通过IMC中的ID访问交互对象，显示UI提示信息等操作
 * 然后可以实现按键触发这个交互物授予玩家的GA，例如交互物是开门 提供开门的GA，交互物是可收集到库存的，那么提供库存收集GA，这个GA实现物品添加到库存的逻辑
 */
UCLASS()
class YICHENGAMEPLAY_API UAbilityTask_GrantNearbyInteraction : public UAbilityTask
{
	GENERATED_BODY()
public:
	UAbilityTask_GrantNearbyInteraction(const FObjectInitializer& ObjectInitializer);
	
	/**
	 * 在指定范围内持续扫描可交互对象，并为玩家授予其 `InteractionAbilityToGrant` 所对应的技能实例
	 *
	 * @param OwningAbility	拥有此任务的 `UGameplayAbility` 实例
	 * @param InteractionScanRange	扫描半径
	 * @param InteractionScanRate	扫描间隔（秒）
	 * @return	返回创建好的任务实例
	 */
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_GrantNearbyInteraction* GrantAbilitiesForNearbyInteractors(UGameplayAbility* OwningAbility, float InteractionScanRange, float InteractionScanRate);
	
	virtual void Activate() override;

private:
	virtual void OnDestroy(bool AbilityEnded) override;

	// 将附加的所有交互对象的GA都应用到 该Task的调用GA的ASC组件上
	void QueryInteractables();
	
	// 扫描范围
	float InteractionScanRange = 100;
	// 扫描间隔
	float InteractionScanRate = 0.100;

	FTimerHandle QueryTimerHandle;

	TMap<FObjectKey, FGameplayAbilitySpecHandle> InteractionAbilityCache;
};
