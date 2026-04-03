// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Components/YcDamageComponent_DistanceAttenuation.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectExecutionCalculation.h"
#include "YiChenAbility/Public/YcAbilitySourceInterface.h"
#include "YiChenAbility/Public/YcGameplayEffectContext.h"
#include "Curves/CurveFloat.h"
#include "Library/YcDamageBlueprintLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageComponent_DistanceAttenuation)

UYcDamageComponent_DistanceAttenuation::UYcDamageComponent_DistanceAttenuation()
{
	Priority = 30;
	DebugName = TEXT("DistanceAttenuation");
}

void UYcDamageComponent_DistanceAttenuation::Execute_Implementation(FYcAttributeSummaryParams& Params)
{
	if (!bEnableDistanceAttenuation)
	{
		return;
	}

	// 从执行参数获取距离信息
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

	// 计算距离
	double Distance = WORLD_MAX;
	FVector ImpactLocation = FVector::ZeroVector;

	if (TypedContext->HasOrigin())
	{
		// 从 Context 获取命中位置
		if (const FHitResult* HitResult = TypedContext->GetHitResult())
		{
			ImpactLocation = HitResult->ImpactPoint;
		}
		else if (Params.TargetASC)
		{
			ImpactLocation = Params.TargetASC->GetAvatarActor_Direct()->GetActorLocation();
		}
		Distance = FVector::Dist(TypedContext->GetOrigin(), ImpactLocation);
	}
	else
	{
		// 尝试从 EffectCauser 获取
		AActor* EffectCauser = TypedContext->GetEffectCauser();
		if (EffectCauser && Params.TargetASC)
		{
			AActor* TargetActor = Params.TargetASC->GetAvatarActor_Direct();
			if (TargetActor)
			{
				Distance = FVector::Dist(EffectCauser->GetActorLocation(), TargetActor->GetActorLocation());
			}
		}
	}

	// 尝试从 AbilitySource 获取衰减曲线
	float Attenuation = 1.0f;
	if (const IYcAbilitySourceInterface* AbilitySource = TypedContext->GetAbilitySource())
	{
		// 使用 AbilitySource 的衰减计算
		const FGameplayTagContainer* AggSourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
		const FGameplayTagContainer* AggTargetTags = Spec.CapturedTargetTags.GetAggregatedTags();
		Attenuation = AbilitySource->GetDistanceAttenuation(Distance, AggSourceTags, AggTargetTags);
	}
	else
	{
		// 使用组件配置的衰减
		Attenuation = CalculateAttenuation(Distance);
	}

	Attenuation = FMath::Max(Attenuation, 0.0f);

	// 应用衰减到系数乘区
	if (Attenuation < 1.0f)
	{
		UYcDamageBlueprintLibrary::MultiplyCoefficient(Params, Attenuation);
		LogDebug(FString::Printf(TEXT("Distance: %.2f, Attenuation: %.2f"), Distance, Attenuation));
	}
}

float UYcDamageComponent_DistanceAttenuation::CalculateAttenuation(float Distance) const
{
	if (Distance <= MinAttenuationDistance)
	{
		return 1.0f;
	}

	if (Distance >= MaxAttenuationDistance)
	{
		return 0.0f;
	}

	// 使用曲线或线性插值
	if (AttenuationCurve)
	{
		const float Alpha = (Distance - MinAttenuationDistance) / (MaxAttenuationDistance - MinAttenuationDistance);
		return AttenuationCurve->GetFloatValue(Alpha);
	}
	else
	{
		// 线性衰减
		return 1.0f - (Distance - MinAttenuationDistance) / (MaxAttenuationDistance - MinAttenuationDistance);
	}
}
