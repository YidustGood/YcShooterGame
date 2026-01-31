// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "ObjectPoolContainer.h"
#include "Engine/World.h"

DEFINE_LOG_CATEGORY(LogObjectPool);

FObjectPoolContainer::FObjectPoolContainer()
{
}

FObjectPoolContainer::~FObjectPoolContainer()
{
	Shutdown();
}

bool FObjectPoolContainer::Initialize(UWorld* InWorld, const FObjectPoolConfig& InConfig)
{
	FScopeLock Lock(&PoolLock);

	if (bIsInitialized)
	{
		UE_LOG(LogObjectPool, Warning, TEXT("Pool [%s] already initialized"), *InConfig.PoolID.ToString());
		return false;
	}

	if (!InConfig.IsValid())
	{
		UE_LOG(LogObjectPool, Error, TEXT("Invalid pool config for [%s]"), *InConfig.PoolID.ToString());
		return false;
	}

	if (!InWorld)
	{
		UE_LOG(LogObjectPool, Error, TEXT("World is null for pool [%s]"), *InConfig.PoolID.ToString());
		return false;
	}

	World = InWorld;
	Config = InConfig;

	// 检测是否为Actor类
	bIsActorPool = Config.bIsActorPool || Config.ObjectClass->IsChildOf(AActor::StaticClass());

	// 预分配内存
	const int32 ReserveSize = Config.MaxSize > 0 ? Config.MaxSize : Config.InitialSize * 2;
	AllObjects.Reserve(ReserveSize);
	AvailableIndices.Reserve(ReserveSize);

	bIsInitialized = true;

	UE_LOG(LogObjectPool, Log, TEXT("Pool [%s] initialized (Class: %s, IsActor: %s)"),
		*Config.PoolID.ToString(),
		*Config.ObjectClass->GetName(),
		bIsActorPool ? TEXT("Yes") : TEXT("No"));

	// 预热
	if (Config.bWarmupOnRegister && Config.InitialSize > 0)
	{
		Warmup(Config.InitialSize);
	}

	return true;
}

void FObjectPoolContainer::Shutdown()
{
	FScopeLock Lock(&PoolLock);

	if (!bIsInitialized)
	{
		return;
	}

	UE_LOG(LogObjectPool, Log, TEXT("Shutting down pool [%s]"), *Config.PoolID.ToString());

	// 销毁所有对象
	for (const TWeakObjectPtr<UObject>& WeakObj : AllObjects)
	{
		if (UObject* Obj = WeakObj.Get())
		{
			// 通知对象即将被移除
			if (IPoolableObject* Poolable = Cast<IPoolableObject>(Obj))
			{
				Poolable->OnRemovedFromPool();
			}
			DestroyObject(Obj);
		}
	}

	AllObjects.Empty();
	AvailableIndices.Empty();
	ObjectToIndexMap.Empty();
	ActiveCount = 0;
	bIsInitialized = false;
}


void FObjectPoolContainer::Warmup(int32 Count)
{
	FScopeLock Lock(&PoolLock);

	if (!bIsInitialized)
	{
		return;
	}

	const int32 TargetCount = (Count < 0) ? Config.InitialSize : Count;
	const int32 CurrentCount = AllObjects.Num();
	int32 ToCreate = TargetCount - CurrentCount;

	if (Config.MaxSize > 0)
	{
		ToCreate = FMath::Min(ToCreate, Config.MaxSize - CurrentCount);
	}

	if (ToCreate <= 0)
	{
		return;
	}

	UE_LOG(LogObjectPool, Log, TEXT("Warming up pool [%s]: creating %d objects"), *Config.PoolID.ToString(), ToCreate);

	for (int32 i = 0; i < ToCreate; ++i)
	{
		UObject* NewObj = CreateObject();
		if (NewObj)
		{
			const int32 Index = AllObjects.Add(NewObj);
			AvailableIndices.Add(Index);
			ObjectToIndexMap.Add(NewObj, Index);
			DeactivateObject(NewObj);
		}
	}
}

