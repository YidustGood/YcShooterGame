// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Components/YcDamageComponent_Immunity.h"

#include "AbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageComponent_Immunity)

UYcDamageComponent_Immunity::UYcDamageComponent_Immunity()
{
	Priority = 10;
	DebugName = TEXT("Immunity");
}

void UYcDamageComponent_Immunity::Execute_Implementation(FYcAttributeSummaryParams& Params)
{
	if (!bEnableImmunityCheck || ImmunityTags.IsEmpty())
	{
		return;
	}

	// 检查目标是否拥有免疫标签
	if (Params.TargetASC && Params.TargetASC->HasAnyMatchingGameplayTags(ImmunityTags))
	{
		// 免疫生效，取消伤害执行
		Params.bCancelExecution = true;

		// 添加免疫标签到临时标签
		Params.TemporaryTags.AppendTags(ImmunityTags);

		if (bEnableDebugLog)
		{
			LogDebug(FString::Printf(TEXT("Immunity triggered! Target has immunity tags.")));
		}
	}
}
