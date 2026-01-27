// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFeatureAction.h"
#include "YcGameFeatureAction_AddGameplayCuePath.generated.h"

/**
 * UYcGameFeatureAction_AddGameplayCuePath
 * 
 * GameFeature Action：负责在 GameFeature 插件中添加 GameplayCue 扫描路径
 * 
 * 功能说明：
 * - 在 GameFeatureData 中配置需要添加的 GameplayCue 目录
 * - 路径相对于插件的 Content 目录
 * - 支持多个目录路径配置
 * - 提供编辑器数据验证
 * 
 * 使用方法：
 * 1. 在 GameFeatureData 的 Actions 数组中添加此 Action
 * 2. 配置 DirectoryPathsToAdd，例如："/GameplayCueNotifies"
 * 3. 路径会自动转换为完整路径，如："/YcShooterExample/GameplayCueNotifies"
 * 
 * @see UAbilitySystemGlobals::GameplayCueNotifyPaths
 */
UCLASS(MinimalAPI, meta = (DisplayName = "Add Gameplay Cue Path"))
class UYcGameFeatureAction_AddGameplayCuePath : public UGameFeatureAction
{
	GENERATED_BODY()
public:

	UYcGameFeatureAction_AddGameplayCuePath();

	//~UObject interface
#if WITH_EDITOR
	/** 编辑器数据验证，检查配置的路径是否有效 */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	//~End of UObject interface

	/** 获取配置的目录路径列表 */
	const TArray<FDirectoryPath>& GetDirectoryPathsToAdd() const { return DirectoryPathsToAdd; }

private:
	/** 
	 * 需要注册到 GameplayCueManager 的目录路径列表
	 * 这些路径相对于插件的 Content 目录
	 * 例如："/GameplayCueNotifies" 会被转换为 "/YcShooterExample/GameplayCueNotifies"
	 */
	UPROPERTY(EditAnywhere, Category = "Game Feature | Gameplay Cues", meta = (RelativeToGameContentDir, LongPackageName))
	TArray<FDirectoryPath> DirectoryPathsToAdd;
};
