// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "ObjectPoolSubsystem.h"
#include "ObjectPoolContainer.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ObjectPoolSubsystem)

void UObjectPoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 启动收缩检查定时器
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ShrinkCheckTimerHandle,
			this,
			&UObjectPoolSubsystem::CheckPoolsShrinkage,
			10.0f,
			true
		);
	}

	UE_LOG(LogObjectPool, Log, TEXT("ObjectPoolSubsystem initialized"));
}

void UObjectPoolSubsystem::Deinitialize()
{
	// 清理定时器
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ShrinkCheckTimerHandle);
	}

	// 清理所有池
	ClearAllPools();

	UE_LOG(LogObjectPool, Log, TEXT("ObjectPoolSubsystem deinitialized"));

	Super::Deinitialize();
}

bool UObjectPoolSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	if (const UWorld* World = Cast<UWorld>(Outer))
	{
		return World->IsGameWorld();
	}
	return false;
}

bool UObjectPoolSubsystem::RegisterPool(const FObjectPoolConfig& Config)
{
	FScopeLock Lock(&SubsystemLock);

	if (!Config.IsValid())
	{
		UE_LOG(LogObjectPool, Warning, TEXT("Invalid pool config"));
		return false;
	}

	if (Pools.Contains(Config.PoolID))
	{
		UE_LOG(LogObjectPool, Warning, TEXT("Pool [%s] already exists"), *Config.PoolID.ToString());
		return false;
	}

	TUniquePtr<FObjectPoolContainer> NewPool = MakeUnique<FObjectPoolContainer>();
	if (!NewPool->Initialize(GetWorld(), Config))
	{
		return false;
	}

	Pools.Add(Config.PoolID, MoveTemp(NewPool));

	UE_LOG(LogObjectPool, Log, TEXT("Registered pool [%s]"), *Config.PoolID.ToString());
	return true;
}

void UObjectPoolSubsystem::UnregisterPool(FName PoolID)
{
	FScopeLock Lock(&SubsystemLock);

	TUniquePtr<FObjectPoolContainer>* PoolPtr = Pools.Find(PoolID);
	if (!PoolPtr)
	{
		return;
	}

	(*PoolPtr)->Shutdown();
	Pools.Remove(PoolID);

	// 清理映射
	for (auto It = ObjectToPoolMap.CreateIterator(); It; ++It)
	{
		if (It.Value() == PoolID)
		{
			It.RemoveCurrent();
		}
	}

	BroadcastEvent(PoolID, EObjectPoolEventType::Cleared, nullptr);
	UE_LOG(LogObjectPool, Log, TEXT("Unregistered pool [%s]"), *PoolID.ToString());
}

bool UObjectPoolSubsystem::HasPool(FName PoolID) const
{
	FScopeLock Lock(&SubsystemLock);
	return Pools.Contains(PoolID);
}


UObject* UObjectPoolSubsystem::AcquireObject(FName PoolID)
{
	FScopeLock Lock(&SubsystemLock);

	TUniquePtr<FObjectPoolContainer>* PoolPtr = Pools.Find(PoolID);
	if (!PoolPtr || !PoolPtr->IsValid())
	{
		UE_LOG(LogObjectPool, Warning, TEXT("Pool [%s] not found"), *PoolID.ToString());
		return nullptr;
	}

	UObject* Object = (*PoolPtr)->Acquire();
	if (Object)
	{
		ObjectToPoolMap.Add(Object, PoolID);
		BroadcastEvent(PoolID, EObjectPoolEventType::Acquired, Object);
	}
	else
	{
		BroadcastEvent(PoolID, EObjectPoolEventType::Exhausted, nullptr);
	}

	return Object;
}

AActor* UObjectPoolSubsystem::AcquireActor(FName PoolID)
{
	return Cast<AActor>(AcquireObject(PoolID));
}

bool UObjectPoolSubsystem::ReleaseObject(UObject* Object)
{
	if (!IsValid(Object))
	{
		return false;
	}

	FScopeLock Lock(&SubsystemLock);

	const FName* PoolIDPtr = ObjectToPoolMap.Find(Object);
	if (!PoolIDPtr)
	{
		UE_LOG(LogObjectPool, Warning, TEXT("Object [%s] not managed by any pool"), *Object->GetName());
		return false;
	}

	const FName PoolID = *PoolIDPtr;
	TUniquePtr<FObjectPoolContainer>* PoolPtr = Pools.Find(PoolID);
	if (!PoolPtr || !PoolPtr->IsValid())
	{
		return false;
	}

	if ((*PoolPtr)->Release(Object))
	{
		BroadcastEvent(PoolID, EObjectPoolEventType::Released, Object);
		return true;
	}

	return false;
}

