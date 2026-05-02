// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Fragments/YcEquipmentFragment.h"
#include "GameplayTagContainer.h"
#include "YcFragment_MeleeAttackConfig.generated.h"

UENUM(BlueprintType)
enum class EYcMeleeAttackType : uint8
{
	Light UMETA(DisplayName="轻击"),
	Heavy UMETA(DisplayName="重击"),
};

UENUM(BlueprintType)
enum class EYcMeleeTraceShape : uint8
{
	Line UMETA(DisplayName="线"),
	Sphere UMETA(DisplayName="球"),
	Capsule UMETA(DisplayName="胶囊"),
	Box UMETA(DisplayName="盒"),
};

USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcMeleeAttackProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="动作", meta=(DisplayName="动作标签"))
	FGameplayTag ActionTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="伤害", meta=(DisplayName="基础伤害", ClampMin="0.0"))
	float BaseDamage = 25.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="伤害", meta=(DisplayName="伤害类型"))
	FGameplayTag DamageTypeTag;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="伤害",
		meta=(DisplayName="命中区域伤害倍率表", Categories="Gameplay.Character.Zone"))
	TMap<FGameplayTag, float> HitZoneDamageMultipliers;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="检测", meta=(DisplayName="检测形状"))
	EYcMeleeTraceShape TraceShape = EYcMeleeTraceShape::Line;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="检测", meta=(DisplayName="检测间隔", ClampMin="0.01"))
	float AttackCheckInterval = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="检测", meta=(DisplayName="胶囊半径", EditCondition="TraceShape == EYcMeleeTraceShape::Capsule", ClampMin="0.0"))
	float TraceCapsuleRadius = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="检测", meta=(DisplayName="胶囊半高", EditCondition="TraceShape == EYcMeleeTraceShape::Capsule", ClampMin="0.0"))
	float TraceCapsuleHalfHeight = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="检测", meta=(DisplayName="球半径", EditCondition="TraceShape == EYcMeleeTraceShape::Sphere", ClampMin="0.0"))
	float TraceSphereRadius = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="检测", meta=(DisplayName="盒半尺寸", EditCondition="TraceShape == EYcMeleeTraceShape::Box"))
	FVector TraceBoxHalfSize = FVector(4.0f, 4.0f, 4.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="检测", meta=(DisplayName="盒朝向", EditCondition="TraceShape == EYcMeleeTraceShape::Box"))
	FRotator TraceBoxOrientation = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="背刺", meta=(DisplayName="背刺伤害倍率", ClampMin="1.0"))
	float BackstabDamageMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="背刺", meta=(DisplayName="背刺最大夹角(度)", ClampMin="0.0", ClampMax="90.0"))
	float BackstabMaxAngleDegrees = 45.0f;
};

USTRUCT(BlueprintType, meta=(DisplayName="近战攻击配置"))
struct YICHENSHOOTERCORE_API FYcFragment_MeleeAttackConfig : public FYcEquipmentFragment
{
	GENERATED_BODY()

	FYcFragment_MeleeAttackConfig();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="攻击模式", meta=(DisplayName="轻击配置"))
	FYcMeleeAttackProfile LightAttack;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="攻击模式", meta=(DisplayName="重击配置"))
	FYcMeleeAttackProfile HeavyAttack;

	const FYcMeleeAttackProfile& GetAttackProfile(EYcMeleeAttackType AttackType) const;

	virtual FString GetDebugString() const override;
};
