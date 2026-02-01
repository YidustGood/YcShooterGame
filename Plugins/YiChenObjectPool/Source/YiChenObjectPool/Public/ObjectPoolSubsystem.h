// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ObjectPoolTypes.h"
#include "ObjectPoolSubsystem.generated.h"

class FObjectPoolContainer;

/**
 * 通用对象池子系统
 * 
 * 提供统一的对象池管理接口，支持：
 * - 多类型对象池
 * - UObject和AActor
 * - 动态扩容/收缩
 * - 性能监控
 * - 蓝图集成
 * 
 * 使用方式：
 * 1. 获取子系统: GetWorld()->GetSubsystem<UObjectPoolSubsystem>()
 * 2. 注册池: RegisterPool(Config)
 * 3. 获取对象: AcquireObject<T>(PoolID) 或 AcquireObject(PoolID)
 * 4. 释放对象: ReleaseObject(Object)
 */
UCLASS()
class YICHENOBJECTPOOL_API UObjectPoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	//~ End USubsystem Interface

	/**
	 * 注册对象池
	 * @param Config 池配置
	 * @return 是否成功
	 */
	UFUNCTION(BlueprintCallable, Category = "ObjectPool")
	bool RegisterPool(const FObjectPoolConfig& Config);

	/**
	 * 注销对象池
	 * @param PoolID 池标识符
	 */
	UFUNCTION(BlueprintCallable, Category = "ObjectPool")
	void UnregisterPool(FName PoolID);

	/**
	 * 检查池是否存在
	 */
	UFUNCTION(BlueprintPure, Category = "ObjectPool")
	bool HasPool(FName PoolID) const;

	/**
	 * 获取对象（蓝图版本）
	 * @param PoolID 池标识符
	 * @return 获取的对象，失败返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "ObjectPool", meta = (DeterminesOutputType = "ObjectClass"))
	UObject* AcquireObject(FName PoolID);

	/**
	 * 获取对象（模板版本，类型安全）
	 */
	template<typename T>
	T* AcquireObject(FName PoolID)
	{
		return Cast<T>(AcquireObject(PoolID));
	}

	/**
	 * 获取Actor（蓝图便捷版本）
	 */
	UFUNCTION(BlueprintCallable, Category = "ObjectPool", meta = (DeterminesOutputType = "ActorClass"))
	AActor* AcquireActor(FName PoolID);

	/**
	 * 释放对象回池
	 * @param Object 要释放的对象
	 * @return 是否成功
	 */
	UFUNCTION(BlueprintCallable, Category = "ObjectPool")
	bool ReleaseObject(UObject* Object);

	/**
	 * 预热指定池
	 * @param PoolID 池标识符
	 * @param Count 预热数量，-1使用配置的InitialSize
	 */
	UFUNCTION(BlueprintCallable, Category = "ObjectPool")
	void WarmupPool(FName PoolID, int32 Count = -1);

	/**
	 * 预热所有池
	 */
	UFUNCTION(BlueprintCallable, Category = "ObjectPool")
	void WarmupAllPools();

	/**
	 * 获取池统计信息
	 */
	UFUNCTION(BlueprintCallable, Category = "ObjectPool|Stats")
	bool GetPoolStats(FName PoolID, FObjectPoolStats& OutStats) const;

	/**
	 * 获取所有池统计
	 */
	UFUNCTION(BlueprintCallable, Category = "ObjectPool|Stats")
	TArray<FObjectPoolStats> GetAllPoolStats() const;

	/**
	 * 打印所有池状态
	 */
	UFUNCTION(BlueprintCallable, Category = "ObjectPool|Debug")
	void PrintAllPoolStats() const;

	/**
	 * 强制收缩所有池
	 */
	UFUNCTION(BlueprintCallable, Category = "ObjectPool")
	void ShrinkAllPools();

	/**
	 * 清空所有池
	 */
	UFUNCTION(BlueprintCallable, Category = "ObjectPool")
	void ClearAllPools();

public:
	/** 池事件委托 */
	UPROPERTY(BlueprintAssignable, Category = "ObjectPool|Events")
	FOnObjectPoolEvent OnPoolEvent;

protected:
	/** 定时检查收缩 */
	void CheckPoolsShrinkage();

	/** 广播事件 */
	void BroadcastEvent(FName PoolID, EObjectPoolEventType EventType, UObject* Object = nullptr);

private:
	/** 所有池容器 */
	TMap<FName, TUniquePtr<FObjectPoolContainer>> Pools;

	/** 对象到池ID的映射 */
	TMap<TWeakObjectPtr<UObject>, FName> ObjectToPoolMap;

	/** 收缩检查定时器 */
	FTimerHandle ShrinkCheckTimerHandle;

	/** 临界区 */
	mutable FCriticalSection SubsystemLock;
};
