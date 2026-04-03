// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Armor/YcArmorLibrary.h"

#include "AbilitySystemInterface.h"
#include "Armor/YcArmorSet.h"
#include "YcAbilitySystemComponent.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcArmorLibrary)

UAbilitySystemComponent* UYcArmorLibrary::GetASCFromActor(AActor* Target)
{
	if (!Target)
	{
		return nullptr;
	}

	// 通过 GAS 接口获取 ASC
	const IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Target);
	if (ASI == nullptr)
	{
		return nullptr;
	}

	return ASI->GetAbilitySystemComponent();
}

const UYcArmorSet* UYcArmorLibrary::GetArmorSetByTag(AActor* Target, FGameplayTag ArmorTag)
{
	if (UAbilitySystemComponent* ASC = GetASCFromActor(Target))
	{
		if (UYcAbilitySystemComponent* YcASC = Cast<UYcAbilitySystemComponent>(ASC))
		{
			return Cast<const UYcArmorSet>(YcASC->GetAttributeSetByTag(ArmorTag));
		}
	}
	return nullptr;
}

TArray<UYcArmorSet*> UYcArmorLibrary::GetAllArmorSets(AActor* Target)
{
	TArray<UYcArmorSet*> Result;

	if (UAbilitySystemComponent* ASC = GetASCFromActor(Target))
	{
		// 遍历所有 SpawnedAttributes，找出所有 UYcArmorSet
		for (UAttributeSet* Set : ASC->GetSpawnedAttributes())
		{
			if (UYcArmorSet* ArmorSet = Cast<UYcArmorSet>(Set))
			{
				Result.Add(ArmorSet);
			}
		}
	}

	return Result;
}

float UYcArmorLibrary::GetArmorByTag(AActor* Target, FGameplayTag ArmorTag)
{
	if (const UYcArmorSet* ArmorSet = GetArmorSetByTag(Target, ArmorTag))
	{
		return ArmorSet->GetArmor();
	}
	return 0.0f;
}

float UYcArmorLibrary::GetMaxArmorByTag(AActor* Target, FGameplayTag ArmorTag)
{
	if (const UYcArmorSet* ArmorSet = GetArmorSetByTag(Target, ArmorTag))
	{
		return ArmorSet->GetMaxArmor();
	}
	return 0.0f;
}

float UYcArmorLibrary::GetArmorNormalizedByTag(AActor* Target, FGameplayTag ArmorTag)
{
	if (const UYcArmorSet* ArmorSet = GetArmorSetByTag(Target, ArmorTag))
	{
		const float MaxArmor = ArmorSet->GetMaxArmor();
		if (MaxArmor > 0.0f)
		{
			return ArmorSet->GetArmor() / MaxArmor;
		}
	}
	return 0.0f;
}

float UYcArmorLibrary::GetArmorAbsorptionByTag(AActor* Target, FGameplayTag ArmorTag)
{
	if (const UYcArmorSet* ArmorSet = GetArmorSetByTag(Target, ArmorTag))
	{
		return ArmorSet->GetArmorAbsorption();
	}
	return 0.0f;
}

bool UYcArmorLibrary::IsArmorBrokenByTag(AActor* Target, FGameplayTag ArmorTag)
{
	if (const UYcArmorSet* ArmorSet = GetArmorSetByTag(Target, ArmorTag))
	{
		return ArmorSet->GetArmor() <= 0.0f;
	}
	return true;
}

// ========== 兼容旧 API ==========

float UYcArmorLibrary::GetArmor(AActor* Target)
{
	// 获取第一个护甲属性集
	TArray<UYcArmorSet*> AllArmorSets = GetAllArmorSets(Target);
	if (!AllArmorSets.IsEmpty())
	{
		return AllArmorSets[0]->GetArmor();
	}
	return 0.0f;
}

float UYcArmorLibrary::GetMaxArmor(AActor* Target)
{
	TArray<UYcArmorSet*> AllArmorSets = GetAllArmorSets(Target);
	if (!AllArmorSets.IsEmpty())
	{
		return AllArmorSets[0]->GetMaxArmor();
	}
	return 0.0f;
}

float UYcArmorLibrary::GetArmorNormalized(AActor* Target)
{
	TArray<UYcArmorSet*> AllArmorSets = GetAllArmorSets(Target);
	if (!AllArmorSets.IsEmpty())
	{
		const float MaxArmor = AllArmorSets[0]->GetMaxArmor();
		if (MaxArmor > 0.0f)
		{
			return AllArmorSets[0]->GetArmor() / MaxArmor;
		}
	}
	return 0.0f;
}