UObject* FObjectPoolContainer::Acquire()
{
	FScopeLock Lock(&PoolLock);

	if (!bIsInitialized)
	{
		return nullptr;
	}

	TotalAcquireCount++;

	// 尝试从可用队列获取
	while (AvailableIndices.Num() > 0)
	{
		const int32 Index = AvailableIndices.Pop(EAllowShrinking::No);
		
		if (!AllObjects.IsValidIndex(Index))
		{
			continue;
		}

		UObject* Obj = AllObjects[Index].Get();
		if (!IsValid(Obj))
		{
			continue;
		}

		// 检查接口
		IPoolableObject* Poolable = Cast<IPoolableObject>(Obj);
		if (Poolable && !Poolable->CanBeAcquired())
		{
			continue;
		}

		// 激活对象
		ActivateObject(Obj);
		ActiveCount++;
		PeakActiveCount = FMath::Max(PeakActiveCount, ActiveCount);

		return Obj;
	}

	// 池耗尽，尝试扩容
	if (Config.bAllowGrowth)
	{
		const int32 CurrentCount = AllObjects.Num();
		if (Config.MaxSize == 0 || CurrentCount < Config.MaxSize)
		{
			const int32 GrowAmount = Config.MaxSize > 0 
				? FMath::Min(Config.GrowthStep, Config.MaxSize - CurrentCount)
				: Config.GrowthStep;

			if (GrowAmount > 0)
			{
				// 临时解锁以避免死锁
				PoolLock.Unlock();
				Grow(GrowAmount);
				PoolLock.Lock();

				// 再次尝试获取
				if (AvailableIndices.Num() > 0)
				{
					const int32 Index = AvailableIndices.Pop(EAllowShrinking::No);
					if (AllObjects.IsValidIndex(Index))
					{
						UObject* Obj = AllObjects[Index].Get();
						if (IsValid(Obj))
						{
							ActivateObject(Obj);
							ActiveCount++;
							PeakActiveCount = FMath::Max(PeakActiveCount, ActiveCount);
							return Obj;
						}
					}
				}
			}
		}
	}

	// 获取失败
	AcquireFailCount++;
	UE_LOG(LogObjectPool, Warning, TEXT("Pool [%s] exhausted! Total: %d, Active: %d, Max: %d"),
		*Config.PoolID.ToString(), AllObjects.Num(), ActiveCount, Config.MaxSize);

	return nullptr;
}

bool FObjectPoolContainer::Release(UObject* Object)
{
	FScopeLock Lock(&PoolLock);

	if (!bIsInitialized || !IsValid(Object))
	{
		return false;
	}

	const int32* IndexPtr = ObjectToIndexMap.Find(Object);
	if (!IndexPtr)
	{
		UE_LOG(LogObjectPool, Warning, TEXT("Object [%s] not managed by pool [%s]"),
			*Object->GetName(), *Config.PoolID.ToString());
		return false;
	}

	const int32 Index = *IndexPtr;

	// 检查是否已在可用队列中
	if (AvailableIndices.Contains(Index))
	{
		return false;
	}

	// 停用对象
	DeactivateObject(Object);

	// 添加到可用队列
	AvailableIndices.Add(Index);
	ActiveCount = FMath::Max(0, ActiveCount - 1);

	return true;
}

void FObjectPoolContainer::Grow(int32 Amount)
{
	FScopeLock Lock(&PoolLock);

	if (!bIsInitialized || Amount <= 0)
	{
		return;
	}

	int32 ActualAmount = Amount;
	if (Config.MaxSize > 0)
	{
		ActualAmount = FMath::Min(Amount, Config.MaxSize - AllObjects.Num());
	}

	if (ActualAmount <= 0)
	{
		return;
	}

	UE_LOG(LogObjectPool, Log, TEXT("Growing pool [%s] by %d"), *Config.PoolID.ToString(), ActualAmount);

	for (int32 i = 0; i < ActualAmount; ++i)
	{
		UObject* NewObj = CreateObject();
		if (NewObj)
		{
			const int32 Index = AllObjects.Add(NewObj);
			AvailableIndices.Add(Index);
			ObjectToIndexMap.Add(NewObj, Index);
			DeactivateObject(NewObj);
		}
	}

	GrowthCount++;
}

void FObjectPoolContainer::Shrink()
{
	FScopeLock Lock(&PoolLock);

	if (!bIsInitialized || !Config.bAllowShrink)
	{
		return;
	}

	const int32 TotalCount = AllObjects.Num();
	if (TotalCount == 0)
	{
		return;
	}

	const float IdleRatio = static_cast<float>(AvailableIndices.Num()) / TotalCount;
	if (IdleRatio < Config.ShrinkThreshold)
	{
		return;
	}

	// 保留至少InitialSize个对象
	const int32 TargetCount = FMath::Max(Config.InitialSize, ActiveCount + Config.GrowthStep);
	const int32 ToRemove = TotalCount - TargetCount;

	if (ToRemove <= 0)
	{
		return;
	}

	UE_LOG(LogObjectPool, Log, TEXT("Shrinking pool [%s]: removing %d objects"), *Config.PoolID.ToString(), ToRemove);

	int32 Removed = 0;
	for (int32 i = AvailableIndices.Num() - 1; i >= 0 && Removed < ToRemove; --i)
	{
		const int32 Index = AvailableIndices[i];
		if (!AllObjects.IsValidIndex(Index))
		{
			AvailableIndices.RemoveAt(i);
			continue;
		}

		UObject* Obj = AllObjects[Index].Get();
		if (!IsValid(Obj))
		{
			AvailableIndices.RemoveAt(i);
			AllObjects[Index].Reset();
			continue;
		}

		// 通知并销毁
		if (IPoolableObject* Poolable = Cast<IPoolableObject>(Obj))
		{
			Poolable->OnRemovedFromPool();
		}

		ObjectToIndexMap.Remove(Obj);
		DestroyObject(Obj);
		AllObjects[Index].Reset();
		AvailableIndices.RemoveAt(i);
		Removed++;
	}

	// 压缩数组（可选，会影响性能但减少内存）
	// AllObjects.RemoveAll([](const TWeakObjectPtr<UObject>& P) { return !P.IsValid(); });

	ShrinkCount++;
}

