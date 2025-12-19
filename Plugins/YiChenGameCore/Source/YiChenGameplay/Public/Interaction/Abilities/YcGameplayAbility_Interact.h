// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/YcGameplayAbility.h"
#include "YcGameplayAbility_Interact.generated.h"

struct FYcInteractionOption;
class UIndicatorDescriptor;

/**
 * 提供给角色交互能力的技能
 *
 * 玩家角色获取可交互对象通过两个射线碰撞检测的AbilityTask实现。
 * 1、UAbilityTask_GrantNearbyInteraction: 该Task基于球形重叠检测获取玩家角色周围可交互对象, 只在权威端执行
 * 会将检测到的所有可交互对象的YcInteractionOption中的InteractionAbilityToGrant技能赋予玩家的ASC组件,
 * 不同可交互对象通过配置不同的技能实现不同的交互逻辑。
 * 2、UAbilityTask_WaitForInteractableTargets_SingleLineTrace:
 * 该Task基于射线检测获取玩家视野聚焦的当前可交互对象, 只在本地控制端执行,
 * 前者是负责处理周围可交互物的GA授予, 后者是负责检测玩家视野聚焦的可交互物目标和触发交互物的技能。
 * 当视野聚焦的可交互物发生变化时会触发委托, 在委托回调中就可以处理当前可交互物, 例如触发物品刚刚授予的GA, 显示UI信息等。
 */
UCLASS()
class YICHENGAMEPLAY_API UYcGameplayAbility_Interact : public UYcGameplayAbility
{
	GENERATED_BODY()
public:
	UYcGameplayAbility_Interact(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	
	/**
	 * 更新当前可交互对象列表并缓存，用于后续交互逻辑。
	 *
	 * @param InteractiveOptions	由交互检测任务返回的交互选项数组。
	 */
	UFUNCTION(BlueprintCallable)
	void UpdateInteractions(const TArray<FYcInteractionOption>& InteractiveOptions);

	/**
	 * 尝试触发当前聚焦交互对象的交互技能。
	 */
	UFUNCTION(BlueprintCallable)
	void TriggerInteraction();
	
protected:
	/** 启动持续扫描更新玩家视线中的可交互对象 */ 
	void LookForInteractables();

	/** 监听交互输入(单次, 触发后失效, 触发后再调用监听可实现持续监听交互按键输入) */ 
	void ListenInteractPress();
	
	/** 输入按下回调 */
	UFUNCTION()
	void OnInputPressed(float TimeWaited);
	
	/** 输入释放回调 */
	UFUNCTION()
	void OnInputReleased(float TimeHeld);

protected:
	/** 当前已缓存的交互选项集合 */
	UPROPERTY(BlueprintReadWrite)
	TArray<FYcInteractionOption> CurrentOptions;

	/** 扫描玩家周围可交互对象的频率, 适当降低可以节省性能 */
	UPROPERTY(EditDefaultsOnly)
	float NearbyInteractionScanRate = 0.1f;

	/** 扫描的范围大小 */
	UPROPERTY(EditDefaultsOnly)
	float NearbyInteractionScanRange = 500;
	
	/** 扫描玩家视野焦点上可交互对象的频率, 适当降低可以节省性能 */
	UPROPERTY(EditDefaultsOnly)
	float ViewedInteractionScanRate = 0.1f;

	/** 扫描玩家视野焦点的长度 */
	UPROPERTY(EditDefaultsOnly)
	float ViewedInteractionScanRange = 200;
	
	/** 扫描玩家视野焦点时所使用的碰撞配置 */
	UPROPERTY(EditDefaultsOnly)
	FCollisionProfileName TraceProfile;
	
	/** 当交互对象Option中未指定 `InteractionWidgetClass` 时使用的默认 UI 类 */
	UPROPERTY(EditDefaultsOnly)
	TSoftClassPtr<UUserWidget> DefaultInteractionWidgetClass;
};