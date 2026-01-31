// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "YcProjectileBase.h"
#include "YcProjectilePoolSubsystem.generated.h"

/**
 * 单个类型的对象池配置
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcProjectilePoolConfig
{
	GENERATED_BODY()

	/** 池标识符 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
	FName PoolID;

	/** 子弹类 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
	TSubclassOf<AYcProjectileBase> ProjectileClass;

	/** 初始池大小 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool", meta = (ClampMin = "1", ClampMax = "1000"))
	int32 InitialPoolSize = 50;

	/** 最大池大小 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool", meta = (ClampMin = "1", ClampMax = "5000"))
	int32 MaxPoolSize = 200;

	/** 扩容步长 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool", meta = (ClampMin = "1", ClampMax = "100"))
	int32 GrowthStep = 10;

	/** 是否允许动态扩容 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool")
	bool bAllowGrowth = true;

	/** 收缩阈值（空闲比例超过此值时收缩） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool", meta = (ClampMin = "0.5", ClampMax = "0.95"))
	float ShrinkThreshold = 0.8f;

	/** 收缩检查间隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pool", meta = (ClampMin = "1.0"))
	float ShrinkCheckInterval = 30.0f;
};

/**
 * 单个池的运行时数据
 */
USTRUCT()
struct FYcProjectilePoolData
{
	GENERATED_BODY()

	/** 池配置 */
	FYcProjectilePoolConfig Config;

	/** 所有池中的子弹（包括活跃和休眠的） */
	UPROPERTY()
	TArray<TObjectPtr<AYcProjectileBase>> AllProjectiles;

	/** 可用子弹索引队列 */
	TArray<int32> AvailableIndices;

	/** 当前活跃数量 */
	int32 ActiveCount = 0;

	/** 峰值活跃数量（用于统计） */
	int32 PeakActiveCount = 0;

	/** 总获取次数 */
	int64 TotalAcquireCount = 0;

	/** 获取失败次数（池耗尽） */
	int64 AcquireFailCount = 0;

	/** 上次收缩检查时间 */
	float LastShrinkCheckTime = 0.0f;

	/** 获取可用数量 */
	int32 GetAvailableCount() const { return AvailableIndices.Num(); }

	/** 获取总数量 */
	int32 GetTotalCount() const { return AllProjectiles.Num(); }

	/** 获取使用率 */
	float GetUsageRate() const 
	{ 
		return AllProjectiles.Num() > 0 ? static_cast<float>(ActiveCount) / AllProjectiles.Num() : 0.0f; 
	}
};

/**
 * 池统计信息
 */
USTRUCT(BlueprintType)
struct YICHENSHOOTERCORE_API FYcProjectilePoolStats
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	FName PoolID;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 TotalCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 ActiveCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 AvailableCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int32 PeakActiveCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int64 TotalAcquireCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	int64 AcquireFailCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Stats")
	float UsageRate = 0.0f;
};

/**
 * 子弹对象池子系统
 * 
 * 商业级FPS游戏的子弹对象池管理系统，特性：
 * - 多类型子弹池支持
 * - 动态扩容/收缩
 * - 网络游戏优化
 * - 性能监控和统计
 * - 预热机制
 */
UCLASS()
class YICHENSHOOTERCORE_API UYcProjectilePoolSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin USubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	//~ End USubsystem Interface

	/**
	 * 注册一个子弹池
	 * @param Config 池配置
	 * @return 是否注册成功
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectilePool")
	bool RegisterPool(const FYcProjectilePoolConfig& Config);

	/**
	 * 注销一个子弹池
	 * @param PoolID 池标识符
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectilePool")
	void UnregisterPool(FName PoolID);

	/**
	 * 从池中获取一个子弹并激活
	 * @param PoolID 池标识符
	 * @param Params 初始化参数
	 * @return 激活的子弹，如果池耗尽则返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectilePool")
	AYcProjectileBase* AcquireProjectile(FName PoolID, const FYcProjectileInitParams& Params);

	/**
	 * 将子弹返回池中
	 * @param Projectile 要返回的子弹
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectilePool")
	void ReleaseProjectile(AYcProjectileBase* Projectile);

	/**
	 * 预热指定池（预先创建对象）
	 * @param PoolID 池标识符
	 * @param Count 预热数量，-1表示使用配置的InitialPoolSize
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectilePool")
	void WarmupPool(FName PoolID, int32 Count = -1);

	/**
	 * 预热所有已注册的池
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectilePool")
	void WarmupAllPools();

	/**
	 * 获取池统计信息
	 * @param PoolID 池标识符
	 * @param OutStats 输出统计信息
	 * @return 是否找到该池
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectilePool|Stats")
	bool GetPoolStats(FName PoolID, FYcProjectilePoolStats& OutStats) const;

	/**
	 * 获取所有池的统计信息
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectilePool|Stats")
	TArray<FYcProjectilePoolStats> GetAllPoolStats() const;

	/**
	 * 打印池状态到日志
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectilePool|Debug")
	void PrintPoolStatus() const;

	/**
	 * 强制收缩所有池
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectilePool")
	void ForceShrinkAllPools();

	/**
	 * 清空所有池
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectilePool")
	void ClearAllPools();

	/** 检查池是否存在 */
	UFUNCTION(BlueprintPure, Category = "ProjectilePool")
	bool HasPool(FName PoolID) const { return Pools.Contains(PoolID); }

protected:
	/** 创建单个子弹实例 */
	AYcProjectileBase* CreateProjectileInstance(const FYcProjectilePoolConfig& Config);

	/** 扩容池 */
	void GrowPool(FName PoolID, int32 GrowthAmount);

	/** 收缩池 */
	void ShrinkPool(FName PoolID);

	/** 定时检查收缩 */
	void CheckPoolShrinkage();

	/** 子弹回收回调 */
	UFUNCTION()
	void OnProjectileRecycled(AYcProjectileBase* Projectile);

private:
	/** 所有池数据 */
	UPROPERTY()
	TMap<FName, FYcProjectilePoolData> Pools;

	/** 子弹到池ID的映射（用于快速查找） */
	UPROPERTY()
	TMap<TObjectPtr<AYcProjectileBase>, FName> ProjectileToPoolMap;

	/** 收缩检查定时器 */
	FTimerHandle ShrinkCheckTimerHandle;

	/** 是否已初始化 */
	bool bIsInitialized = false;

	/** 日志类别 */
	static constexpr const TCHAR* LogCategory = TEXT("ProjectilePool");
};