void FObjectPoolContainer::CheckShrinkage(float CurrentTime)
{
	if (!bIsInitialized || !Config.bAllowShrink)
	{
		return;
	}

	if (CurrentTime - LastShrinkCheckTime >= Config.ShrinkCheckInterval)
	{
		LastShrinkCheckTime = CurrentTime;
		Shrink();
	}
}


FObjectPoolStats FObjectPoolContainer::GetStats() const
{
	FScopeLock Lock(&PoolLock);

	FObjectPoolStats Stats;
	Stats.PoolID = Config.PoolID;
	Stats.ClassName = Config.ObjectClass ? Config.ObjectClass->GetName() : TEXT("None");
	Stats.TotalCount = AllObjects.Num();
	Stats.ActiveCount = ActiveCount;
	Stats.AvailableCount = AvailableIndices.Num();
	Stats.PeakActiveCount = PeakActiveCount;
	Stats.TotalAcquireCount = TotalAcquireCount;
	Stats.AcquireFailCount = AcquireFailCount;
	Stats.GrowthCount = GrowthCount;
	Stats.ShrinkCount = ShrinkCount;
	Stats.UsageRate = Stats.TotalCount > 0 ? static_cast<float>(ActiveCount) / Stats.TotalCount : 0.0f;
	Stats.bIsActorPool = bIsActorPool;

	return Stats;
}

UObject* FObjectPoolContainer::CreateObject()
{
	UWorld* WorldPtr = World.Get();
	if (!WorldPtr || !Config.ObjectClass)
	{
		return nullptr;
	}

	UObject* NewObj = nullptr;

	if (bIsActorPool)
	{
		// Actor需要通过SpawnActor创建
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.ObjectFlags |= RF_Transient;

		NewObj = WorldPtr->SpawnActor<AActor>(
			static_cast<TSubclassOf<AActor>>(Config.ObjectClass),
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			SpawnParams
		);
	}
	else
	{
		// 普通UObject使用NewObject
		NewObj = NewObject<UObject>(WorldPtr, Config.ObjectClass);
		if (NewObj)
		{
			NewObj->SetFlags(RF_Transient);
		}
	}

	return NewObj;
}

void FObjectPoolContainer::DestroyObject(UObject* Object)
{
	if (!IsValid(Object))
	{
		return;
	}

	if (bIsActorPool)
	{
		if (AActor* Actor = Cast<AActor>(Object))
		{
			Actor->Destroy();
		}
	}
	else
	{
		Object->MarkAsGarbage();
	}
}

void FObjectPoolContainer::ActivateObject(UObject* Object)
{
	if (!IsValid(Object))
	{
		return;
	}

	// 调用接口方法
	if (IPoolableObject* Poolable = Cast<IPoolableObject>(Object))
	{
		Poolable->SetPoolState(EPoolableObjectState::Active);
		Poolable->OnAcquiredFromPool();
	}

	// Actor特殊处理
	if (bIsActorPool)
	{
		if (AActor* Actor = Cast<AActor>(Object))
		{
			Actor->SetActorHiddenInGame(false);
			Actor->SetActorEnableCollision(true);
			Actor->SetActorTickEnabled(true);
		}
	}
}

void FObjectPoolContainer::DeactivateObject(UObject* Object)
{
	if (!IsValid(Object))
	{
		return;
	}

	// 调用接口方法
	if (IPoolableObject* Poolable = Cast<IPoolableObject>(Object))
	{
		Poolable->OnReleasedToPool();
		Poolable->ResetPooledObject();
		Poolable->SetPoolState(EPoolableObjectState::Pooled);
	}

	// Actor特殊处理
	if (bIsActorPool)
	{
		if (AActor* Actor = Cast<AActor>(Object))
		{
			Actor->SetActorHiddenInGame(true);
			Actor->SetActorEnableCollision(false);
			Actor->SetActorTickEnabled(false);
			Actor->SetActorLocation(FVector::ZeroVector);
		}
	}
}
