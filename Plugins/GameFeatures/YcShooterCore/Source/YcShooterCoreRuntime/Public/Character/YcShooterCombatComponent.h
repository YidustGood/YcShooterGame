// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcAbilityComponent.h"
#include "YcShooterCombatComponent.generated.h"

class UYcCombatSet;

/**
 * 射击游戏战斗组件
 * 用于管理角色的战斗相关属性集（CombatSet），包括基础伤害、基础治疗等战斗属性
 * 继承自UYcAbilityComponent，自动处理ASC生命周期管理和属性集的创建/释放
 */
UCLASS(ClassGroup=(YiChenShooterCore), meta=(BlueprintSpawnableComponent))
class YCSHOOTERCORERUNTIME_API UYcShooterCombatComponent : public UYcAbilityComponent
{
	GENERATED_BODY()
public:
	UYcShooterCombatComponent(const FObjectInitializer& ObjectInitializer);
	
	/**
	 * 获取当前BaseDamage
	 * @return 当前BaseDamage值
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Combat")
	float GetBaseDamage() const;
	
protected:
	/**
	 * 与AbilitySystemComponent初始化时的业务逻辑
	 * 创建战斗属性集（CombatSet），用于存储基础伤害、基础治疗等战斗属性
	 * @param ASC 已验证的AbilitySystemComponent
	 */
	virtual void DoInitializeWithAbilitySystem(UYcAbilitySystemComponent* ASC) override;
	
	/**
	 * 与AbilitySystemComponent解除关联时的清理逻辑
	 * 清理战斗属性集引用
	 */
	virtual void DoUninitializeFromAbilitySystem() override;
	
	/** 
	 * 此Actor使用的战斗属性集，包含基础伤害、基础治疗等战斗相关属性
	 * 在DoInitializeWithAbilitySystem中创建并注册到ASC组件
	 * YcDamageExecution中对目标执行伤害计算的时候就会从攻击者的ASC中获取CombatSet属性集中的BaseDamage
	 * 这个BaseDamage可以设置为攻击者当前装备的武器的BaseDamage, 还可以通过应用BUFF GE来增幅BaseDamage实现动态增伤机制
	 */
	UPROPERTY(VisibleAnywhere, Category= "AttributeSet")
	TObjectPtr<const UYcCombatSet> CombatSet;
};