void UObjectPoolSubsystem::WarmupPool(FName PoolID, int32 Count)
{
	FScopeLock Lock(&SubsystemLock);

	TUniquePtr<FObjectPoolContainer>* PoolPtr = Pools.Find(PoolID);
	if (PoolPtr && PoolPtr->IsValid())
	{
		(*PoolPtr)->Warmup(Count);
	}
}

void UObjectPoolSubsystem::WarmupAllPools()
{
	FScopeLock Lock(&SubsystemLock);

	for (auto& Pair : Pools)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->Warmup();
		}
	}
}

bool UObjectPoolSubsystem::GetPoolStats(FName PoolID, FObjectPoolStats& OutStats) const
{
	FScopeLock Lock(&SubsystemLock);

	const TUniquePtr<FObjectPoolContainer>* PoolPtr = Pools.Find(PoolID);
	if (!PoolPtr || !PoolPtr->IsValid())
	{
		return false;
	}

	OutStats = (*PoolPtr)->GetStats();
	return true;
}

TArray<FObjectPoolStats> UObjectPoolSubsystem::GetAllPoolStats() const
{
	FScopeLock Lock(&SubsystemLock);

	TArray<FObjectPoolStats> AllStats;
	AllStats.Reserve(Pools.Num());

	for (const auto& Pair : Pools)
	{
		if (Pair.Value.IsValid())
		{
			AllStats.Add(Pair.Value->GetStats());
		}
	}

	return AllStats;
}

void UObjectPoolSubsystem::PrintAllPoolStats() const
{
	TArray<FObjectPoolStats> AllStats = GetAllPoolStats();

	UE_LOG(LogObjectPool, Log, TEXT("========== Object Pool Status =========="));
	UE_LOG(LogObjectPool, Log, TEXT("Total Pools: %d"), AllStats.Num());

	for (const FObjectPoolStats& Stats : AllStats)
	{
		UE_LOG(LogObjectPool, Log, TEXT("-------------------------------------------"));
		UE_LOG(LogObjectPool, Log, TEXT("Pool: %s (%s)"), *Stats.PoolID.ToString(), Stats.bIsActorPool ? TEXT("Actor") : TEXT("UObject"));
		UE_LOG(LogObjectPool, Log, TEXT("  Class: %s"), *Stats.ClassName);
		UE_LOG(LogObjectPool, Log, TEXT("  Total: %d, Active: %d, Available: %d"), Stats.TotalCount, Stats.ActiveCount, Stats.AvailableCount);
		UE_LOG(LogObjectPool, Log, TEXT("  Peak: %d, Usage: %.1f%%"), Stats.PeakActiveCount, Stats.UsageRate * 100.0f);
		UE_LOG(LogObjectPool, Log, TEXT("  Acquire: %lld total, %lld failed"), Stats.TotalAcquireCount, Stats.AcquireFailCount);
		UE_LOG(LogObjectPool, Log, TEXT("  Growth: %d, Shrink: %d"), Stats.GrowthCount, Stats.ShrinkCount);
	}

	UE_LOG(LogObjectPool, Log, TEXT("========================================="));
}

void UObjectPoolSubsystem::ShrinkAllPools()
{
	FScopeLock Lock(&SubsystemLock);

	for (auto& Pair : Pools)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->Shrink();
			BroadcastEvent(Pair.Key, EObjectPoolEventType::Shrunk, nullptr);
		}
	}
}

void UObjectPoolSubsystem::ClearAllPools()
{
	FScopeLock Lock(&SubsystemLock);

	for (auto& Pair : Pools)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->Shutdown();
			BroadcastEvent(Pair.Key, EObjectPoolEventType::Cleared, nullptr);
		}
	}

	Pools.Empty();
	ObjectToPoolMap.Empty();
}

void UObjectPoolSubsystem::CheckPoolsShrinkage()
{
	FScopeLock Lock(&SubsystemLock);

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	for (auto& Pair : Pools)
	{
		if (Pair.Value.IsValid())
		{
			Pair.Value->CheckShrinkage(CurrentTime);
		}
	}
}

void UObjectPoolSubsystem::BroadcastEvent(FName PoolID, EObjectPoolEventType EventType, UObject* Object)
{
	if (OnPoolEvent.IsBound())
	{
		OnPoolEvent.Broadcast(PoolID, EventType, Object);
	}
}
