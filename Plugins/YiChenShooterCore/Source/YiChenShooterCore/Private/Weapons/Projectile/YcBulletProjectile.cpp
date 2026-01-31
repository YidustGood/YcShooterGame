// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/Projectile/YcBulletProjectile.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcBulletProjectile)

AYcBulletProjectile::AYcBulletProjectile()
{
	// 子弹网格
	BulletMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BulletMesh"));
	BulletMesh->SetupAttachment(RootComponent);
	BulletMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BulletMesh->CastShadow = false;

	// 默认值
	MaxPenetrations = 0;
	PenetrationDamageMultiplier = 0.7f;
	HeadshotMultiplier = 2.0f;
}

void AYcBulletProjectile::ActivateFromPool(const FYcProjectileInitParams& Params)
{
	Super::ActivateFromPool(Params);

	// 重置穿透计数
	CurrentPenetrations = 0;
	HitActors.Reset();

	// 生成弹道轨迹
	SpawnTrailEffect();
}

void AYcBulletProjectile::ReturnToPool()
{
	Super::ReturnToPool();
}

void AYcBulletProjectile::HandleHit_Implementation(const FHitResult& HitResult, AActor* HitActor)
{
	UE_LOG(LogTemp, Warning, TEXT("HitActor: %s"), *HitActor->GetName());
	if (!HasAuthority())
	{
		return;
	}
	
	// 检查是否已命中过该Actor
	if (HitActor)
	{
		for (const TWeakObjectPtr<AActor>& WeakActor : HitActors)
		{
			if (WeakActor.Get() == HitActor)
			{
				return; // 已命中过，跳过
			}
		}
		HitActors.Add(HitActor);
	}
	
	// 生成命中特效
	SpawnHitEffect(HitResult);
	
	FYcProjectileHitResult ProjectileHitResult;
	ProjectileHitResult.HitResult = HitResult;
	
	// 应用伤害
	if (HitActor)
	{
		ProjectileHitResult.AppliedDamage = ApplyProjectileDamage(HitActor, HitResult);
	
		// 检测爆头
		if (HitResult.BoneName == TEXT("head") || HitResult.BoneName == TEXT("Head"))
		{
			ProjectileHitResult.bWasHeadshot = true;
		}
	}
	
	// 广播命中事件
	OnProjectileHit.Broadcast(this, ProjectileHitResult);
	
	// 检查穿透
	if (CurrentPenetrations < MaxPenetrations && HitActor)
	{
		CurrentPenetrations++;
		// 继续飞行，不返回池
		return;
	}
	
	// 返回对象池
	ReturnToPool();
}

float AYcBulletProjectile::ApplyProjectileDamage_Implementation(AActor* HitActor, const FHitResult& HitResult)
{
	if (!HitActor)
	{
		return 0.0f;
	}

	// 计算伤害（考虑穿透衰减）
	float FinalDamage = CurrentDamage;
	for (int32 i = 0; i < CurrentPenetrations; ++i)
	{
		FinalDamage *= PenetrationDamageMultiplier;
	}

	// 爆头加成
	if (HitResult.BoneName == TEXT("head") || HitResult.BoneName == TEXT("Head"))
	{
		FinalDamage *= HeadshotMultiplier;
	}

	// 应用伤害 - 这里使用UE的通用伤害系统
	// 实际项目中应该使用GAS或自定义伤害系统
	UGameplayStatics::ApplyPointDamage(
		HitActor,
		FinalDamage,
		GetActorForwardVector(),
		HitResult,
		CachedInstigatorController.Get(),
		CachedInstigator.Get(),
		nullptr // DamageTypeClass
	);

	return FinalDamage;
}

void AYcBulletProjectile::ResetProjectile()
{
	Super::ResetProjectile();

	CurrentPenetrations = 0;
	HitActors.Reset();
}

void AYcBulletProjectile::SpawnHitEffect_Implementation(const FHitResult& HitResult)
{
	if (HitEffect)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			HitEffect,
			HitResult.ImpactPoint,
			HitResult.ImpactNormal.Rotation(),
			true
		);
	}
}

void AYcBulletProjectile::SpawnTrailEffect_Implementation()
{
	if (TrailEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(
			TrailEffect,
			RootComponent,
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true
		);
	}
}
