// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "JsonObjectConverter.h"
#include "JsonObjectWrapper.h"
#include "UObject/Object.h"
#include "FJsonMixinLibrary.generated.h"

/**
 * 
 */
UCLASS(Meta = (ScriptMixin = "FJsonObjectWrapper"))
class YCANGELSCRIPTMIXIN_API UFJsonMixinLibrary : public UObject
{
	GENERATED_BODY()
	
	UFUNCTION(ScriptCallable)
	static FString ToJsonString(FJsonObjectWrapper& JsonObjectWrapper)
	{
		FString JsonStr = FString();
		FJsonObjectConverter::UStructToJsonObjectString(JsonObjectWrapper, JsonStr);
		return JsonStr;
	}

	UFUNCTION(ScriptCallable)
	static void LoadFromString(FJsonObjectWrapper& JsonObjectWrapper, const FString& Str)
	{
		JsonObjectWrapper.JsonObjectFromString(Str);
	}
};
