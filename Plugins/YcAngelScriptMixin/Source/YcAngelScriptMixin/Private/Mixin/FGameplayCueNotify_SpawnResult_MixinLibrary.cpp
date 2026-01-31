// Fill out your copyright notice in the Description page of Project Settings.


#include "Mixin/FGameplayCueNotify_SpawnResult_MixinLibrary.h"

void UFGameplayCueNotify_SpawnResult_MixinLibrary::GetCameraLensEffectObjects(
	const FGameplayCueNotify_SpawnResult& SpawnResults, TArray<UObject*>& OutCameraLensEffectObjects)
{
	OutCameraLensEffectObjects.Empty();
	for (const auto& Interface : SpawnResults.CameraLensEffects)
	{
		if (UObject* Object = Interface.GetObject())
		{
			OutCameraLensEffectObjects.Add(Object);
		}
	}
}
