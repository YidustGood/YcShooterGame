// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PlayMontageCallbackProxy.h"
#include "UObject/Object.h"
#include "SkeletalMeshComponentMixLibrary.generated.h"

/**
 * 
 */
UCLASS(Meta = (ScriptMixin = "USkeletalMeshComponent"))
class YCANGELSCRIPTMIXIN_API USkeletalMeshComponentMixLibrary : public UObject
{
	GENERATED_BODY()
	UFUNCTION(ScriptCallable)
	static UPlayMontageCallbackProxy* PlayMontage(USkeletalMeshComponent* SkeletalMeshComponent,
		class UAnimMontage* MontageToPlay, 
		float PlayRate = 1.f, 
		float StartingPosition = 0.f, 
		FName StartingSection = NAME_None,
		bool bShouldStopAllMontages = true)
	{
		return UPlayMontageCallbackProxy::CreateProxyObjectForPlayMontage(SkeletalMeshComponent, MontageToPlay, PlayRate, StartingPosition, StartingSection, bShouldStopAllMontages);
		
	}
	
};