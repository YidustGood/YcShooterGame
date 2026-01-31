// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IPoolableObject.generated.h"

/**
 * 对象池状态
 */
UENUM(BlueprintType)
enum class EPoolableObjectState : uint8
{
	/** 在池中休眠 */
	Pooled,
	/** 已激活使用中 */
	Active,
	/** 等待回收 */
	PendingRecycle
};

/**
 * 可池化对象接口
 * 
 * 所有需要被对象池管理的类都应实现此接口。
 * 支持UObject和AActor派生类。
 * 
 * 使用方式:
 * 1. 让你的类继承此接口: class AMyActor : public AActor, public IPoolableObject
 * 2. 实现所有纯虚函数
 * 3. 在OnAcquiredFromPool中初始化对象状态
 * 4. 在OnReleasedToPool中清理对象状态
 */
UINTERFACE(MinimalAPI, Blueprintable, meta = (CannotImplementInterfaceInBlueprint))
class UPoolableObject : public UInterface
{
	GENERATED_BODY()
};

class YICHENOBJECTPOOL_API IPoolableObject
{
	GENERATED_BODY()

public:
	/**
	 * 当对象从池中获取时调用
	 * 在此处初始化对象状态、启用Tick、显示可视化等
	 */
	virtual void OnAcquiredFromPool() = 0;

	/**
	 * 当对象返回池中时调用
	 * 在此处清理状态、禁用Tick、隐藏可视化、清除引用等
	 */
	virtual void OnReleasedToPool() = 0;

	/**
	 * 获取当前池状态
	 */
	virtual EPoolableObjectState GetPoolState() const = 0;

	/**
	 * 设置池状态（由池管理器调用）
	 */
	virtual void SetPoolState(EPoolableObjectState NewState) = 0;

	/**
	 * 检查对象是否可以从池中获取
	 * 默认实现检查状态是否为Pooled
	 */
	virtual bool CanBeAcquired() const
	{
		return GetPoolState() == EPoolableObjectState::Pooled;
	}

	/**
	 * 检查对象是否可以返回池中
	 * 默认实现检查状态是否为Active或PendingRecycle
	 */
	virtual bool CanBeReleased() const
	{
		const EPoolableObjectState State = GetPoolState();
		return State == EPoolableObjectState::Active || State == EPoolableObjectState::PendingRecycle;
	}

	/**
	 * 获取池标识符
	 * 用于多类型池管理，同一类型的对象应返回相同的ID
	 */
	virtual FName GetPoolIdentifier() const = 0;

	/**
	 * 对象即将被销毁时调用（池被清空或对象被永久移除）
	 * 可选实现，用于最终清理
	 */
	virtual void OnRemovedFromPool() {}

	/**
	 * 重置对象到初始状态
	 * 在OnReleasedToPool之后调用，确保对象可以被安全复用
	 */
	virtual void ResetPooledObject() {}
};
