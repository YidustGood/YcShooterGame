// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Weapons/YcHitScanWeaponInstance.h"
#include "YcMeleeWeaponInstance.generated.h"

/**
 * 近战武器实例
 * 复用 HitScan 武器的基础能力，但将近战特有的伤害配置解析收敛在该类型内。
 */
UCLASS()
class YICHENSHOOTERCORE_API UYcMeleeWeaponInstance : public UYcHitScanWeaponInstance
{
	GENERATED_BODY()

public:
	UYcMeleeWeaponInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual bool BuildCurrentDamageProfileView(const FYcGameplayEffectContext* EffectContext, FYcWeaponDamageProfileView& OutView) const override;
};
