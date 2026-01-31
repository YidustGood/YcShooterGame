// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AsyncAction_ObserveTeam.h"
#include "GameModes/AsyncAction_ExperienceReady.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "AsYcGameCoreAsyncLibrary.generated.h"

/**
 * 
 */
UCLASS()
class YCANGELSCRIPTMIXIN_API UAsYcGameCoreAsyncLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(ScriptCallable, Category = "Ability|Tasks", meta=(WorldContext = "WorldContextObject"))
	static UAsyncAction_ExperienceReady* WaitForExperienceReady(UObject* WorldContextObject)
	{
		return UAsyncAction_ExperienceReady::WaitForExperienceReady(WorldContextObject);
	}
	
	UFUNCTION(ScriptCallable, Category = "Ability|Tasks", meta=(WorldContext = "WorldContextObject"))
	static UAsyncAction_ObserveTeam* ObserveTeam(UObject* WorldContextObject, AActor* TeamAgent)
	{
		return UAsyncAction_ObserveTeam::ObserveTeam(TeamAgent);
	}
	
};
