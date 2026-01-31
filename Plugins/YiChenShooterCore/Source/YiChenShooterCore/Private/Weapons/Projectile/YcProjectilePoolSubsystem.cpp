// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Weapons/Projectile/YcProjectilePoolSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcProjectilePoolSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogProjectilePool, Log, All);

void UYcProjectilePoolSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	bIsInitialized = true;

	// 启动收缩检查定时器
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ShrinkCheckTimerHandle,
			this,
			&UYcProjectilePoolSubsystem::CheckPoolShrinkage,
			10.0f, // 每10秒检查一次
			true
		);
	}

	UE_LOG(LogProjectilePool, Log, TEXT("ProjectilePoolSubsystem initialized"));
}

void UYcProjectilePoolSubsystem::Deinitialize()
{
	// 清理定时器
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ShrinkCheckTimerHandle);
	}

	// 清理所有池
	ClearAllPools();

	bIsInitialized = false;

	UE_LOG(LogProjectilePool, Log, TEXT("ProjectilePoolSubsystem deinitialized"));

	Super::Deinitialize();
}

bool UYcProjectilePoolSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	// 只在游戏世界中创建
	if (const UWorld* World = Cast<UWorld>(Outer))
	{
		return World->IsGameWorld();
	}
	return false;
}

bool UYcProjectilePoolSubsystem::RegisterPool(const FYcProjectilePoolConfig& Config)
{
	if (Config.PoolID.IsNone())
	{
		UE_LOG(LogProjectilePool, Warning, TEXT("Cannot register pool with empty PoolID"));
		return false;
	}

	if (!Config.ProjectileClass)
	{
		UE_LOG(LogProjectilePool, Warning, TEXT("Cannot register pool [%s] with null ProjectileClass"), *Config.PoolID.ToString());
		return false;
	}

	if (Pools.Contains(Config.PoolID))
	{
		UE_LOG(LogProjectilePool, Warning, TEXT("Pool [%s] already registered"), *Config.PoolID.ToString());
		return false;
	}

	// 创建池数据
	FYcProjectilePoolData& PoolData = Pools.Add(Config.PoolID);
	PoolData.Config = Config;
	PoolData.AllProjectiles.Reserve(Config.MaxPoolSize);
	PoolData.AvailableIndices.Reserve(Config.InitialPoolSize);

	UE_LOG(LogProjectilePool, Log, TEXT("Registered pool [%s] with class [%s], initial size: %d, max size: %d"),
		*Config.PoolID.ToString(),
		*Config.ProjectileClass->GetName(),
		Config.InitialPoolSize,
		Config.MaxPoolSize);

	return true;
}

void UYcProjectilePoolSubsystem::UnregisterPool(FName PoolID)
{
	FYcProjectilePoolData* PoolData = Pools.Find(PoolID);
	if (!PoolData)
	{
		return;
	}

	// 销毁所有子弹
	for (AYcProjectileBase* Projectile : PoolData->AllProjectiles)
	{
		if (IsValid(Projectile))
		{
			ProjectileToPoolMap.Remove(Projectile);
			Projectile->Destroy();
		}
	}

	Pools.Remove(PoolID);

	UE_LOG(LogProjectilePool, Log, TEXT("Unregistered pool [%s]"), *PoolID.ToString());
}

