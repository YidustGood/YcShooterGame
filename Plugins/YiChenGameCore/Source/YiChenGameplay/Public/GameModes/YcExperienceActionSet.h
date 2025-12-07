// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "YcExperienceActionSet.generated.h"

class UGameFeatureAction;

/**
 * 游戏体验的GameFeatureAction集合资产
 * 
 * 定义进入体验过程中要开启的一系列GameFeature插件和需要执行的一系列GameFeatureAction。
 * ActionSet是一组预定义的Action和Feature集合，可以在多个Experience中复用，
 * 实现游戏功能的模块化和可复用性。
 */
UCLASS()
class YICHENGAMEPLAY_API UYcExperienceActionSet : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UYcExperienceActionSet();

#if WITH_EDITOR
	/**
	 * 编辑器下的数据验证
	 * 验证Actions中没有空的Action
	 * @param Context 数据验证上下文
	 * @return 数据验证结果
	 */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	
#if WITH_EDITORONLY_DATA
	//~UPrimaryDataAsset interface
	virtual void UpdateAssetBundleData() override;
	//~End of UPrimaryDataAsset interface
	
	/**
	 * 开发阶段的ActionSet描述
	 * 仅在编辑器中存在，方便开发阶段的资产管理和维护
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Develop")
	FString DevDescription;
#endif
	
	/**
	 * 此ActionSet中要执行的GameFeatureAction列表
	 * 这些Action会在ActionSet被应用到Experience时执行，
	 * 用于实现各种游戏业务逻辑，如添加UI、输入、组件等
	 */
	UPROPERTY(EditAnywhere, Instanced, Category="Actions to Perform")
	TArray<TObjectPtr<UGameFeatureAction>> Actions;

	/**
	 * 此ActionSet要激活的GameFeature插件列表
	 * 这些插件会在ActionSet被应用到Experience时被遍历激活
	 */
	UPROPERTY(EditAnywhere, Category="Feature Dependencies")
	TArray<FString> GameFeaturesToEnable;
};
