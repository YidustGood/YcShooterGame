// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Fragments/ItemFragment_GridRegions.h"
#include "Fragments/ItemFragment_GridRegions.h"
#include "UObject/Object.h"
#include "YiChenGameCoreMixin.generated.h"

/**
 * 
 */
UCLASS(Meta = (ScriptMixin = "FGridRegionShapeCell"))
class YCANGELSCRIPTMIXIN_API UFGridRegionShapeCellMixin : public UObject
{
	GENERATED_BODY()
	
	UFUNCTION(ScriptCallable)
	static FIntPoint ToPoint(const FGridRegionShapeCell& ShapeCell)
	{
		return ShapeCell.ToPoint();
	}
};
