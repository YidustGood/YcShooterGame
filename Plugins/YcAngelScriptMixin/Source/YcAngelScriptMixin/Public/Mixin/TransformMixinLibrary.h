// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "TransformMixinLibrary.generated.h"

/**
 * 
 */
UCLASS(Meta = (ScriptMixin = "FTransform"))
class YCANGELSCRIPTMIXIN_API UTransformMixinLibrary : public UObject
{
	GENERATED_BODY()
public:
	
	/** 
	 *	Transform a direction vector by the supplied transform - will not change its length. 
	 *	For example, if T was an object's transform, this would transform a direction from local space to world space.
	 */
	UFUNCTION(ScriptCallable)
	static FVector TransformDirection(const FTransform& T, FVector Direction)
	{
		return T.TransformVectorNoScale(Direction);
	}
};
