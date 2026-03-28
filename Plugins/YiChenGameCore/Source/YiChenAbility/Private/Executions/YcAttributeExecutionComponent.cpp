// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Executions/YcAttributeExecutionComponent.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectExecutionCalculation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAttributeExecutionComponent)

UYcAttributeExecutionComponent::UYcAttributeExecutionComponent()
	: Priority(100)
	, bEnabled(true)
{
}

bool UYcAttributeExecutionComponent::ShouldExecute(const FYcAttributeSummaryParams& Params, const FGameplayTagContainer& InSourceTags, const FGameplayTagContainer& InTargetTags) const
{
	// 检查是否启用
	if (!bEnabled)
	{
		return false;
	}

	// 检查类型标签
	if (!TypeTags.IsEmpty())
	{
		// 子类需要提供类型标签检查逻辑
		// 默认不检查，由子类重写
	}

	// 检查源标签
	if (!IsSourceTagsMatched(InSourceTags))
	{
		return false;
	}

	// 检查目标标签
	if (!IsTargetTagsMatched(InTargetTags))
	{
		return false;
	}

	// 调用蓝图可重写的检查
	return K2_ShouldExecute(Params, InSourceTags, InTargetTags);
}

bool UYcAttributeExecutionComponent::K2_ShouldExecute_Implementation(const FYcAttributeSummaryParams& Params, const FGameplayTagContainer& InSourceTags, const FGameplayTagContainer& InTargetTags) const
{
	// 默认总是返回 true，子类可重写
	return true;
}

void UYcAttributeExecutionComponent::Execute_Implementation(FYcAttributeSummaryParams& Params)
{
	// 基类空实现，子类必须重写
}

UWorld* UYcAttributeExecutionComponent::GetWorld() const
{
	if (const UObject* Outer = GetOuter())
	{
		return Outer->GetWorld();
	}
	return nullptr;
}

#if WITH_EDITOR
FText UYcAttributeExecutionComponent::GetDefaultNodeTitle() const
{
	if (!DebugName.IsEmpty())
	{
		return FText::FromString(DebugName);
	}
	return GetClass()->GetDisplayNameText();
}
#endif

float UYcAttributeExecutionComponent::GetSourceAttribute(const FYcAttributeSummaryParams& Params, const FGameplayAttribute& Attribute, bool* bFound) const
{
	if (Params.SourceASC)
	{
		return Params.SourceASC->GetGameplayAttributeValue(Attribute, *bFound);
	}
	if (bFound)
	{
		*bFound = false;
	}
	return 0.0f;
}

float UYcAttributeExecutionComponent::GetTargetAttribute(const FYcAttributeSummaryParams& Params, const FGameplayAttribute& Attribute, bool* bFound) const
{
	if (Params.TargetASC)
	{
		return Params.TargetASC->GetGameplayAttributeValue(Attribute, *bFound);
	}
	if (bFound)
	{
		*bFound = false;
	}
	return 0.0f;
}

void UYcAttributeExecutionComponent::ApplyModToAttribute(FYcAttributeSummaryParams& Params, const FGameplayAttribute& Attribute, float Value, EGameplayModOp::Type Op) const
{
	if (Params.TargetASC)
	{
		Params.TargetASC->ApplyModToAttribute(Attribute, Op, Value);
	}
}

void UYcAttributeExecutionComponent::AddOutputModifier(FYcAttributeSummaryParams& Params, const FGameplayAttribute& Attribute, float Value, EGameplayModOp::Type Op) const
{
	if (Params.ExecOutput)
	{
		Params.ExecOutput->AddOutputModifier(FGameplayModifierEvaluatedData(Attribute, Op, Value));
	}
}

float UYcAttributeExecutionComponent::GetSetByCallerValue(const FYcAttributeSummaryParams& Params, const FGameplayTag& Tag, bool* bFound) const
{
	if (const float* ValuePtr = Params.SetByCallerMagnitudes.Find(Tag))
	{
		if (bFound)
		{
			*bFound = true;
		}
		return *ValuePtr;
	}
	if (bFound)
	{
		*bFound = false;
	}
	return 0.0f;
}

void UYcAttributeExecutionComponent::LogDebug(const FString& Message) const
{
#if !UE_BUILD_SHIPPING
	if (!bEnableDebugLog)
	{
		return;
	}
	const FString ComponentName = DebugName.IsEmpty() ? GetName() : DebugName;
	UE_LOG(LogTemp, Log, TEXT("[%s] %s"), *ComponentName, *Message);
#endif
}
