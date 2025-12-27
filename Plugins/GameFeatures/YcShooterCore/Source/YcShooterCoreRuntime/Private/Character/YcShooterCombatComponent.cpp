// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Character/YcShooterCombatComponent.h"
#include "AbilitySystem/Attributes/YcCombatSet.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcShooterCombatComponent)

UYcShooterCombatComponent::UYcShooterCombatComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 组件本身不需要Tick，仅通过属性变化和技能系统驱动逻辑
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
}

float UYcShooterCombatComponent::GetBaseDamage() const
{
	return (CombatSet ? CombatSet->GetBaseDamage() : 0.0f);
}

void UYcShooterCombatComponent::DoInitializeWithAbilitySystem(UYcAbilitySystemComponent* ASC)
{
	Super::DoInitializeWithAbilitySystem(ASC);
	
	// 创建战斗属性集，用于存储基础伤害、基础治疗等战斗属性
	// 这些属性会在伤害执行（YcDamageExecution）中被使用，用于计算最终伤害值
	CombatSet = AddAttributeSet<UYcCombatSet>();
}

void UYcShooterCombatComponent::DoUninitializeFromAbilitySystem()
{
	// 清理战斗属性集引用
	CombatSet = nullptr;
	
	Super::DoUninitializeFromAbilitySystem();
}