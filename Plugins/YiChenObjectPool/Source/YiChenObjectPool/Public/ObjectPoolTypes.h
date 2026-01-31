// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "ObjectPoolTypes.generated.h"

/**
 * 对象池配置
 */
USTRUCT(BlueprintType)
struct YICHENOBJECTPOOL_API FObjectPoolConfig
{
	GENERATED_BODY()

	/** 池标识符 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
	FName PoolID;

	/** 对象类 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
	TSubclassOf<UObject> ObjectClass;

	/** 初始池大小 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool", meta = (ClampMin = "0", ClampMax = "10000"))
	int32 InitialSize = 10;

	/** 最大池大小（0表示无限制） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool", meta = (ClampMin = "0", ClampMax = "100000"))
	int32 MaxSize = 100;

	/** 扩容步长 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool", meta = (ClampMin = "1", ClampMax = "100"))
	int32 GrowthStep = 5;

	/** 是否允许动态扩容 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
	bool bAllowGrowth = true;

	/** 是否允许动态收缩 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
	bool bAllowShrink = true;

	/** 收缩阈值（空闲比例超过此值时收缩） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool", meta = (ClampMin = "0.5", ClampMax = "0.99"))
	float ShrinkThreshold = 0.8f;

	/** 收缩检查间隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool", meta = (ClampMin = "1.0"))
	float ShrinkCheckInterval = 30.0f;

	/** 是否在注册时立即预热 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
	bool bWarmupOnRegister = true;

	/** 是否为Actor池（自动检测，但可手动指定） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
	bool bIsActorPool = false;

	FObjectPoolConfig()
	{
		PoolID = NAME_None;
		ObjectClass = nullptr;
	}

	bool IsValid() const
	{
		return !PoolID.IsNone() && ObjectClass != nullptr;
	}
};


/**
 * 对象池统计信息
 */
USTRUCT(BlueprintType)
struct YICHENOBJECTPOOL_API FObjectPoolStats
{
	GENERATED_BODY()

	/** 池标识符 */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FName PoolID;

	/** 对象类名 */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FString ClassName;

	/** 总对象数 */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 TotalCount = 0;

	/** 活跃对象数 */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 ActiveCount = 0;

	/** 可用对象数 */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 AvailableCount = 0;

	/** 峰值活跃数 */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 PeakActiveCount = 0;

	/** 总获取次数 */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int64 TotalAcquireCount = 0;

	/** 获取失败次数 */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int64 AcquireFailCount = 0;

	/** 扩容次数 */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 GrowthCount = 0;

	/** 收缩次数 */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 ShrinkCount = 0;

	/** 使用率 (0.0 - 1.0) */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float UsageRate = 0.0f;

	/** 是否为Actor池 */
	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	bool bIsActorPool = false;
};

/**
 * 对象池事件类型
 */
UENUM(BlueprintType)
enum class EObjectPoolEventType : uint8
{
	/** 对象被获取 */
	Acquired,
	/** 对象被释放 */
	Released,
	/** 池扩容 */
	Grown,
	/** 池收缩 */
	Shrunk,
	/** 池耗尽 */
	Exhausted,
	/** 池被清空 */
	Cleared
};

/** 对象池事件委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnObjectPoolEvent, FName, PoolID, EObjectPoolEventType, EventType, UObject*, Object);
