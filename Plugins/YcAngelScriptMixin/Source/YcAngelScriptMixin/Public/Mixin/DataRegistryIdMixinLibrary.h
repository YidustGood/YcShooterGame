// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DataRegistryId.h"
#include "UObject/Object.h"
#include "DataRegistryIdMixinLibrary.generated.h"

UCLASS(Meta = (ScriptMixin = "FDataRegistryId"))
class YCANGELSCRIPTMIXIN_API UDataRegistryIdMixinLibrary : public UObject
{
	GENERATED_BODY()

public:
	UFUNCTION(ScriptCallable)
	static bool IsValid(const FDataRegistryId& DataRegistryId)
	{
		return DataRegistryId.IsValid();
	}

	UFUNCTION(ScriptCallable)
	static FString ToString(const FDataRegistryId& DataRegistryId)
	{
		return DataRegistryId.ToString();
	}
};

