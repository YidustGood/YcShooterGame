// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Subsystem/YcDamageInfluenceSubsystem.h"

#include "AbilitySystemComponent.h"
#include "Library/YcDamageBlueprintLibrary.h"
#include "Engine/DataTable.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageInfluenceSubsystem)

void UYcDamageInfluenceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UYcDamageInfluenceSubsystem::Deinitialize()
{
	CachedInfluenceMap.Empty();
	bCacheInitialized = false;
	
	Super::Deinitialize();
}

void UYcDamageInfluenceSubsystem::SetInfluenceTable(UDataTable* InInfluenceTable)
{
	InfluenceTable = InInfluenceTable;
	bCacheInitialized = false;
	InitializeCache();
}

void UYcDamageInfluenceSubsystem::InitializeCache()
{
	if (!InfluenceTable || bCacheInitialized)
	{
		return;
	}

	CachedInfluenceMap.Empty();

	// 遍历数据表，构建缓存
	TArray<FYcDamageInfluenceRow*> AllRows;
	InfluenceTable->GetAllRows(TEXT("YcDamageInfluenceSubsystem"), AllRows);

	for (const FYcDamageInfluenceRow* Row : AllRows)
	{
		if (Row && Row->DamageTypeTag.IsValid())
		{
			CachedInfluenceMap.FindOrAdd(Row->DamageTypeTag).Add(*Row);
		}
	}

	bCacheInitialized = true;
}

TArray<FYcDamageInfluenceRow> UYcDamageInfluenceSubsystem::GetInfluencesForDamageType(const FGameplayTag& DamageTypeTag) const
{
	if (!bCacheInitialized)
	{
		const_cast<UYcDamageInfluenceSubsystem*>(this)->InitializeCache();
	}

	if (const TArray<FYcDamageInfluenceRow>* Found = CachedInfluenceMap.Find(DamageTypeTag))
	{
		return *Found;
	}

	return TArray<FYcDamageInfluenceRow>();
}

void UYcDamageInfluenceSubsystem::ApplyInfluences(FYcDamageSummaryParams& Params, const FGameplayTag& DamageTypeTag) const
{
	if (!DamageTypeTag.IsValid())
	{
		return;
	}

	// 获取加成配置
	TArray<FYcDamageInfluenceRow> Influences = GetInfluencesForDamageType(DamageTypeTag);
	if (Influences.IsEmpty())
	{
		return;
	}

	// 遍历应用每个加成
	for (const FYcDamageInfluenceRow& Influence : Influences)
	{
		// 确定属性来源
		UAbilitySystemComponent* SourceASC = nullptr;
		switch (Influence.Source)
		{
		case EYcDamageInfluenceSource::FromSource:
			SourceASC = Params.SourceASC;
			break;
		case EYcDamageInfluenceSource::FromTarget:
			SourceASC = Params.TargetASC;
			break;
		default:
			continue;
		}

		if (!SourceASC)
		{
			continue;
		}

		// 获取属性值
		bool bFound = false;
		float AttributeValue = GetAttributeValue(SourceASC, Influence.Attribute, bFound);
		if (!bFound)
		{
			continue;
		}

		// 计算加成值
		float InfluenceValue = Influence.CalculateValue(AttributeValue);

		// 应用到对应的乘区
		switch (Influence.Priority)
		{
		case EYcAttributeInfluencePriority::Override:
			UYcDamageBlueprintLibrary::SetOverride(Params, InfluenceValue);
			break;
		case EYcAttributeInfluencePriority::PreMultiplyAdditive:
			UYcDamageBlueprintLibrary::AddPreMultiplyAdditive(Params, InfluenceValue);
			break;
		case EYcAttributeInfluencePriority::Coefficient:
			UYcDamageBlueprintLibrary::MultiplyCoefficient(Params, InfluenceValue);
			break;
		case EYcAttributeInfluencePriority::PostMultiplyAdditive:
			UYcDamageBlueprintLibrary::AddPostMultiplyAdditive(Params, InfluenceValue);
			break;
		}
	}
}

float UYcDamageInfluenceSubsystem::GetAttributeValue(UAbilitySystemComponent* ASC, const FGameplayAttribute& Attribute, bool& bFound) const
{
	bFound = false;
	if (!ASC || !Attribute.IsValid())
	{
		return 0.0f;
	}

	return ASC->GetGameplayAttributeValue(Attribute, bFound);
}
