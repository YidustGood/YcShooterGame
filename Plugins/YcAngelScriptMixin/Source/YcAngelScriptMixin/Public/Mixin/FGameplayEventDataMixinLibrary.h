// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "UObject/Object.h"
#include "FGameplayEventDataMixinLibrary.generated.h"

/**
 * FGameplayEventData 上有一批在脚本里以 const 形式暴露的对象引用，
 * 这里提供 mixin 访问，便于 Angelscript 在需要时拿到可变对象。
 */
UCLASS(Meta = (ScriptMixin = "FGameplayEventData"))
class YCANGELSCRIPTMIXIN_API UFGameplayEventDataMixinLibrary : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(ScriptCallable)
	static AActor* GetMutableTargetActor(const FGameplayEventData& EventData)
	{
		return const_cast<AActor*>(EventData.Target.Get());
	}

	UFUNCTION(ScriptCallable)
	static AActor* GetMutableInstigatorActor(const FGameplayEventData& EventData)
	{
		return const_cast<AActor*>(EventData.Instigator.Get());
	}

	UFUNCTION(ScriptCallable)
	static UObject* GetMutableOptionalObject(const FGameplayEventData& EventData)
	{
		return const_cast<UObject*>(EventData.OptionalObject.Get());
	}

	UFUNCTION(ScriptCallable)
	static UObject* GetMutableOptionalObject2(const FGameplayEventData& EventData)
	{
		return const_cast<UObject*>(EventData.OptionalObject2.Get());
	}
};
