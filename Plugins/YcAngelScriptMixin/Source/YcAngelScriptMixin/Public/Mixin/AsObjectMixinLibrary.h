// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagAssetInterface.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/Object.h"
#include "AsObjectMixinLibrary.generated.h"

/**
 * 为AngelScript脚本的UObject扩展功能
 */
UCLASS(Meta = (ScriptMixin = "UObject"))
class YCANGELSCRIPTMIXIN_API UAsObjectMixinLibrary : public UObject
{
	GENERATED_BODY()
public:

	// This can be used in script as:
	//  As_DelayUntilNextTick();回调函数记得添加UFUNCTION()
	UFUNCTION(ScriptCallable)
	static void As_DelayUntilNextTickForAs(UObject* Object, const FName ExecutionFunction)
	{
		const int32 UUID = Object->GetUniqueID();
		const FLatentActionInfo LatentActionInfo(0, FMath::Rand(), *ExecutionFunction.ToString(), Object);
		UKismetSystemLibrary::DelayUntilNextTick(Object, LatentActionInfo);
	}

	// This can be used in script as:
	//  As_Delay(); 回调函数记得添加UFUNCTION()
	UFUNCTION(ScriptCallable)
	static void DelayForAs(UObject* Object,FName ExecutionFunction, const float Duration = 2.0f)
	{
		const FLatentActionInfo LatentInfo(0, FMath::Rand(), *ExecutionFunction.ToString(), Object);
		UKismetSystemLibrary::Delay(Object, Duration, LatentInfo);
	}

	UFUNCTION(ScriptCallable)
	static bool As_HasMatchingGameplayTag(UObject* Object,const FGameplayTagContainer& TagContainer)
	{
		TScriptInterface<IGameplayTagAssetInterface> GameplayTagAssetInterface;
		GameplayTagAssetInterface.SetObject(Object);
		GameplayTagAssetInterface.SetInterface(Cast<IGameplayTagAssetInterface>(Object));
		return GameplayTagAssetInterface.GetInterface()->HasAnyMatchingGameplayTags(TagContainer);
	}
	// 从自生的CDO对象复制一份新的实例对象
	UFUNCTION(ScriptCallable)
	static UObject* As_DuplicateObjectForAs(const UObject* Object,const FString& PackagePath, const FString& ObjectName)
	{
		UPackage* MyPackage = CreatePackage(*PackagePath);
		UObject* NewObj = DuplicateObject<UObject>(Object->GetClass()->GetDefaultObject(), MyPackage);
		return NewObj;
	}

	// 从自生的CDO对象复制一份新的实例对象
	UFUNCTION(ScriptCallable)
	static void As_MarkAsGarbageForAs(UObject* Object)
	{
		if(IsValid(Object))
		{
			Object->MarkAsGarbage();
		}
	}
	
	
};
