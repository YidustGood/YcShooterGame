// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "ObjectPoolLibrary.h"
#include "ObjectPoolSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ObjectPoolLibrary)

UObjectPoolSubsystem* UObjectPoolLibrary::GetObjectPoolSubsystem(const UObject* WorldContextObject)
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

	return World->GetSubsystem<UObjectPoolSubsystem>();
}

bool UObjectPoolLibrary::QuickSetupPool(
	const UObject* WorldContextObject,
	FName PoolID,
	TSubclassOf<UObject> ObjectClass,
	int32 InitialSize,
	int32 MaxSize)
{
	UObjectPoolSubsystem* Subsystem = GetObjectPoolSubsystem(WorldContextObject);
	if (!Subsystem)
	{
		return false;
	}

	// 如果池已存在，直接返回成功
	if (Subsystem->HasPool(PoolID))
	{
		return true;
	}

	FObjectPoolConfig Config;
	Config.PoolID = PoolID;
	Config.ObjectClass = ObjectClass;
	Config.InitialSize = InitialSize;
	Config.MaxSize = MaxSize;
	Config.GrowthStep = FMath::Max(1, InitialSize / 5);
	Config.bAllowGrowth = true;
	Config.bAllowShrink = true;
	Config.bWarmupOnRegister = true;

	return Subsystem->RegisterPool(Config);
}

UObject* UObjectPoolLibrary::QuickAcquire(const UObject* WorldContextObject, FName PoolID)
{
	UObjectPoolSubsystem* Subsystem = GetObjectPoolSubsystem(WorldContextObject);
	if (!Subsystem)
	{
		return nullptr;
	}

	return Subsystem->AcquireObject(PoolID);
}

AActor* UObjectPoolLibrary::QuickAcquireActor(const UObject* WorldContextObject, FName PoolID)
{
	return Cast<AActor>(QuickAcquire(WorldContextObject, PoolID));
}

bool UObjectPoolLibrary::QuickRelease(const UObject* WorldContextObject, UObject* Object)
{
	UObjectPoolSubsystem* Subsystem = GetObjectPoolSubsystem(WorldContextObject);
	if (!Subsystem)
	{
		return false;
	}

	return Subsystem->ReleaseObject(Object);
}

float UObjectPoolLibrary::GetPoolUsageRate(const UObject* WorldContextObject, FName PoolID)
{
	UObjectPoolSubsystem* Subsystem = GetObjectPoolSubsystem(WorldContextObject);
	if (!Subsystem)
	{
		return -1.0f;
	}

	FObjectPoolStats Stats;
	if (Subsystem->GetPoolStats(PoolID, Stats))
	{
		return Stats.UsageRate;
	}

	return -1.0f;
}

bool UObjectPoolLibrary::IsPoolHealthy(const UObject* WorldContextObject, FName PoolID, float WarningThreshold)
{
	const float UsageRate = GetPoolUsageRate(WorldContextObject, PoolID);
	if (UsageRate < 0.0f)
	{
		return false;
	}

	return UsageRate < WarningThreshold;
}

int32 UObjectPoolLibrary::GetPoolAvailableCount(const UObject* WorldContextObject, FName PoolID)
{
	UObjectPoolSubsystem* Subsystem = GetObjectPoolSubsystem(WorldContextObject);
	if (!Subsystem)
	{
		return -1;
	}

	FObjectPoolStats Stats;
	if (Subsystem->GetPoolStats(PoolID, Stats))
	{
		return Stats.AvailableCount;
	}

	return -1;
}
