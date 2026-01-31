// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YcEditorUtilitiesStatics.generated.h"

/**
 * 
 */
UCLASS()
class YCEDITORUTILITIES_API UYcEditorUtilitiesStatics : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
#if WITH_EDITOR
	// 打开GameplayTags管理窗口
	UFUNCTION(BlueprintCallable, ScriptCallable, Category = "YcEditorUtilities")
	static void OpenGameplayTagsManager();
#endif
};
