// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once
#include "AbilityTask_WaitForInteractableTargets.h"
#include "Interaction/YcInteractionTypes.h"
#include "AbilityTask_WaitForInteractableTargets_SingleLineTrace.generated.h"

struct FCollisionProfileName;
class UGameplayAbility;
class UObject;


/**
 * 扫描玩家视野焦点上首个可交互对象的AbilityTask
 * 运行这个Task会以设定的间隔事件持续循环扫描视野焦点上一定距离的可交互物体, 如果检测结果有变化会进行广播通知
 */
UCLASS()
class YICHENGAMEPLAY_API
	UAbilityTask_WaitForInteractableTargets_SingleLineTrace : public UAbilityTask_WaitForInteractableTargets
{
	GENERATED_BODY()
public:
	/**
	 * 在指定起点与碰撞配置下，持续以固定速率发射单线射线，检测视线内可交互对象
	 * 当检测到的对象集合发生变化时，通过父类的 `OnInteractableTargetsChanged` 事件广播
	 *
	 * @param OwningAbility	拥有此任务的 `UGameplayAbility` 实例
	 * @param InteractionQuery	查询者信息（通常为玩家及其控制器）
	 * @param TraceProfile	用于射线检测的碰撞配置名称。
	 * @param StartLocation	射线起点描述
	 * @param InteractionScanRange	射线长度
	 * @param InteractionScanRate	检测间隔（秒）
	 * @return	返回创建好的任务实例
	 */
	UFUNCTION(BlueprintCallable, Category="Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UAbilityTask_WaitForInteractableTargets_SingleLineTrace* WaitForInteractableTargets_SingleLineTrace(UGameplayAbility* OwningAbility,
																											   FYcInteractionQuery InteractionQuery,
																											   FCollisionProfileName TraceProfile,
																											   FGameplayAbilityTargetingLocationInfo StartLocation,
																											   float InteractionScanRange = 100,
																											   float InteractionScanRate = 0.100);
	
	virtual void Activate() override;
	
private:
	virtual void OnDestroy(bool AbilityEnded) override;

	// 真正进行交互物射线检测的函数
	void PerformTrace();

	// 射线查询发起者的信息
	UPROPERTY()
	FYcInteractionQuery InteractionQuery;

	UPROPERTY()
	FGameplayAbilityTargetingLocationInfo StartLocation;

	// 射线检测的长度, 需要控制好长度范围, 避免检测过远的对象
	float InteractionScanLength = 100;
	// 射线检测的频率, 适当降低可以节省性能
	float InteractionScanRate = 0.100;

	FTimerHandle TimerHandle;
};
