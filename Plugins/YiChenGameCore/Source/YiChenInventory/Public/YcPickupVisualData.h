// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "YcPickupVisualData.generated.h"

class UMaterialInterface;
class UStaticMesh;

/**
 * 地面拾取物的世界展示资源。
 * 通过 ItemDefinition -> FItemFragment_DataAsset -> Asset.Visual.Pickup 进行引用。
 */
UCLASS(BlueprintType)
class YICHENINVENTORY_API UYcPickupVisualData : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	/** 拾取物显示用的静态网格。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup Visual", meta = (AssetBundles = "Pickup"))
	TSoftObjectPtr<UStaticMesh> DisplayMesh;

	/** 可选材质覆盖。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup Visual", meta = (AssetBundles = "Pickup"))
	TArray<TSoftObjectPtr<UMaterialInterface>> OverrideMaterials;

	/** 显示网格的相对变换。 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pickup Visual")
	FTransform MeshRelativeTransform = FTransform::Identity;

	UFUNCTION(BlueprintPure, Category = "Pickup Visual")
	UStaticMesh* GetResolvedDisplayMesh() const;

	UFUNCTION(BlueprintPure, Category = "Pickup Visual")
	TArray<UMaterialInterface*> GetResolvedOverrideMaterials() const;
};
