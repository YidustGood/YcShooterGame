// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayCueNotifyTypes.h"
#include "FGameplayCueNotify_SpawnResult_MixinLibrary.generated.h"

/**
 * 
 */
UCLASS(Meta = (ScriptMixin = "FGameplayCueNotify_SpawnResult"))
class YCANGELSCRIPTMIXIN_API UFGameplayCueNotify_SpawnResult_MixinLibrary : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(ScriptCallable)
	static void GetCameraLensEffectObjects(const FGameplayCueNotify_SpawnResult& SpawnResults, TArray<UObject*>& OutCameraLensEffectObjects);

	// 常规返回值做法，性能稍差与参数返回的形式
	// UFUNCTION(ScriptCallable)
	// static TArray<UObject*> GetCameraLensEffectObjects(const FGameplayCueNotify_SpawnResult& SpawnResults)
	// {
	// 	TArray<TObjectPtr<UObject>> CameraLensEffectObjects;
	// 	for (const auto& Interface : SpawnResults.CameraLensEffects)
	// 	{
	// 		if (UObject* Object = Interface.GetObject())
	// 		{
	// 			CameraLensEffectObjects.Add(Object);
	// 		}
	// 	}
	// 	return CameraLensEffectObjects;
	// }
};
