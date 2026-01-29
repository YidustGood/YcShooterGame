// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "AbilitySystem/Attributes/YcMovementAttributeSet.h"
#include "Net/UnrealNetwork.h"
#include "GameplayEffectExtension.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcMovementAttributeSet)

// @TODO 看后续优化为数据驱动从配置读取, 避免硬编码
UYcMovementAttributeSet::UYcMovementAttributeSet()
	: MaxWalkSpeed(500.0f)  // UE默认角色移动速度
	, MaxWalkSpeedMultiplier(1.0f)  // 默认倍率为1.0
{
}

void UYcMovementAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// 在属性实际改变前限制值的范围
	ClampAttribute(Attribute, NewValue);
}

void UYcMovementAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	// 属性改变后的处理逻辑
	// 这里可以添加额外的逻辑，比如广播事件等
}

void UYcMovementAttributeSet::OnRep_MaxWalkSpeed(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UYcMovementAttributeSet, MaxWalkSpeed, OldValue);
}

void UYcMovementAttributeSet::OnRep_MaxWalkSpeedMultiplier(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UYcMovementAttributeSet, MaxWalkSpeedMultiplier, OldValue);
}

void UYcMovementAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UYcMovementAttributeSet, MaxWalkSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UYcMovementAttributeSet, MaxWalkSpeedMultiplier, COND_None, REPNOTIFY_Always);
}

void UYcMovementAttributeSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	// @TODO 看后续优化为数据驱动从配置读取, 避免硬编码
	if (Attribute == GetMaxWalkSpeedAttribute())
	{
		// 限制基础移动速度在合理范围内 (50 - 2000)
		NewValue = FMath::Clamp(NewValue, 50.0f, 2000.0f);
	}
	else if (Attribute == GetMaxWalkSpeedMultiplierAttribute())
	{
		// 限制倍率在合理范围内 (0.1 - 5.0)
		// 0.1 = 最慢10%速度，5.0 = 最快500%速度
		NewValue = FMath::Clamp(NewValue, 0.1f, 5.0f);
	}
}