AYcProjectileBase* UYcProjectilePoolSubsystem::AcquireProjectile(FName PoolID, const FYcProjectileInitParams& Params)
{
	FYcProjectilePoolData* PoolData = Pools.Find(PoolID);
	if (!PoolData)
	{
		UE_LOG(LogProjectilePool, Warning, TEXT("Pool [%s] not found"), *PoolID.ToString());
		return nullptr;
	}

	PoolData->TotalAcquireCount++;

	// 尝试从可用队列获取
	if (PoolData->AvailableIndices.Num() > 0)
	{
		const int32 Index = PoolData->AvailableIndices.Pop(EAllowShrinking::No);
		AYcProjectileBase* Projectile = PoolData->AllProjectiles[Index];

		if (IsValid(Projectile) && Projectile->IsAvailableInPool())
		{
			Projectile->ActivateFromPool(Params);
			PoolData->ActiveCount++;
			PoolData->PeakActiveCount = FMath::Max(PoolData->PeakActiveCount, PoolData->ActiveCount);
			return Projectile;
		}
	}

	// 池耗尽，尝试扩容
	if (PoolData->Config.bAllowGrowth && PoolData->AllProjectiles.Num() < PoolData->Config.MaxPoolSize)
	{
		const int32 GrowthAmount = FMath::Min(
			PoolData->Config.GrowthStep,
			PoolData->Config.MaxPoolSize - PoolData->AllProjectiles.Num()
		);

		GrowPool(PoolID, GrowthAmount);

		// 再次尝试获取
		if (PoolData->AvailableIndices.Num() > 0)
		{
			const int32 Index = PoolData->AvailableIndices.Pop(EAllowShrinking::No);
			AYcProjectileBase* Projectile = PoolData->AllProjectiles[Index];

			if (IsValid(Projectile) && Projectile->IsAvailableInPool())
			{
				Projectile->ActivateFromPool(Params);
				PoolData->ActiveCount++;
				PoolData->PeakActiveCount = FMath::Max(PoolData->PeakActiveCount, PoolData->ActiveCount);
				return Projectile;
			}
		}
	}

	// 获取失败
	PoolData->AcquireFailCount++;
	UE_LOG(LogProjectilePool, Warning, TEXT("Pool [%s] exhausted! Total: %d, Active: %d, MaxSize: %d"),
		*PoolID.ToString(),
		PoolData->AllProjectiles.Num(),
		PoolData->ActiveCount,
		PoolData->Config.MaxPoolSize);

	return nullptr;
}

void UYcProjectilePoolSubsystem::ReleaseProjectile(AYcProjectileBase* Projectile)
{
	if (!IsValid(Projectile))
	{
		return;
	}

	const FName* PoolIDPtr = ProjectileToPoolMap.Find(Projectile);
	if (!PoolIDPtr)
	{
		UE_LOG(LogProjectilePool, Warning, TEXT("Projectile [%s] not managed by pool"), *Projectile->GetName());
		return;
	}

	FYcProjectilePoolData* PoolData = Pools.Find(*PoolIDPtr);
	if (!PoolData)
	{
		return;
	}

	// 找到索引并添加到可用队列
	const int32 Index = PoolData->AllProjectiles.IndexOfByKey(Projectile);
	if (Index != INDEX_NONE && !PoolData->AvailableIndices.Contains(Index))
	{
		PoolData->AvailableIndices.Add(Index);
		PoolData->ActiveCount = FMath::Max(0, PoolData->ActiveCount - 1);
	}
}

void UYcProjectilePoolSubsystem::WarmupPool(FName PoolID, int32 Count)
{
	FYcProjectilePoolData* PoolData = Pools.Find(PoolID);
	if (!PoolData)
	{
		UE_LOG(LogProjectilePool, Warning, TEXT("Cannot warmup pool [%s]: not found"), *PoolID.ToString());
		return;
	}

	const int32 TargetCount = (Count < 0) ? PoolData->Config.InitialPoolSize : Count;
	const int32 CurrentCount = PoolData->AllProjectiles.Num();
	const int32 ToCreate = FMath::Min(TargetCount - CurrentCount, PoolData->Config.MaxPoolSize - CurrentCount);

	if (ToCreate <= 0)
	{
		return;
	}

	UE_LOG(LogProjectilePool, Log, TEXT("Warming up pool [%s]: creating %d projectiles"), *PoolID.ToString(), ToCreate);

	for (int32 i = 0; i < ToCreate; ++i)
	{
		AYcProjectileBase* Projectile = CreateProjectileInstance(PoolData->Config);
		if (Projectile)
		{
			const int32 Index = PoolData->AllProjectiles.Add(Projectile);
			PoolData->AvailableIndices.Add(Index);
			ProjectileToPoolMap.Add(Projectile, PoolID);

			// 绑定回收事件
			Projectile->OnProjectileRecycled.AddDynamic(this, &UYcProjectilePoolSubsystem::OnProjectileRecycled);
		}
	}

	UE_LOG(LogProjectilePool, Log, TEXT("Pool [%s] warmup complete: %d total projectiles"),
		*PoolID.ToString(), PoolData->AllProjectiles.Num());
}

