// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcPickupVisualData.h"

#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcPickupVisualData)

UStaticMesh* UYcPickupVisualData::GetResolvedDisplayMesh() const
{
	return DisplayMesh.Get();
}

TArray<UMaterialInterface*> UYcPickupVisualData::GetResolvedOverrideMaterials() const
{
	TArray<UMaterialInterface*> Results;
	Results.Reserve(OverrideMaterials.Num());
	for (const TSoftObjectPtr<UMaterialInterface>& MaterialPtr : OverrideMaterials)
	{
		if (UMaterialInterface* Material = MaterialPtr.Get())
		{
			Results.Add(Material);
		}
	}
	return Results;
}
