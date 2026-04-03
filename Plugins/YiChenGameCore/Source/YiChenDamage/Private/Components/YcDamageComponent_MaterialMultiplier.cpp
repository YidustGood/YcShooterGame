// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Components/YcDamageComponent_MaterialMultiplier.h"

#include "GameplayEffectExecutionCalculation.h"
#include "YiChenAbility/Public/YcAbilitySourceInterface.h"
#include "YiChenAbility/Public/YcGameplayEffectContext.h"
#include "Library/YcDamageBlueprintLibrary.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageComponent_MaterialMultiplier)

UYcDamageComponent_MaterialMultiplier::UYcDamageComponent_MaterialMultiplier()
{
	Priority = 35;
	DebugName = TEXT("MaterialMultiplier");
}

void UYcDamageComponent_MaterialMultiplier::Execute_Implementation(FYcAttributeSummaryParams& Params)
{
	if (!bEnableMaterialMultiplier)
	{
		return;
	}

	if (!Params.ExecParams)
	{
		return;
	}

	const FGameplayEffectSpec& Spec = Params.ExecParams->GetOwningSpec();
	FYcGameplayEffectContext* TypedContext = FYcGameplayEffectContext::ExtractEffectContext(Spec.GetContext());
	if (!TypedContext)
	{
		return;
	}

	float MaterialMultiplier = DefaultMultiplier;

	// 尝试从 AbilitySource 获取物理材质倍率（可增可减，如爆头2.0倍、四肢0.8倍）
	if (const IYcAbilitySourceInterface* AbilitySource = TypedContext->GetAbilitySource())
	{
		if (const UPhysicalMaterial* PhysMat = TypedContext->GetPhysicalMaterial())
		{
			const FGameplayTagContainer* AggSourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
			const FGameplayTagContainer* AggTargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
			MaterialMultiplier = AbilitySource->GetPhysicalMaterialMultiplier(PhysMat, AggSourceTags, AggTargetTags);
		}
	}

	MaterialMultiplier = FMath::Max(MaterialMultiplier, 0.0f);

	// 应用材质倍率到系数乘区（倍率可能大于1如爆头，也可能小于1如四肢）
	if (!FMath::IsNearlyEqual(MaterialMultiplier, 1.0f))
	{
		UYcDamageBlueprintLibrary::MultiplyCoefficient(Params, MaterialMultiplier);
		LogDebug(FString::Printf(TEXT("Material Multiplier: %.2f"), MaterialMultiplier));
	}
}
