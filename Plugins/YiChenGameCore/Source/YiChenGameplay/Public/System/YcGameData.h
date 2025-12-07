// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "YcGameData.generated.h"

class UGameplayEffect;

/**
 * 全局游戏数据资源
 * 
 * 运行时不可变的数据资产，包含全局游戏配置数据。
 * 存储游戏系统所需的默认GameplayEffect和其他全局配置。
 */
UCLASS(BlueprintType, Const, Meta = (DisplayName = "YcGameData", ShortTooltip = "Data asset containing global game data."))
class YICHENGAMEPLAY_API UYcGameData : public UPrimaryDataAsset
{
	GENERATED_BODY()
public:
	UYcGameData();

	/**
	 * 获取全局游戏数据
	 * @return 加载的全局游戏数据引用
	 */
	static const UYcGameData& Get();
	
	/**
	 * 伤害效果
	 * 用于施加伤害的GameplayEffect，使用SetByCaller方式设置伤害值
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Default Gameplay Effects", meta = (DisplayName = "Damage Gameplay Effect (SetByCaller)"))
	TSoftClassPtr<UGameplayEffect> DamageGameplayEffect_SetByCaller;

	/**
	 * 治疗效果
	 * 用于施加治疗的GameplayEffect，使用SetByCaller方式设置治疗值
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Default Gameplay Effects", meta = (DisplayName = "Heal Gameplay Effect (SetByCaller)"))
	TSoftClassPtr<UGameplayEffect> HealGameplayEffect_SetByCaller;

	/**
	 * 动态标签效果
	 * 用于添加和删除动态GameplayTag的GameplayEffect
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Default Gameplay Effects")
	TSoftClassPtr<UGameplayEffect> DynamicTagGameplayEffect;
};
