// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcAbilityComponent.h"
#include "YcMovementSyncComponent.generated.h"

class UYcMovementAttributeSet;
class UCharacterMovementComponent;

/**
 * 移动同步组件
 * 
 * 功能说明：
 * - 继承自UYcAbilityComponent，自动处理ASC生命周期
 * - 监听MovementAttributeSet中的属性变化
 * - 自动将属性值同步到CharacterMovementComponent
 * - 支持多个GameplayEffect对移速的叠加影响
 * 
 * 使用方法：
 * 1. 将此组件添加到Character上
 * 2. 确保Character有CharacterMovementComponent
 * 3. 确保PlayerState的ASC中注册了UYcMovementAttributeSet
 * 4. 组件会自动处理属性同步
 * 
 * 设计说明：
 * - 最终移速 = MaxWalkSpeed * MaxWalkSpeedMultiplier
 * - 支持服务器和客户端的正确同步
 * - 继承UYcAbilityComponent，无需手动处理ASC初始化时机
 */
UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class YICHENGAMEPLAY_API UYcMovementSyncComponent : public UYcAbilityComponent
{
	GENERATED_BODY()

public:
	UYcMovementSyncComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	/**
	 * 模板方法：与AbilitySystemComponent初始化时的业务逻辑
	 * 获取MovementAttributeSet并注册属性变化回调
	 * @param ASC 已验证的AbilitySystemComponent
	 */
	virtual void DoInitializeWithAbilitySystem(UYcAbilitySystemComponent* ASC) override;
	
	/**
	 * 模板方法：与AbilitySystemComponent解除关联时的清理逻辑
	 * 移除属性变化回调，清理引用
	 */
	virtual void DoUninitializeFromAbilitySystem() override;

	// 属性变化回调
	void OnMaxWalkSpeedChanged(const FOnAttributeChangeData& Data);
	void OnMaxWalkSpeedMultiplierChanged(const FOnAttributeChangeData& Data);

	// 更新CharacterMovementComponent的速度
	void UpdateMovementSpeed();

private:
	// 缓存的MovementAttributeSet引用
	UPROPERTY()
	TObjectPtr<const UYcMovementAttributeSet> MovementAttributeSet;

	// 缓存的CharacterMovementComponent引用
	UPROPERTY()
	TObjectPtr<UCharacterMovementComponent> CharacterMovementComponent;

	// 属性变化监听句柄
	FDelegateHandle MaxWalkSpeedChangedHandle;
	FDelegateHandle MaxWalkSpeedMultiplierChangedHandle;
};
