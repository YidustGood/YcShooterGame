// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "LatentActionInfoMixinLibrary.generated.h"

/**
 * 
 */
UCLASS(Meta = (ScriptMixin = "FLatentActionInfo"))
class YCANGELSCRIPTMIXIN_API UFLatentActionInfoMixinLibrary : public UObject
{
	GENERATED_BODY()
public:

	// 绑定的回调函数记得添加UFUNCTION()
	UFUNCTION(ScriptCallable)
	static void Init(FLatentActionInfo& LatentActionInfo, const FName ExecutionFunction, UObject* CallbackTarget)
	{
		LatentActionInfo.Linkage = 0;
		LatentActionInfo.UUID = FMath::Rand();
		LatentActionInfo.ExecutionFunction = ExecutionFunction;
		LatentActionInfo.CallbackTarget = CallbackTarget;
	}
};
