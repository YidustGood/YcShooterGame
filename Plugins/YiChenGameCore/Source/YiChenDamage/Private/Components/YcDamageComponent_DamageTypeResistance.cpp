// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Components/YcDamageComponent_DamageTypeResistance.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet.h"
#include "Settings/YcDamageTypeSettings.h"
#include "Resistance/YcResistanceComponent.h"
#include "Library/YcDamageBlueprintLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageComponent_DamageTypeResistance)

UYcDamageComponent_DamageTypeResistance::UYcDamageComponent_DamageTypeResistance()
	: MaxResistance(0.8f)
	, DefaultResistance(0.0f)
{
	Priority = 30; // 在基础伤害计算之后执行
	DebugName = TEXT("DamageTypeResistance");
}

void UYcDamageComponent_DamageTypeResistance::Execute_Implementation(FYcAttributeSummaryParams& Params)
{
	// 转换为伤害参数
	FYcDamageSummaryParams& DamageParams = static_cast<FYcDamageSummaryParams&>(Params);
	
	// 检查伤害类型是否有效
	if (!DamageParams.DamageTypeTag.IsValid())
	{
		if (bEnableDebugLog)
		{
			LogDebug(TEXT("No damage type tag, skipping resistance calculation"));
		}
		return;
	}

	float Resistance = DefaultResistance;
	FGameplayTag ResistanceAttributeTag;

	// 从配置资产获取伤害类型定义
	if (DamageTypeSettings)
	{
		if (const FYcDamageTypeDefinition* Def = DamageTypeSettings->FindDamageType(DamageParams.DamageTypeTag))
		{
			ResistanceAttributeTag = Def->ResistanceAttributeTag;
		}
	}

	// 获取抗性值（优先使用高效方案）
	if (ResistanceAttributeTag.IsValid())
	{
		Resistance = GetTargetResistance(DamageParams, ResistanceAttributeTag);
	}

	// 限制抗性上限
	Resistance = FMath::Clamp(Resistance, 0.0f, MaxResistance);

	// 计算抗性系数（抗性 0.3 = 减少 30% 伤害 = 系数 0.7）
	const float ResistanceCoefficient = 1.0f - Resistance;

	// 应用抗性
	if (ResistanceCoefficient < 1.0f)
	{
		UYcDamageBlueprintLibrary::MultiplyCoefficient(DamageParams, ResistanceCoefficient);

		if (bEnableDebugLog)
		{
			LogDebug(FString::Printf(
				TEXT("DamageType: %s, Resistance: %.2f, Coefficient: %.2f"),
				*DamageParams.DamageTypeTag.ToString(),
				Resistance,
				ResistanceCoefficient
			));
		}
	}
}

float UYcDamageComponent_DamageTypeResistance::GetTargetResistance(const FYcDamageSummaryParams& Params, const FGameplayTag& ResistanceTag) const
{
	if (!Params.TargetASC || !ResistanceTag.IsValid())
	{
		return 0.0f;
	}

	// 优先方案：从 UYcResistanceComponent 获取（O(1) 查询）
	AActor* TargetActor = Params.TargetASC->GetOwnerActor();
	if (TargetActor)
	{
		// 尝试从 OwnerActor 获取
		if (UYcResistanceComponent* ResistanceComp = TargetActor->FindComponentByClass<UYcResistanceComponent>())
		{
			const float Resistance = ResistanceComp->GetResistance(ResistanceTag);
			if (Resistance > 0.0f)
			{
				return Resistance;
			}
		}

		// 尝试从 AvatarActor 获取（ASC 可能在 PlayerState 上）
		AActor* AvatarActor = Params.TargetASC->GetAvatarActor();
		if (AvatarActor && AvatarActor != TargetActor)
		{
			if (UYcResistanceComponent* ResistanceComp = AvatarActor->FindComponentByClass<UYcResistanceComponent>())
			{
				const float Resistance = ResistanceComp->GetResistance(ResistanceTag);
				if (Resistance > 0.0f)
				{
					return Resistance;
				}
			}
		}
	}

	// 回退方案：从 AttributeSet 获取（O(N*M) 遍历）
	return GetTargetAttributeValueByTag(Params, ResistanceTag);
}

float UYcDamageComponent_DamageTypeResistance::GetTargetAttributeValueByTag(const FYcDamageSummaryParams& Params, const FGameplayTag& AttributeTag) const
{
	if (!Params.TargetASC || !AttributeTag.IsValid())
	{
		return 0.0f;
	}

	// 通过 GameplayAttribute 获取属性值
	// 遍历目标的所有属性集查找匹配标签的属性
	auto AttributeSets = Params.TargetASC->GetSpawnedAttributes();

	for (const UAttributeSet* AttributeSet : AttributeSets)
	{
		if (!AttributeSet)
		{
			continue;
		}

		// 通过反射查找带有匹配标签的属性
		for (TFieldIterator<FProperty> PropIt(AttributeSet->GetClass()); PropIt; ++PropIt)
		{
			if (FStructProperty* StructProp = CastField<FStructProperty>(*PropIt))
			{
				// 检查属性是否有 GameplayTag 元数据
				// 属性名称格式: Resistance.Fire -> 查找 FireResistance
				FString TagName = AttributeTag.GetTagName().ToString();
				FString PropertyName = StructProp->GetName();
				
				// 尝试多种匹配方式
				bool bMatches = false;
				if (PropertyName.Equals(TagName, ESearchCase::IgnoreCase))
				{
					bMatches = true;
				}
				else if (TagName.Contains(".") && PropertyName.Equals(TagName.RightChop(TagName.Find(".") + 1), ESearchCase::IgnoreCase))
				{
					// Resistance.Fire -> Fire
					bMatches = true;
				}
				else if (FString::Printf(TEXT("%sResistance"), *TagName.RightChop(TagName.Find(".") + 1)).Equals(PropertyName, ESearchCase::IgnoreCase))
				{
					// Fire -> FireResistance
					bMatches = true;
				}

				if (bMatches)
				{
					if (auto AttrData = StructProp->ContainerPtrToValuePtr<FGameplayAttributeData>(AttributeSet))
					{
						return AttrData->GetCurrentValue();
					}
				}
			}
		}
	}

	return 0.0f;
}
