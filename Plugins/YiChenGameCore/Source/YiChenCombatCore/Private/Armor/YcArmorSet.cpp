// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Armor/YcArmorSet.h"

#include "Armor/YcArmorMessage.h"
#include "GameFramework/Actor.h"
#include "YcAbilitySystemComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcArmorSet)

UE_DEFINE_GAMEPLAY_TAG(TAG_Gameplay_ArmorBroken, "Gameplay.ArmorBroken");

UYcArmorSet::UYcArmorSet()
	: Armor(0.0f)
	, MaxArmor(100.0f)
	, ArmorAbsorption(0.3f)
	, bOutOfArmor(true)
	, MaxArmorBeforeAttributeChange(0.0f)
	, ArmorBeforeAttributeChange(0.0f)
{
}

void UYcArmorSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UYcArmorSet, Armor, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UYcArmorSet, MaxArmor, COND_None, REPNOTIFY_Always);
}

void UYcArmorSet::OnRep_Armor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UYcArmorSet, Armor, OldValue);

	const float CurrentArmor = GetArmor();
	const float EstimatedMagnitude = CurrentArmor - OldValue.GetCurrentValue();
	
	OnArmorChanged.Broadcast(nullptr, nullptr, nullptr, EstimatedMagnitude, OldValue.GetCurrentValue(), CurrentArmor);

	// 如果之前有护甲但现在归零了，触发护甲破碎事件
	if (!bOutOfArmor && CurrentArmor <= 0.0f)
	{
		OnArmorBroken.Broadcast(nullptr, nullptr, nullptr, EstimatedMagnitude, OldValue.GetCurrentValue(), CurrentArmor);
	}

	bOutOfArmor = (CurrentArmor <= 0.0f);
}

void UYcArmorSet::OnRep_MaxArmor(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UYcArmorSet, MaxArmor, OldValue);

	OnMaxArmorChanged.Broadcast(nullptr, nullptr, nullptr, GetMaxArmor() - OldValue.GetCurrentValue(), OldValue.GetCurrentValue(), GetMaxArmor());
}

void UYcArmorSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UYcArmorSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	ClampAttribute(Attribute, NewValue);
}

void UYcArmorSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	if (Attribute == GetMaxArmorAttribute())
	{
		// 确保当前护甲值不超过新的最大护甲值
		if (GetArmor() > NewValue)
		{
			UYcAbilitySystemComponent* ASC = GetYcAbilitySystemComponent();
			check(ASC);

			ASC->ApplyModToAttribute(GetArmorAttribute(), EGameplayModOp::Override, NewValue);
		}
	}

	// 护甲值变化时广播委托（服务端直接修改时 OnRep 不会触发）
	if (Attribute == GetArmorAttribute())
	{
		const float Magnitude = NewValue - OldValue;
		OnArmorChanged.Broadcast(nullptr, nullptr, nullptr, Magnitude, OldValue, NewValue);

		// 如果之前有护甲但现在归零了，触发护甲破碎事件
		if (!bOutOfArmor && NewValue <= 0.0f)
		{
			OnArmorBroken.Broadcast(nullptr, nullptr, nullptr, Magnitude, OldValue, NewValue);
		}

		bOutOfArmor = (NewValue <= 0.0f);
	}

	// 如果之前归零但现在护甲值大于0，更新状态
	if (bOutOfArmor && (GetArmor() > 0.0f))
	{
		bOutOfArmor = false;
	}
}

void UYcArmorSet::ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const
{
	if (Attribute == GetArmorAttribute())
	{
		// 不允许护甲值为负数或超过最大护甲值
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxArmor());
	}
	else if (Attribute == GetMaxArmorAttribute())
	{
		// 不允许最大护甲值低于0
		NewValue = FMath::Max(NewValue, 0.0f);
	}
	else if (Attribute == GetArmorAbsorptionAttribute())
	{
		// 吸收比例限制在0.0~1.0之间
		NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
	}
}
