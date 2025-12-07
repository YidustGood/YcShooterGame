// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "YcExperienceDefinition.generated.h"

class UYcPawnData;
class UGameFeatureAction;
class UYcExperienceActionSet;

/**
 * 游戏体验定义资产
 * 
 * 作为GameMode的补充增强，定义游戏体验的具体内容。包括要激活的GameFeature插件、
 * 默认PawnData、以及游戏体验要执行的GameFeatureAction, 可用自定义实现各种不同的GameFeatureAction。
 * 通过组合GameFeature、GameFeatureAction实现数据驱动的游戏功能组装。
 */
UCLASS()
class YICHENGAMEPLAY_API UYcExperienceDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UYcExperienceDefinition();

#if WITH_EDITOR
	/**
	 * 编辑器下的数据验证
	 * 验证Actions中没有空的Action，检查是否为当前C++类的有效子类
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
	 * 开发阶段的体验描述
	 * 仅在编辑器中存在，方便开发阶段的资产管理和维护
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Develop")
	FString DevDescription;
#endif
	
	/**
	 * 此体验要激活的GameFeature插件列表
	 * 这些插件会在体验加载时被激活，在体验卸载时被禁用
	 */
	UPROPERTY(EditDefaultsOnly, Category = Gameplay)
	TArray<FString> GameFeaturesToEnable;

	/**
	 * 此体验的默认Pawn配置数据
	 * 包含生成Pawn所需的配置信息，如Pawn类、技能集、输入配置等
	 * 开启了Experience后GameMode生成Pawn会从这里指定的PawnData读取
	 */
	UPROPERTY(EditDefaultsOnly, Category=Gameplay)
	TObjectPtr<const UYcPawnData> DefaultPawnData;

	/**
	 * 体验生命周期中要执行的GameFeatureAction列表
	 * 这些Action会在体验加载时执行操作，通过实现不同的GameFeatureAction可组装出不同的游戏体验内容
	 * 用于实现各种游戏业务逻辑，如添加UI、输入、组件等
	 */
	UPROPERTY(EditDefaultsOnly, Instanced, Category="Actions")
	TArray<TObjectPtr<UGameFeatureAction>> Actions;

	/**
	 * 体验生命周期中要执行的GameFeatureAction集合
	 * ActionSet是一组预定义的GameFeatureAction集合，可以在多个Experience中复用，实际开发中可用抽出一些通用的ActionSet做复用
	 */
	UPROPERTY(EditDefaultsOnly, Category=Gameplay)
	TArray<TObjectPtr<UYcExperienceActionSet>> ActionSets;
};
