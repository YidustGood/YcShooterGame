// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Armor/YcDamageComponent_Armor.h"

#include "Armor/YcArmorSet.h"
#include "YcAbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "YcDamageGameplayTags.h"
#include "Library/YcDamageBlueprintLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageComponent_Armor)

UYcDamageComponent_Armor::UYcDamageComponent_Armor()
{
	Priority = 100;
	DebugName = TEXT("ArmorMitigator");
}

void UYcDamageComponent_Armor::Execute_Implementation(FYcAttributeSummaryParams& Params)
{
	// 转换为伤害参数
	FYcDamageSummaryParams& DamageParams = static_cast<FYcDamageSummaryParams&>(Params);
	
	if (!bEnableArmorMitigation)
	{
		return;
	}
	
	// 检查伤害类型是否忽略护甲
	if (!IgnoreArmorDamageTypes.IsEmpty() && DamageParams.DamageTypeTag.IsValid())
	{
		if (IgnoreArmorDamageTypes.HasTag(DamageParams.DamageTypeTag))
		{
			LogDebug(TEXT("Ignored: Damage type bypasses armor"));
			return;
		}
	}

	// 获取 YcAbilitySystemComponent
	UYcAbilitySystemComponent* YcASC = Cast<UYcAbilitySystemComponent>(Params.TargetASC);
	if (!YcASC)
	{
		return;
	}

	// 获取目标护甲属性集（基于 HitZone 或默认）
	UYcArmorSet* TargetArmorSet = GetTargetArmorSet(YcASC, DamageParams.HitZone);
	if (!TargetArmorSet)
	{
		return;
	}

	// 执行护甲吸收
	ExecuteArmorAbsorption(DamageParams, TargetArmorSet);
}

void UYcDamageComponent_Armor::ExecuteArmorAbsorption(FYcDamageSummaryParams& Params, const UYcArmorSet* ArmorSet) const
{
	if (!ArmorSet)
	{
		return;
	}

	const float CurrentArmor = ArmorSet->GetArmor();
	if (CurrentArmor <= 0.0f)
	{
		return; // 护甲已耗尽，不提供减伤
	}

	// 计算当前伤害值
	float DamageToMitigate = Params.GetFinalDamage();
	if (DamageToMitigate <= 0.0f)
	{
		return;
	}

	// 获取护甲吸收比例
	const float AbsorptionRatio = ArmorSet->GetArmorAbsorption();

	// 计算护甲应该吸收的伤害量
	const float AbsorbedDamage = DamageToMitigate * AbsorptionRatio;

	// 实际消耗的护甲耐久度（不超过当前护甲值）
	const float ArmorToConsume = FMath::Min(AbsorbedDamage, CurrentArmor);

	// 实际吸收的伤害等于消耗的护甲耐久度
	const float ActualAbsorbedDamage = ArmorToConsume;

	// 将吸收的伤害添加到乘后加成（负值，减少伤害），因为伤害可能存在buff影响，所以放在乘后区域做最终伤害吸收
	UYcDamageBlueprintLibrary::AddPostMultiplyAdditive(Params, -ActualAbsorbedDamage);

	// 消耗护甲耐久度
	ApplyModToAttribute(Params, ArmorSet->GetArmorAttribute(), -ArmorToConsume);

	// 更新伤害信息
	Params.DamageInfo.PracticalDamage = Params.GetFinalDamage();

	// 检查护甲是否耗尽
	const float RemainingArmor = CurrentArmor - ArmorToConsume;
	if (RemainingArmor <= 0.0f)
	{
		Params.bClamped = true;
		UYcDamageBlueprintLibrary::AddTemporaryTag(Params, YcDamageGameplayTags::Damage_Event_ArmorBroken);

		// 发送护甲破碎事件
		if (bSendArmorBrokenEvent && ArmorBrokenEventTag.IsValid())
		{
			if (AActor* TargetActor = Params.TargetASC->GetAvatarActor_Direct())
			{
				FGameplayEventData Payload;
				Payload.EventTag = ArmorBrokenEventTag;
				Payload.Target = TargetActor;
				Payload.Instigator = Params.SourceASC ? Params.SourceASC->GetAvatarActor_Direct() : nullptr;
				UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, ArmorBrokenEventTag, Payload);
			}
		}

		LogDebug(FString::Printf(TEXT("Armor broken! Tag: %s"), *ArmorSet->AttributeSetTag.ToString()));
	}

	LogDebug(FString::Printf(TEXT("Armor absorbed: %.2f, Remaining: %.2f"), ActualAbsorbedDamage, RemainingArmor));
}

FGameplayTag UYcDamageComponent_Armor::ConvertHitZoneToArmorTag(const FGameplayTag& HitZone) const
{
	// @TODO 看是否需要做护甲Tag和命中区域Tag的转换，直接用命中区域tag分配给护甲其实也是可行的
	// 将 HitZone 映射到 ArmorTag
	// 例如: HitZone.Head -> Armor.Head, HitZone.Body -> Armor.Body
	if (!HitZone.IsValid())
	{
		return FGameplayTag();
	}

	// 简单的命名转换：将 HitZone 替换为 Armor
	FString HitZoneString = HitZone.ToString();
	FString ArmorTagString = HitZoneString.Replace(TEXT("HitZone."), TEXT("Armor."));
	
	return FGameplayTag::RequestGameplayTag(*ArmorTagString);
}

UYcArmorSet* UYcDamageComponent_Armor::GetTargetArmorSet(UYcAbilitySystemComponent* YcASC, const FGameplayTag& HitZone) const
{
	if (!YcASC)
	{
		return nullptr;
	}
	
	// 1. 直接用HitZoneTag查询属性集
	if (UYcArmorSet* ArmorSet = Cast<UYcArmorSet>(YcASC->GetAttributeSetByTag(HitZone)))
	{
		return ArmorSet;
	}

	// 2. 尝试将HitZone转换为 ArmorTag 查找对应的护甲
	if (HitZone.IsValid())
	{
		FGameplayTag ArmorTag = ConvertHitZoneToArmorTag(HitZone);
		if (ArmorTag.IsValid())
		{
			if (UYcArmorSet* ArmorSet = Cast<UYcArmorSet>(YcASC->GetAttributeSetByTag(ArmorTag)))
			{
				return ArmorSet;
			}
		}
	}

	return nullptr;
}
