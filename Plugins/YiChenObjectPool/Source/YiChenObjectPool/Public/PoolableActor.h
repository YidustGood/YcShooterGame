// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "IPoolableObject.h"
#include "PoolableActor.generated.h"

/**
 * 可池化Actor基类
 * 
 * 继承此类可快速创建支持对象池的Actor。
 * 已实现IPoolableObject接口的基本功能。
 * 
 * 子类应重写:
 * - OnActivated(): 对象激活时的自定义逻辑
 * - OnDeactivated(): 对象停用时的自定义逻辑
 * - OnReset(): 重置对象状态
 */
UCLASS(Abstract, Blueprintable)
class YICHENOBJECTPOOL_API APoolableActor : public AActor, public IPoolableObject
{
	GENERATED_BODY()

public:
	APoolableActor();

	//~ Begin IPoolableObject Interface
	virtual void OnAcquiredFromPool() override;
	virtual void OnReleasedToPool() override;
	virtual EPoolableObjectState GetPoolState() const override { return PoolState; }
	virtual void SetPoolState(EPoolableObjectState NewState) override { PoolState = NewState; }
	virtual FName GetPoolIdentifier() const override { return PoolIdentifier; }
	virtual void OnRemovedFromPool() override;
	virtual void ResetPooledObject() override;
	//~ End IPoolableObject Interface

	/** 设置池标识符 */
	UFUNCTION(BlueprintCallable, Category = "Pool")
	void SetPoolIdentifier(FName NewIdentifier) { PoolIdentifier = NewIdentifier; }

	/** 请求返回池（会触发OnDeactivated） */
	UFUNCTION(BlueprintCallable, Category = "Pool")
	void RequestReturnToPool();

protected:
	/** 对象激活时调用（蓝图可重写） */
	UFUNCTION(BlueprintNativeEvent, Category = "Pool")
	void OnActivated();

	/** 对象停用时调用（蓝图可重写） */
	UFUNCTION(BlueprintNativeEvent, Category = "Pool")
	void OnDeactivated();

	/** 重置对象状态（蓝图可重写） */
	UFUNCTION(BlueprintNativeEvent, Category = "Pool")
	void OnReset();

protected:
	/** 池标识符 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Pool")
	FName PoolIdentifier;

	/** 当前池状态 */
	UPROPERTY(BlueprintReadOnly, Category = "Pool")
	EPoolableObjectState PoolState;
};
