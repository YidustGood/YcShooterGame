// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YcProjectileBase.h"
#include "YcProjectilePoolSubsystem.h"
#include "YcProjectilePoolLibrary.generated.h"

/**
 * 子弹对象池蓝图函数库
 * 
 * 提供便捷的静态函数用于：
 * - 快速发射子弹
 * - 池管理
 * - 调试功能
 */
UCLASS()
class YICHENSHOOTERCORE_API UYcProjectilePoolLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 获取子弹池子系统
	 * @param WorldContextObject 世界上下文
	 * @return 子弹池子系统，如果不存在则返回nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "ProjectilePool", meta = (WorldContext = "WorldContextObject"))
	static UYcProjectilePoolSubsystem* GetProjectilePoolSubsystem(const UObject* WorldContextObject);

	/**
	 * 快速发射子弹
	 * @param WorldContextObject 世界上下文
	 * @param PoolID 池标识符
	 * @param SpawnLocation 发射位置
	 * @param Direction 发射方向
	 * @param Instigator 发射者
	 * @param Damage 伤害值
	 * @param Speed 初始速度
	 * @return 发射的子弹，如果失败则返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectilePool", meta = (WorldContext = "WorldContextObject"))
	static AYcProjectileBase* FireProjectile(
		const UObject* WorldContextObject,
		FName PoolID,
		FVector SpawnLocation,
		FVector Direction,
		AActor* Instigator,
		float Damage = 10.0f,
		float Speed = 10000.0f
	);

	/**
	 * 快速发射子弹（完整参数版本）
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectilePool", meta = (WorldContext = "WorldContextObject"))
	static AYcProjectileBase* FireProjectileWithParams(
		const UObject* WorldContextObject,
		FName PoolID,
		const FYcProjectileInitParams& Params
	);

	/**
	 * 快速注册并预热子弹池
	 * @param WorldContextObject 世界上下文
	 * @param PoolID 池标识符
	 * @param ProjectileClass 子弹类
	 * @param InitialSize 初始大小
	 * @param MaxSize 最大大小
	 * @return 是否成功
	 */
	UFUNCTION(BlueprintCallable, Category = "ProjectilePool", meta = (WorldContext = "WorldContextObject"))
	static bool QuickSetupPool(
		const UObject* WorldContextObject,
		FName PoolID,
		TSubclassOf<AYcProjectileBase> ProjectileClass,
		int32 InitialSize = 50,
		int32 MaxSize = 200
	);

	/**
	 * 获取池使用率
	 * @param WorldContextObject 世界上下文
	 * @param PoolID 池标识符
	 * @return 使用率 (0.0 - 1.0)，如果池不存在返回-1
	 */
	UFUNCTION(BlueprintPure, Category = "ProjectilePool|Stats", meta = (WorldContext = "WorldContextObject"))
	static float GetPoolUsageRate(const UObject* WorldContextObject, FName PoolID);

	/**
	 * 检查池是否健康（未耗尽）
	 * @param WorldContextObject 世界上下文
	 * @param PoolID 池标识符
	 * @param WarningThreshold 警告阈值（使用率超过此值视为不健康）
	 * @return 是否健康
	 */
	UFUNCTION(BlueprintPure, Category = "ProjectilePool|Stats", meta = (WorldContext = "WorldContextObject"))
	static bool IsPoolHealthy(const UObject* WorldContextObject, FName PoolID, float WarningThreshold = 0.9f);
};
