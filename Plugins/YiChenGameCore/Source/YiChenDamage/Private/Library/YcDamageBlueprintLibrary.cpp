// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Library/YcDamageBlueprintLibrary.h"

#include "AbilitySystemComponent.h"
#include "Executions/YcDamageSummaryParams.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageBlueprintLibrary)

float UYcDamageBlueprintLibrary::GetSourceAttribute(const UObject* WorldContextObject, UAbilitySystemComponent* SourceASC, const FGameplayAttribute& Attribute, bool& bFound)
{
	bFound = false;
	if (!SourceASC)
	{
		return 0.0f;
	}
	return SourceASC->GetGameplayAttributeValue(Attribute, bFound);
}

float UYcDamageBlueprintLibrary::GetTargetAttribute(const UObject* WorldContextObject, UAbilitySystemComponent* TargetASC, const FGameplayAttribute& Attribute, bool& bFound)
{
	bFound = false;
	if (!TargetASC)
	{
		return 0.0f;
	}
	return TargetASC->GetGameplayAttributeValue(Attribute, bFound);
}

void UYcDamageBlueprintLibrary::SetOverride(FYcAttributeSummaryParams& Params, float Value)
{
	Params.bOverride = true;
	Params.OverrideValue = Value;
	Params.AddInfluence(EYcAttributeInfluencePriority::Override, Value);
}

void UYcDamageBlueprintLibrary::AddPreMultiplyAdditive(FYcAttributeSummaryParams& Params, float Value)
{
	Params.PreMultiplyAdditive += Value;
	Params.AddInfluence(EYcAttributeInfluencePriority::PreMultiplyAdditive, Value);
}

void UYcDamageBlueprintLibrary::MultiplyCoefficient(FYcAttributeSummaryParams& Params, float Value)
{
	Params.Coefficient *= Value;
	Params.AddInfluence(EYcAttributeInfluencePriority::Coefficient, Value);
}

void UYcDamageBlueprintLibrary::AddPostMultiplyAdditive(FYcAttributeSummaryParams& Params, float Value)
{
	Params.PostMultiplyAdditive += Value;
	Params.AddInfluence(EYcAttributeInfluencePriority::PostMultiplyAdditive, Value);
}

float UYcDamageBlueprintLibrary::GetFinalValue(const FYcAttributeSummaryParams& Params)
{
	return Params.GetFinalValue();
}

float UYcDamageBlueprintLibrary::GetFinalDamage(const FYcDamageSummaryParams& Params)
{
	return Params.GetFinalDamage();
}

float UYcDamageBlueprintLibrary::GetEstimatedDamage(const FYcDamageSummaryParams& Params)
{
	return Params.DamageInfo.EstimatedDamage;
}

float UYcDamageBlueprintLibrary::GetPracticalDamage(const FYcDamageSummaryParams& Params)
{
	return Params.DamageInfo.PracticalDamage;
}

void UYcDamageBlueprintLibrary::CancelExecution(FYcAttributeSummaryParams& Params)
{
	Params.bCancelExecution = true;
}

void UYcDamageBlueprintLibrary::AddTemporaryTag(FYcAttributeSummaryParams& Params, const FGameplayTag& Tag)
{
	Params.TemporaryTags.AddTag(Tag);
}

bool UYcDamageBlueprintLibrary::HasTemporaryTag(const FYcAttributeSummaryParams& Params, const FGameplayTag& Tag)
{
	return Params.TemporaryTags.HasTag(Tag);
}

float UYcDamageBlueprintLibrary::GetSetByCallerValue(const FYcAttributeSummaryParams& Params, const FGameplayTag& Tag, bool& bFound)
{
	bFound = false;
	if (const float* ValuePtr = Params.SetByCallerMagnitudes.Find(Tag))
	{
		bFound = true;
		return *ValuePtr;
	}
	return 0.0f;
}

bool UYcDamageBlueprintLibrary::HasSetByCaller(const FYcAttributeSummaryParams& Params, const FGameplayTag& Tag)
{
	return Params.SetByCallerMagnitudes.Contains(Tag);
}

FString UYcDamageBlueprintLibrary::GetDamageDebugString(const FYcDamageSummaryParams& Params)
{
	FString Result;
	Result += FString::Printf(TEXT("BaseDamage: %.2f\n"), Params.BaseValue);
	Result += FString::Printf(TEXT("Override: %s (%.2f)\n"), Params.bOverride ? TEXT("true") : TEXT("false"), Params.OverrideValue);
	Result += FString::Printf(TEXT("PreMultiplyAdditive: %.2f\n"), Params.PreMultiplyAdditive);
	Result += FString::Printf(TEXT("Coefficient: %.2f\n"), Params.Coefficient);
	Result += FString::Printf(TEXT("PostMultiplyAdditive: %.2f\n"), Params.PostMultiplyAdditive);
	Result += FString::Printf(TEXT("FinalDamage: %.2f\n"), Params.GetFinalDamage());
	Result += FString::Printf(TEXT("Canceled: %s\n"), Params.bCancelExecution ? TEXT("true") : TEXT("false"));
	
	if (!Params.TemporaryTags.IsEmpty())
	{
		Result += FString::Printf(TEXT("TemporaryTags: %s\n"), *Params.TemporaryTags.ToStringSimple());
	}

	if (!Params.InfluenceRecords.IsEmpty())
	{
		Result += TEXT("Influences:\n");
		for (const FYcAttributeInfluence& Influence : Params.InfluenceRecords)
		{
			FString PriorityStr;
			switch (Influence.Priority)
			{
			case EYcAttributeInfluencePriority::Override:
				PriorityStr = TEXT("Override");
				break;
			case EYcAttributeInfluencePriority::PreMultiplyAdditive:
				PriorityStr = TEXT("PreMultiply");
				break;
			case EYcAttributeInfluencePriority::Coefficient:
				PriorityStr = TEXT("Coefficient");
				break;
			case EYcAttributeInfluencePriority::PostMultiplyAdditive:
				PriorityStr = TEXT("PostMultiply");
				break;
			}
			Result += FString::Printf(TEXT("  [%s] %.2f\n"), *PriorityStr, Influence.Value);
		}
	}

	return Result;
}

void UYcDamageBlueprintLibrary::PrintDamageDebug(const FYcDamageSummaryParams& Params)
{
	UE_LOG(LogTemp, Log, TEXT("=== Damage Debug ===\n%s"), *GetDamageDebugString(Params));
}
