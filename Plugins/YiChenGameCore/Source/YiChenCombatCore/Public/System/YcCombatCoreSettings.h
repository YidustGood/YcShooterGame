// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettings.h"
#include "YcCombatCoreSettings.generated.h"

class UGameplayEffect;

/**
 * CombatCore 运行时设置。
 *
 * 说明：
 * - 这里存放战斗领域的全局默认配置（Health/Armor/Damage/Heal）。
 * - 适合插件级、跨项目可复用的默认值。
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "YiChen CombatCore"))
class YICHENCOMBATCORE_API UYcCombatCoreSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/**
	 * UYcHealthComponent::DamageSelfDestruct 使用的默认 GameplayEffect。
	 * 建议配置为支持 TAG_Gameplay_Attribute_SetByCaller_Damage 的 SetByCaller 伤害 GE。
	 */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Health", meta = (AllowedClasses = "/Script/GameplayAbilities.GameplayEffect"))
	TSoftClassPtr<UGameplayEffect> DamageSelfDestructGameplayEffect;

	/**
	 * 通用 SetByCaller 伤害 GE。
	 * 用于需要在运行时写入 TAG_Gameplay_Attribute_SetByCaller_Damage 的通用伤害场景。
	 */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Health", meta = (AllowedClasses = "/Script/GameplayAbilities.GameplayEffect"))
	TSoftClassPtr<UGameplayEffect> DamageGameplayEffect_SetByCaller;

	/**
	 * 通用 SetByCaller 治疗 GE。
	 * 用于需要在运行时写入 TAG_Gameplay_Attribute_SetByCaller_Heal 的通用治疗场景。
	 */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Health", meta = (AllowedClasses = "/Script/GameplayAbilities.GameplayEffect"))
	TSoftClassPtr<UGameplayEffect> HealGameplayEffect_SetByCaller;

	/** 获取已加载的自毁伤害 GE 类；未配置时返回 nullptr。 */
	TSubclassOf<UGameplayEffect> GetDamageSelfDestructGameplayEffect() const;

	/** 获取已加载的通用伤害 GE 类；未配置时返回 nullptr。 */
	TSubclassOf<UGameplayEffect> GetDamageGameplayEffect_SetByCaller() const;

	/** 获取已加载的通用治疗 GE 类；未配置时返回 nullptr。 */
	TSubclassOf<UGameplayEffect> GetHealGameplayEffect_SetByCaller() const;
};
