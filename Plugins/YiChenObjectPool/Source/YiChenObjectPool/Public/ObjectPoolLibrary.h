// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ObjectPoolTypes.h"
#include "ObjectPoolLibrary.generated.h"

class UObjectPoolSubsystem;

/**
 * 对象池蓝图函数库
 * 
 * 提供便捷的静态函数用于对象池操作
 */
UCLASS()
class YICHENOBJECTPOOL_API UObjectPoolLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 获取对象池子系统
	 */
	UFUNCTION(BlueprintPure, Category = "ObjectPool", meta = (WorldContext = "WorldContextObject"))
	static UObjectPoolSubsystem* GetObjectPoolSubsystem(const UObject* WorldContextObject);

	/**
	 * 快速设置对象池
	 * @param WorldContextObject 世界上下文
	 * @param PoolID 池标识符
	 * @param ObjectClass 对象类
	 * @param InitialSize 初始大小
	 * @param MaxSize 最大大小（0表示无限制）
	 * @return 是否成功
	 */
	UFUNCTION(BlueprintCallable, Category = "ObjectPool", meta = (WorldContext = "WorldContextObject"))
	static bool QuickSetupPool(
		const UObject* WorldContextObject,
		FName PoolID,
		TSubclassOf<UObject> ObjectClass,
		int32 InitialSize = 10,
		int32 MaxSize = 100
	);

	/**
	 * 快速获取对象
	 */
	UFUNCTION(BlueprintCallable, Category = "ObjectPool", meta = (WorldContext = "WorldContextObject", DeterminesOutputType = "ObjectClass"))
	static UObject* QuickAcquire(
		const UObject* WorldContextObject,
		FName PoolID
	);

	/**
	 * 快速获取Actor
	 */
	UFUNCTION(BlueprintCallable, Category = "ObjectPool", meta = (WorldContext = "WorldContextObject"))
	static AActor* QuickAcquireActor(
		const UObject* WorldContextObject,
		FName PoolID
	);

	/**
	 * 快速释放对象
	 */
	UFUNCTION(BlueprintCallable, Category = "ObjectPool", meta = (WorldContext = "WorldContextObject"))
	static bool QuickRelease(
		const UObject* WorldContextObject,
		UObject* Object
	);

	/**
	 * 获取池使用率
	 * @return 使用率 (0.0-1.0)，池不存在返回-1
	 */
	UFUNCTION(BlueprintPure, Category = "ObjectPool|Stats", meta = (WorldContext = "WorldContextObject"))
	static float GetPoolUsageRate(const UObject* WorldContextObject, FName PoolID);

	/**
	 * 检查池是否健康
	 */
	UFUNCTION(BlueprintPure, Category = "ObjectPool|Stats", meta = (WorldContext = "WorldContextObject"))
	static bool IsPoolHealthy(const UObject* WorldContextObject, FName PoolID, float WarningThreshold = 0.9f);

	/**
	 * 获取池可用数量
	 */
	UFUNCTION(BlueprintPure, Category = "ObjectPool|Stats", meta = (WorldContext = "WorldContextObject"))
	static int32 GetPoolAvailableCount(const UObject* WorldContextObject, FName PoolID);
};