void UYcProjectilePoolSubsystem::WarmupAllPools()
{
	for (auto& Pair : Pools)
	{
		WarmupPool(Pair.Key);
	}
}

bool UYcProjectilePoolSubsystem::GetPoolStats(FName PoolID, FYcProjectilePoolStats& OutStats) const
{
	const FYcProjectilePoolData* PoolData = Pools.Find(PoolID);
	if (!PoolData)
	{
		return false;
	}

	OutStats.PoolID = PoolID;
	OutStats.TotalCount = PoolData->GetTotalCount();
	OutStats.ActiveCount = PoolData->ActiveCount;
	OutStats.AvailableCount = PoolData->GetAvailableCount();
	OutStats.PeakActiveCount = PoolData->PeakActiveCount;
	OutStats.TotalAcquireCount = PoolData->TotalAcquireCount;
	OutStats.AcquireFailCount = PoolData->AcquireFailCount;
	OutStats.UsageRate = PoolData->GetUsageRate();

	return true;
}

TArray<FYcProjectilePoolStats> UYcProjectilePoolSubsystem::GetAllPoolStats() const
{
	TArray<FYcProjectilePoolStats> AllStats;
	AllStats.Reserve(Pools.Num());

	for (const auto& Pair : Pools)
	{
		FYcProjectilePoolStats Stats;
		if (GetPoolStats(Pair.Key, Stats))
		{
			AllStats.Add(Stats);
		}
	}

	return AllStats;
}

void UYcProjectilePoolSubsystem::PrintPoolStatus() const
{
	UE_LOG(LogProjectilePool, Log, TEXT("========== Projectile Pool Status =========="));
	UE_LOG(LogProjectilePool, Log, TEXT("Total Pools: %d"), Pools.Num());

	for (const auto& Pair : Pools)
	{
		const FYcProjectilePoolData& PoolData = Pair.Value;
		UE_LOG(LogProjectilePool, Log, TEXT("-------------------------------------------"));
		UE_LOG(LogProjectilePool, Log, TEXT("Pool: %s"), *Pair.Key.ToString());
		UE_LOG(LogProjectilePool, Log, TEXT("  Class: %s"), *PoolData.Config.ProjectileClass->GetName());
		UE_LOG(LogProjectilePool, Log, TEXT("  Total: %d / %d (Max)"), PoolData.GetTotalCount(), PoolData.Config.MaxPoolSize);
		UE_LOG(LogProjectilePool, Log, TEXT("  Active: %d, Available: %d"), PoolData.ActiveCount, PoolData.GetAvailableCount());
		UE_LOG(LogProjectilePool, Log, TEXT("  Peak Active: %d"), PoolData.PeakActiveCount);
		UE_LOG(LogProjectilePool, Log, TEXT("  Usage Rate: %.1f%%"), PoolData.GetUsageRate() * 100.0f);
		UE_LOG(LogProjectilePool, Log, TEXT("  Acquire: %lld total, %lld failed"), PoolData.TotalAcquireCount, PoolData.AcquireFailCount);
	}

	UE_LOG(LogProjectilePool, Log, TEXT("============================================"));
}

void UYcProjectilePoolSubsystem::ForceShrinkAllPools()
{
	for (auto& Pair : Pools)
	{
		ShrinkPool(Pair.Key);
	}
}

void UYcProjectilePoolSubsystem::ClearAllPools()
{
	TArray<FName> PoolIDs;
	Pools.GetKeys(PoolIDs);

	for (const FName& PoolID : PoolIDs)
	{
		UnregisterPool(PoolID);
	}
}

