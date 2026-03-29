// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Executions/YcAttributeExecution.h"

#include "AbilitySystemComponent.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAttributeExecution)

UYcAttributeExecution::UYcAttributeExecution()
	: bEnableDebugLog(false)
	, bAutoEnableDebugInPIE(true)
{
}

#if WITH_EDITOR
EDataValidationResult UYcAttributeExecution::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	// 检查 Components 是否有空条目
	for (int32 i = 0; i < Components.Num(); ++i)
	{
		if (Components[i] == nullptr)
		{
			Context.AddError(FText::Format(
				INVTEXT("Components[{0}] 不能为 None，请选择一个有效的组件类型或删除此条目"),
				FText::AsNumber(i)));
			Result = EDataValidationResult::Invalid;
		}
	}

	// 检查是否至少有一个组件
	if (Components.IsEmpty())
	{
		Context.AddWarning(INVTEXT("Components 数组为空，计算将不会执行任何逻辑"));
	}

	return Result;
}
#endif

void UYcAttributeExecution::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
#if WITH_SERVER_CODE
	// 初始化参数
	FYcAttributeSummaryParams Params;
	InitializeParams(Params, ExecutionParams);
	Params.ExecParams = &ExecutionParams;
	Params.ExecOutput = &OutExecutionOutput;

	// 获取标签
	FGameplayTagContainer SourceTags;
	FGameplayTagContainer TargetTags;
	GetTagContainers(ExecutionParams, SourceTags, TargetTags);

	// 排序组件（带缓存优化）
	SortComponents();

	// 遍历执行组件
	for (const TObjectPtr<UYcAttributeExecutionComponent>& Component : Components)
	{
		if (!Component)
		{
			continue;
		}

		// 检查是否应该执行
		if (!Component->ShouldExecute(Params, SourceTags, TargetTags))
		{
			continue;
		}

		// 执行组件逻辑
		Component->Execute(Params);

		// 检查是否取消后续执行
		if (Params.bCancelExecution)
		{
			break;
		}
	}
	
	// 通过 ASC 获取 World
	UWorld* World = GetWorldFromParams(Params);
	
	// 输出调试信息
	const bool bShouldLog = bEnableDebugLog || (bAutoEnableDebugInPIE && World && World->IsPlayInEditor());
	if (bShouldLog)
	{
		LogDebugInfo(Params);
	}
#endif
}

void UYcAttributeExecution::InitializeParams(FYcAttributeSummaryParams& Params, const FGameplayEffectCustomExecutionParameters& ExecutionParams) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	// 获取 ASC
	Params.SourceASC = ExecutionParams.GetSourceAbilitySystemComponent();
	Params.TargetASC = ExecutionParams.GetTargetAbilitySystemComponent();

	// 获取 SetByCaller 数据
	for (const TPair<FGameplayTag, float>& Pair : Spec.SetByCallerTagMagnitudes)
	{
		Params.SetByCallerMagnitudes.Add(Pair.Key, Pair.Value);
	}
}

void UYcAttributeExecution::GetTagContainers(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayTagContainer& OutSourceTags,
	FGameplayTagContainer& OutTargetTags) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	if (Spec.CapturedSourceTags.GetAggregatedTags())
	{
		OutSourceTags = *Spec.CapturedSourceTags.GetAggregatedTags();
	}

	if (Spec.CapturedTargetTags.GetAggregatedTags())
	{
		OutTargetTags = *Spec.CapturedTargetTags.GetAggregatedTags();
	}
}

void UYcAttributeExecution::SortComponents() const
{
	// 已排序则跳过
	if (bComponentsSorted)
	{
		return;
	}

	// 根据 bSortByPriority 决定是否排序
	if (bSortByPriority)
	{
		// 按 Priority 排序（数值小的先执行）
		Components.Sort([](const TObjectPtr<UYcAttributeExecutionComponent>& A, const TObjectPtr<UYcAttributeExecutionComponent>& B)
		{
			return A->Priority < B->Priority;
		});
	}
	// 否则保持编辑器配置顺序

	bComponentsSorted = true;
}

UWorld* UYcAttributeExecution::GetWorldFromParams(const FYcAttributeSummaryParams& Params) const
{
	// 优先从 TargetASC 获取
	if (Params.TargetASC)
	{
		return Params.TargetASC->GetWorld();
	}
	
	// 其次从 SourceASC 获取
	if (Params.SourceASC)
	{
		return Params.SourceASC->GetWorld();
	}
	
	// 最后尝试 UObject 的 GetWorld
	return GetWorld();
}

void UYcAttributeExecution::LogDebugInfo(const FYcAttributeSummaryParams& Params) const
{
	UE_LOG(LogTemp, Log, TEXT("=== YcAttributeExecution Debug ==="));
	UE_LOG(LogTemp, Log, TEXT("BaseValue: %.2f"), Params.BaseValue);
	UE_LOG(LogTemp, Log, TEXT("Override: %s (%.2f)"), Params.bOverride ? TEXT("true") : TEXT("false"), Params.OverrideValue);
	UE_LOG(LogTemp, Log, TEXT("PreMultiplyAdditive: %.2f"), Params.PreMultiplyAdditive);
	UE_LOG(LogTemp, Log, TEXT("Coefficient: %.2f"), Params.Coefficient);
	UE_LOG(LogTemp, Log, TEXT("PostMultiplyAdditive: %.2f"), Params.PostMultiplyAdditive);
	UE_LOG(LogTemp, Log, TEXT("FinalValue: %.2f"), Params.GetFinalValue());
	UE_LOG(LogTemp, Log, TEXT("Canceled: %s"), Params.bCancelExecution ? TEXT("true") : TEXT("false"));
	
	if (!Params.TemporaryTags.IsEmpty())
	{
		UE_LOG(LogTemp, Log, TEXT("TemporaryTags: %s"), *Params.TemporaryTags.ToStringSimple());
	}

	UE_LOG(LogTemp, Log, TEXT("================================="));
}
