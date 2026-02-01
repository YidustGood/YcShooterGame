// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "IPoolableObject.h"
#include "ObjectPoolTypes.h"

DECLARE_LOG_CATEGORY_EXTERN(LogObjectPool, Log, All);

/**
 * 单个对象池容器（非UCLASS，纯C++实现以获得最佳性能）
 * 
 * 管理单一类型对象的池化。支持UObject和AActor。
 * 使用索引队列而非指针队列，减少内存碎片。
 */
class YICHENOBJECTPOOL_API FObjectPoolContainer
{
public:
	FObjectPoolContainer();
	~FObjectPoolContainer();

	/** 初始化池 */
	bool Initialize(UWorld* InWorld, const FObjectPoolConfig& InConfig);

	/** 关闭池，销毁所有对象 */
	void Shutdown();

	/** 预热池 */
	void Warmup(int32 Count = -1);

	/** 获取对象 */
	UObject* Acquire();

	/** 释放对象 */
	bool Release(UObject* Object);

	/** 扩容 */
	void Grow(int32 Amount);

	/** 收缩 */
	void Shrink();

	/** 检查是否需要收缩 */
	void CheckShrinkage(float CurrentTime);

	/** 获取统计信息 */
	FObjectPoolStats GetStats() const;

	/** 获取配置 */
	const FObjectPoolConfig& GetConfig() const { return Config; }

	/** 是否已初始化 */
	bool IsInitialized() const { return bIsInitialized; }

	/** 获取可用数量 */
	int32 GetAvailableCount() const { return AvailableIndices.Num(); }

	/** 获取总数量 */
	int32 GetTotalCount() const { return AllObjects.Num(); }

	/** 获取活跃数量 */
	int32 GetActiveCount() const { return ActiveCount; }

private:
	/** 创建单个对象 */
	UObject* CreateObject();

	/** 销毁单个对象 */
	void DestroyObject(UObject* Object);

	/** 激活对象 */
	void ActivateObject(UObject* Object);

	/** 停用对象 */
	void DeactivateObject(UObject* Object);

private:
	/** 池配置 */
	FObjectPoolConfig Config;

	/** 所属世界 */
	TWeakObjectPtr<UWorld> World;

	/** 所有对象 */
	TArray<TWeakObjectPtr<UObject>> AllObjects;

	/** 可用对象索引队列 */
	TArray<int32> AvailableIndices;

	/** 对象到索引的映射 */
	TMap<UObject*, int32> ObjectToIndexMap;

	/** 活跃对象数 */
	int32 ActiveCount = 0;

	/** 峰值活跃数 */
	int32 PeakActiveCount = 0;

	/** 总获取次数 */
	int64 TotalAcquireCount = 0;

	/** 获取失败次数 */
	int64 AcquireFailCount = 0;

	/** 扩容次数 */
	int32 GrowthCount = 0;

	/** 收缩次数 */
	int32 ShrinkCount = 0;

	/** 上次收缩检查时间 */
	float LastShrinkCheckTime = 0.0f;

	/** 是否已初始化 */
	bool bIsInitialized = false;

	/** 是否为Actor池 */
	bool bIsActorPool = false;

	/** 临界区（线程安全） */
	mutable FCriticalSection PoolLock;
};