AYcProjectileBase* UYcProjectilePoolSubsystem::CreateProjectileInstance(const FYcProjectilePoolConfig& Config)
{
	UWorld* World = GetWorld();
	if (!World || !Config.ProjectileClass)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	SpawnParams.ObjectFlags |= RF_Transient;

	AYcProjectileBase* Projectile = World->SpawnActor<AYcProjectileBase>(
		Config.ProjectileClass,
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (Projectile)
	{
		Projectile->SetPoolID(Config.PoolID);
	}

	return Projectile;
}

void UYcProjectilePoolSubsystem::GrowPool(FName PoolID, int32 GrowthAmount)
{
	FYcProjectilePoolData* PoolData = Pools.Find(PoolID);
	if (!PoolData || GrowthAmount <= 0)
	{
		return;
	}

	const int32 ActualGrowth = FMath::Min(
		GrowthAmount,
		PoolData->Config.MaxPoolSize - PoolData->AllProjectiles.Num()
	);

	if (ActualGrowth <= 0)
	{
		return;
	}

	UE_LOG(LogProjectilePool, Log, TEXT("Growing pool [%s] by %d"), *PoolID.ToString(), ActualGrowth);

	for (int32 i = 0; i < ActualGrowth; ++i)
	{
		AYcProjectileBase* Projectile = CreateProjectileInstance(PoolData->Config);
		if (Projectile)
		{
			const int32 Index = PoolData->AllProjectiles.Add(Projectile);
			PoolData->AvailableIndices.Add(Index);
			ProjectileToPoolMap.Add(Projectile, PoolID);

			Projectile->OnProjectileRecycled.AddDynamic(this, &UYcProjectilePoolSubsystem::OnProjectileRecycled);
		}
	}
}

void UYcProjectilePoolSubsystem::ShrinkPool(FName PoolID)
{
	FYcProjectilePoolData* PoolData = Pools.Find(PoolID);
	if (!PoolData)
	{
		return;
	}

	// 计算空闲比例
	const float IdleRatio = PoolData->GetTotalCount() > 0 
		? static_cast<float>(PoolData->GetAvailableCount()) / PoolData->GetTotalCount() 
		: 0.0f;

	// 如果空闲比例未超过阈值，不收缩
	if (IdleRatio < PoolData->Config.ShrinkThreshold)
	{
		return;
	}

	// 保留至少InitialPoolSize个对象
	const int32 TargetCount = FMath::Max(
		PoolData->Config.InitialPoolSize,
		PoolData->ActiveCount + PoolData->Config.GrowthStep
	);

	const int32 ToRemove = PoolData->AllProjectiles.Num() - TargetCount;
	if (ToRemove <= 0)
	{
		return;
	}

	UE_LOG(LogProjectilePool, Log, TEXT("Shrinking pool [%s]: removing %d idle projectiles"), *PoolID.ToString(), ToRemove);

	int32 Removed = 0;
	for (int32 i = PoolData->AvailableIndices.Num() - 1; i >= 0 && Removed < ToRemove; --i)
	{
		const int32 Index = PoolData->AvailableIndices[i];
		AYcProjectileBase* Projectile = PoolData->AllProjectiles[Index];

		if (IsValid(Projectile) && Projectile->IsAvailableInPool())
		{
			ProjectileToPoolMap.Remove(Projectile);
			Projectile->Destroy();
			PoolData->AllProjectiles[Index] = nullptr;
			PoolData->AvailableIndices.RemoveAt(i);
			Removed++;
		}
	}

	// 压缩数组，移除nullptr
	PoolData->AllProjectiles.RemoveAll([](const TObjectPtr<AYcProjectileBase>& P) { return P == nullptr; });

	// 重建可用索引
	PoolData->AvailableIndices.Reset();
	for (int32 i = 0; i < PoolData->AllProjectiles.Num(); ++i)
	{
		if (IsValid(PoolData->AllProjectiles[i]) && PoolData->AllProjectiles[i]->IsAvailableInPool())
		{
			PoolData->AvailableIndices.Add(i);
		}
	}

	UE_LOG(LogProjectilePool, Log, TEXT("Pool [%s] shrunk: %d total, %d available"),
		*PoolID.ToString(), PoolData->AllProjectiles.Num(), PoolData->AvailableIndices.Num());
}

void UYcProjectilePoolSubsystem::CheckPoolShrinkage()
{
	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

	for (auto& Pair : Pools)
	{
		FYcProjectilePoolData& PoolData = Pair.Value;

		if (CurrentTime - PoolData.LastShrinkCheckTime >= PoolData.Config.ShrinkCheckInterval)
		{
			PoolData.LastShrinkCheckTime = CurrentTime;
			ShrinkPool(Pair.Key);
		}
	}
}

void UYcProjectilePoolSubsystem::OnProjectileRecycled(AYcProjectileBase* Projectile)
{
	ReleaseProjectile(Projectile);
}
