// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "YcProjectileBase.h"
#include "YcBulletProjectile.generated.h"

class UStaticMeshComponent;
class UParticleSystemComponent;
class UNiagaraComponent;

/**
 * 标准子弹投射物
 * 
 * 适用于步枪、手枪等常规武器的子弹
 * 支持：
 * - 可视化网格/粒子
 * - 弹道轨迹
 * - 穿透逻辑
 */
UCLASS(Blueprintable)
class YICHENSHOOTERCORE_API AYcBulletProjectile : public AYcProjectileBase
{
	GENERATED_BODY()

public:
	AYcBulletProjectile();

	//~ Begin AYcProjectileBase Interface
	virtual void ActivateFromPool(const FYcProjectileInitParams& Params) override;
	virtual void ReturnToPool() override;
	//~ End AYcProjectileBase Interface

protected:
	//~ Begin AYcProjectileBase Interface
	virtual void HandleHit_Implementation(const FHitResult& HitResult, AActor* HitActor) override;
	virtual float ApplyProjectileDamage_Implementation(AActor* HitActor, const FHitResult& HitResult) override;
	virtual void ResetProjectile() override;
	//~ End AYcProjectileBase Interface

	/** 生成命中特效 */
	UFUNCTION(BlueprintNativeEvent, Category = "Projectile|Effects")
	void SpawnHitEffect(const FHitResult& HitResult);

	/** 生成弹道轨迹 */
	UFUNCTION(BlueprintNativeEvent, Category = "Projectile|Effects")
	void SpawnTrailEffect();

protected:
	/** 子弹网格 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UStaticMeshComponent> BulletMesh;

	/** 最大穿透次数 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Combat")
	int32 MaxPenetrations = 0;

	/** 穿透伤害衰减 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Combat", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float PenetrationDamageMultiplier = 0.7f;

	/** 爆头伤害倍率 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Combat")
	float HeadshotMultiplier = 2.0f;

	/** 命中特效类 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Effects")
	TObjectPtr<UParticleSystem> HitEffect;

	/** 弹道轨迹特效类 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Projectile|Effects")
	TObjectPtr<UParticleSystem> TrailEffect;

private:
	/** 当前穿透次数 */
	int32 CurrentPenetrations = 0;

	/** 已命中的Actor列表（防止重复命中） */
	UPROPERTY()
	TArray<TWeakObjectPtr<AActor>> HitActors;
};
