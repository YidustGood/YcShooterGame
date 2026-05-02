// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Components/YcDamageComponent_BackstabMultiplier.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffectExecutionCalculation.h"
#include "YiChenAbility/Public/YcGameplayEffectContext.h"
#include "Library/YcDamageBlueprintLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageComponent_BackstabMultiplier)

UYcDamageComponent_BackstabMultiplier::UYcDamageComponent_BackstabMultiplier()
{
	Priority = 40;
	DebugName = TEXT("BackstabMultiplier");
}

void UYcDamageComponent_BackstabMultiplier::Execute_Implementation(FYcAttributeSummaryParams& Params)
{
	if (!Params.ExecParams || !Params.SourceASC || !Params.TargetASC)
	{
		return;
	}

	const FGameplayEffectSpec& Spec = Params.ExecParams->GetOwningSpec();
	FYcGameplayEffectContext* TypedContext = FYcGameplayEffectContext::ExtractEffectContext(Spec.GetContext());
	if (!TypedContext)
	{
		return;
	}

	const FYcBackstabRuntimePayload* BackstabPayload = TypedContext->FindRuntimePayload<FYcBackstabRuntimePayload>();
	if (!BackstabPayload)
	{
		return;
	}

	if (BackstabPayload->DamageMultiplier <= 1.0f)
	{
		return;
	}

	AActor* SourceActor = Params.SourceASC->GetAvatarActor();
	if (!SourceActor)
	{
		SourceActor = Params.SourceASC->GetOwnerActor();
	}

	AActor* TargetActor = Params.TargetASC->GetAvatarActor();
	if (!TargetActor)
	{
		TargetActor = Params.TargetASC->GetOwnerActor();
	}

	if (!SourceActor || !TargetActor)
	{
		return;
	}

	FVector ToAttacker = SourceActor->GetActorLocation() - TargetActor->GetActorLocation();
	FVector TargetForward = TargetActor->GetActorForwardVector();
	if (bProjectToHorizontalPlane)
	{
		ToAttacker.Z = 0.0f;
		TargetForward.Z = 0.0f;
	}

	if (!ToAttacker.Normalize() || !TargetForward.Normalize())
	{
		return;
	}

	const float DotThreshold = -FMath::Cos(FMath::DegreesToRadians(BackstabPayload->MaxAngleDegrees));
	const float FacingDot = FVector::DotProduct(TargetForward, ToAttacker);
	if (FacingDot > DotThreshold)
	{
		return;
	}

	UYcDamageBlueprintLibrary::MultiplyCoefficient(Params, BackstabPayload->DamageMultiplier);
	LogDebug(FString::Printf(TEXT("Backstab Multiplier: %.2f"), BackstabPayload->DamageMultiplier));
}
