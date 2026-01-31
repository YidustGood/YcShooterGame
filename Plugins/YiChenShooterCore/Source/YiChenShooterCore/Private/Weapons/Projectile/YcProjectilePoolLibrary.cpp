// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/Projectile/YcProjectilePoolLibrary.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcProjectilePoolLibrary)

UYcProjectilePoolSubsystem* UYcProjectilePoolLibrary::GetProjectilePoolSubsystem(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!World)
	{
		return nullptr;
	}

	return World->GetSubsystem<UYcProjectilePoolSubsystem>();
}

AYcProjectileBase* UYcProjectilePoolLibrary::FireProjectile(
	const UObject* WorldContextObject,
	FName PoolID,
	FVector SpawnLocation,
	FVector Direction,
	AActor* Instigator,
	float Damage,
	float Speed)
{
	FYcProjectileInitParams Params;
	Params.SpawnLocation = SpawnLocation;
	Params.Direction = Direction.GetSafeNormal();
	Params.Damage = Damage;
	Params.InitialSpeed = Speed;
	Params.MaxSpeed = Speed * 1.5f;
	Params.Instigator = Instigator;

	if (Instigator)
	{
		if (const APawn* Pawn = Cast<APawn>(Instigator))
		{
			Params.InstigatorController = Pawn->GetController();
		}
	}

	return FireProjectileWithParams(WorldContextObject, PoolID, Params);
}

AYcProjectileBase* UYcProjectilePoolLibrary::FireProjectileWithParams(
	const UObject* WorldContextObject,
	FName PoolID,
	const FYcProjectileInitParams& Params)
{
	UYcProjectilePoolSubsystem* PoolSubsystem = GetProjectilePoolSubsystem(WorldContextObject);
	if (!PoolSubsystem)
	{
		return nullptr;
	}

	return PoolSubsystem->AcquireProjectile(PoolID, Params);
}

bool UYcProjectilePoolLibrary::QuickSetupPool(
	const UObject* WorldContextObject,
	FName PoolID,
	TSubclassOf<AYcProjectileBase> ProjectileClass,
	int32 InitialSize,
	int32 MaxSize)
{
	UYcProjectilePoolSubsystem* PoolSubsystem = GetProjectilePoolSubsystem(WorldContextObject);
	if (!PoolSubsystem)
	{
		return false;
	}

	// 如果池已存在，直接返回成功
	if (PoolSubsystem->HasPool(PoolID))
	{
		return true;
	}

	FYcProjectilePoolConfig Config;
	Config.PoolID = PoolID;
	Config.ProjectileClass = ProjectileClass;
	Config.InitialPoolSize = InitialSize;
	Config.MaxPoolSize = MaxSize;
	Config.GrowthStep = FMath::Max(1, InitialSize / 5);
	Config.bAllowGrowth = true;

	if (PoolSubsystem->RegisterPool(Config))
	{
		PoolSubsystem->WarmupPool(PoolID);
		return true;
	}

	return false;
}

float UYcProjectilePoolLibrary::GetPoolUsageRate(const UObject* WorldContextObject, FName PoolID)
{
	UYcProjectilePoolSubsystem* PoolSubsystem = GetProjectilePoolSubsystem(WorldContextObject);
	if (!PoolSubsystem)
	{
		return -1.0f;
	}

	FYcProjectilePoolStats Stats;
	if (PoolSubsystem->GetPoolStats(PoolID, Stats))
	{
		return Stats.UsageRate;
	}

	return -1.0f;
}

bool UYcProjectilePoolLibrary::IsPoolHealthy(const UObject* WorldContextObject, FName PoolID, float WarningThreshold)
{
	const float UsageRate = GetPoolUsageRate(WorldContextObject, PoolID);
	if (UsageRate < 0.0f)
	{
		return false; // 池不存在
	}

	return UsageRate < WarningThreshold;
}
