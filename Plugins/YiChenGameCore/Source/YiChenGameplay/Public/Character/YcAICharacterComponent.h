// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"
#include "YcAICharacterComponent.generated.h"

/**
 * AI角色组件，负责AI控制的角色（如怪物、小兵等）的初始化
 * 
 * 与UYcHeroComponent对称设计，但针对没有PlayerState的AI角色：
 * - 不需要PlayerState
 * - ASC直接挂载在Character上
 * - 不处理玩家输入绑定
 * 
 * 适用场景：
 * - 怪物、小兵等非玩家控制的AI角色
 * - ASC直接挂载在Character上的角色
 * - 不需要PlayerState持久化数据的角色
 */
UCLASS(ClassGroup=(YiChenGameCore), meta=(BlueprintSpawnableComponent))
class YICHENGAMEPLAY_API UYcAICharacterComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()
public:
	UYcAICharacterComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	/** 此功能的名称，用于初始化状态系统识别 */
	static const FName NAME_ActorFeatureName;
	
	//~ Begin IGameFrameworkInitStateInterface interface
	/** 获取此组件的功能名称 */
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	
	/**
	 * 检查是否允许初始化状态转换
	 * @param Manager GameFrameworkComponentManager实例
	 * @param CurrentState 当前初始化状态
	 * @param DesiredState 期望转换到的初始化状态
	 * @return 如果允许状态转换则返回true
	 */
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	
	/**
	 * 处理初始化状态转换
	 * 在DataAvailable -> DataInitialized阶段初始化ASC
	 * @param Manager GameFrameworkComponentManager实例
	 * @param CurrentState 当前初始化状态
	 * @param DesiredState 转换到的初始化状态
	 */
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	
	/**
	 * 当监听到其他组件状态改变时的回调
	 * @param Params 包含改变的组件特性名称和新状态的参数
	 */
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	
	/**
	 * 检查并尝试推进初始化状态链
	 */
	virtual void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface interface
	
protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
private:
	/** 缓存的所有者Pawn指针 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess="true"))
	TObjectPtr<APawn> OwnerPawn;
};
