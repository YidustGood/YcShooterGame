// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"
#include "YcHeroComponent.generated.h"

class UYcInputConfig;
struct FInputActionValue;

namespace YcInput
{
	static const float LookYawRate = 300.0f;
	static const float LookPitchRate = 165.0f;
};

/**
 * 英雄组件，负责玩家控制的Pawn的输入管理和技能输入处理
 * 它依赖PawnExtensionComponent来协调初始化流程
 */
UCLASS(ClassGroup=(YiChenGameCore), meta=(BlueprintSpawnableComponent))
class YICHENGAMEPLAY_API UYcHeroComponent : public UPawnComponent, public IGameFrameworkInitStateInterface
{
	GENERATED_BODY()
public:
	UYcHeroComponent(const FObjectInitializer& ObjectInitializer);
	
	/**
	 * 在指定Actor上查找HeroComponent。
	 * @param Actor 要查找的Actor
	 * @return 找到的HeroComponent，如果不存在则返回nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "YcGameCore|Hero")
	static UYcHeroComponent* FindYcHeroComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UYcHeroComponent>() : nullptr); }

	/** 此功能的名称，用于初始化状态系统识别 */
	static const FName NAME_ActorFeatureName;
	
	//~ Begin IGameFrameworkInitStateInterface interface
	/** 获取此组件的功能名称 */
	virtual FName GetFeatureName() const override { return NAME_ActorFeatureName; }
	
	/**
	 * 检查是否允许初始化状态转换，可以在内部做一些判断行为, 以确保是按照我们想要的顺序进行状态迭代 
	 * @param Manager GameFrameworkComponentManager实例
	 * @param CurrentState 当前初始化状态
	 * @param DesiredState 期望转换到的初始化状态
	 * @return 如果允许状态转换则返回true
	 */
	virtual bool CanChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) const override;
	
	/**
	 * 处理初始化状态转换。
	 * @param Manager GameFrameworkComponentManager实例
	 * @param CurrentState 当前初始化状态
	 * @param DesiredState 转换到的初始化状态
	 */
	virtual void HandleChangeInitState(UGameFrameworkComponentManager* Manager, FGameplayTag CurrentState, FGameplayTag DesiredState) override;
	
	/**
	 * 当监听到其他组件状态改变时的回调。
	 * @param Params 包含改变的组件特性名称和新状态的参数
	 */
	virtual void OnActorInitStateChanged(const FActorInitStateChangedParams& Params) override;
	
	/**
	 * 检查并尝试推进初始化状态链。
	 * 调用Owner Pawn上所有实现IGameFrameworkInitStateInterface的组件的CheckDefaultInitialization()，
	 * 然后沿着预定义的状态链尝试推进这些组件的初始化状态。
	 * 当组件设置了一些数据后，就应调用此函数，尝试推进初始化进度，以便依赖该数据的其他组件能够继续推进他们的初始化进度。
	 */
	virtual void CheckDefaultInitialization() override;
	//~ End IGameFrameworkInitStateInterface interface
	
protected:
	virtual void OnRegister() override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	///////////////// 输入相关 /////////////////
public:
	/** 输入已准备好可绑定时通过GameFrameworkComponentManager发送的扩展事件名称 */
	static const FName NAME_BindInputsNow;
	
	/**
	 * 添加额外的InputConfig到此Pawn的输入系统。
	 * 通常由GameFeatureAction调用，以动态添加功能特定的输入配置。
	 * @param InputConfig 要添加的输入配置
	 */
	void AddAdditionalInputConfig(const UYcInputConfig* InputConfig);
	
	/**
	 * 移除之前添加的InputConfig。
	 * @param InputConfig 要移除的输入配置
	 */
	void RemoveAdditionalInputConfig(const UYcInputConfig* InputConfig);
	
	/**
	 * 检查输入是否已准备好可绑定。
	 * 仅当此组件由真实玩家控制且初始化进度足够时才返回true。
	 * @return 如果输入已准备好则返回true
	 */
	bool IsReadyToBindInputs() const;
	
protected:
	/**
	 * 初始化玩家输入绑定。
	 * 会从PawnData获取InputConfig进行绑定，包括绑定Native类型的输入，并发出输入已准备好的扩展事件，通知其他模块可以添加额外的输入配置。
	 * GameFeatureAction_AddInputConfigs通过监听此事件来响应添加输入配置。
	 * @param PlayerInputComponent 玩家的输入组件
	 */
	virtual void InitializePlayerInput(UInputComponent* PlayerInputComponent);
	
	/**
	 * 处理Ability类型输入的按下事件。
	 * 所有Ability类型InputAction触发的按下事件都会绑定到此函数，
	 * 会传入触发InputAction绑定的InputTag，然后传递给ASC组件处理技能激活逻辑。
	 * @param InputTag 触发的输入标签
	 */
	void Input_AbilityInputTagPressed(FGameplayTag InputTag);
	
	/**
	 * 处理Ability类型输入的释放事件。
	 * @param InputTag 触发的输入标签
	 */
	void Input_AbilityInputTagReleased(FGameplayTag InputTag);
	
	/**
	 * 处理角色移动输入。
	 * 内部会根据第一人称/第三人称模式标记计算移动方向。
	 * @param InputActionValue 输入动作值
	 */
	void Input_Move(const FInputActionValue& InputActionValue);
	
	/**
	 * 处理鼠标视角输入。
	 * @param InputActionValue 输入动作值
	 */
	void Input_LookMouse(const FInputActionValue& InputActionValue);
	
	/**
	 * 处理摇杆视角输入。
	 * @param InputActionValue 输入动作值
	 */
	void Input_LookStick(const FInputActionValue& InputActionValue);
	
	/**
	 * 处理下蹲输入。
	 * @param InputActionValue 输入动作值
	 */
	void Input_Crouch(const FInputActionValue& InputActionValue);
	///////////////// ~输入相关 /////////////////
	
private:
	/** 是否为第一人称模式，因为第一人称和第三人称的输入处理有所不同 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	bool bFirstPersonMode;
	
	/** 玩家输入绑定是否已应用，仅对真实玩家控制的Pawn为true */
	bool bReadyToBindInputs;
	
	/** 缓存的所有者Pawn指针 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta=(AllowPrivateAccess = "true"))
	TObjectPtr<APawn> OwnerPawn;
};