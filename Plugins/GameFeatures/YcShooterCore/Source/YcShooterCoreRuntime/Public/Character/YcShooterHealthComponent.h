// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcAbilityComponent.h"

#include "YcShooterHealthComponent.generated.h"

struct FGameplayTag;
class UYcAbilitySystemComponent;
class UYcShooterHealthSet;
class UYcCombatSet;
struct FGameplayEffectSpec;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FYcShooterHealth_DeathEvent, AActor*, OwningActor);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FYcShooterHealth_AttributeChanged, UYcShooterHealthComponent*, HealthComponent, float, OldValue, float, NewValue, AActor*, Instigator);

/**
 *	定义当前死亡状态
 *	用于表示当前角色的死亡状态, 我们为角色死亡定义了一个过程, 既开始和完成死亡, 以便划分出不同的逻辑阶段
 */
UENUM(BlueprintType)
enum class EYcDeathState : uint8
{
	NotDead = 0,	// 未死亡
	DeathStarted,	// 开始死亡
	DeathFinished	// 完成死亡
};

/**
 * 射击游戏健康组件
 * 用于处理与健康相关的所有功能，包括健康值管理、死亡序列处理等
 * 监听ASC生命周期消息进行初始化，监听健康属性变化事件，处理健康值归零事件
 * 健康值归零后会在HandleOutOfHealth()这个回调中发送GameplayEvent触发死亡技能
 * 然后具体的死亡逻辑一部分可以在死亡技能中实现，一部分可以在OnDeathStarted和OnDeathFinished委托的自定义回调函数中实现
 */
UCLASS(ClassGroup=(YiChenShooterCore), meta=(BlueprintSpawnableComponent))
class YCSHOOTERCORERUNTIME_API UYcShooterHealthComponent : public UYcAbilityComponent
{
	GENERATED_BODY()

public:
	/**
	 * 构造函数
	 * @param ObjectInitializer 对象初始化器
	 */
	UYcShooterHealthComponent(const FObjectInitializer& ObjectInitializer);
	
	/**
	 * 查找指定Actor上的健康组件
	 * @param Actor 目标Actor
	 * @return 找到的健康组件，如果不存在则返回nullptr
	 */
	UFUNCTION(BlueprintPure, Category = "YcGameCore|Health")
	static UYcShooterHealthComponent* FindHealthComponent(const AActor* Actor) { return (Actor ? Actor->FindComponentByClass<UYcShooterHealthComponent>() : nullptr); }

	/**
	 * 模板方法：与AbilitySystemComponent初始化时的业务逻辑
	 * 创建健康属性集和战斗属性集，注册属性变化委托
	 * @param ASC 已验证的AbilitySystemComponent
	 */
	virtual void DoInitializeWithAbilitySystem(UYcAbilitySystemComponent* ASC) override;
	
	/**
	 * 模板方法：与AbilitySystemComponent解除关联时的清理逻辑
	 * 移除属性变化委托，清理属性集引用
	 */
	virtual void DoUninitializeFromAbilitySystem() override;
	
	/**
	 * 获取当前健康值
	 * @return 当前健康值
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Health")
	float GetHealth() const;

	/**
	 * 获取当前最大健康值
	 * @return 当前最大健康值
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Health")
	float GetMaxHealth() const;
	
	/**
	 * 获取归一化的健康值，范围[0.0, 1.0]
	 * @return 归一化的健康值
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Health")
	float GetHealthNormalized() const;
	
	/**
	 * 获取当前死亡状态
	 * @return 死亡状态
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Health")
	EYcDeathState GetDeathState() const { return DeathState; }
	
	/**
	 * 检查是否已死亡或正在死亡
	 * @return 如果死亡状态大于NotDead则返回true
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "YcGameCore|Health", Meta = (ExpandBoolAsExecs = "ReturnValue"))
	bool IsDeadOrDying() const { return (DeathState > EYcDeathState::NotDead); }
	
	/**
	 * Blueprint可调用的自毁伤害函数
	 * @param bFellOutOfWorld 是否因掉出世界而死亡
	 */
	UFUNCTION(BlueprintCallable, DisplayName = "DamageSelfDestruct")
	void K2_DamageSelfDestruct(bool bFellOutOfWorld = false);
	
	/**
	 * 开始死亡序列
	 * 由服务端YcGameplayAbility_ShooterDeath技能激活时调用开始，开始后会修改DeathState状态同步到客户端，客户端根据状态进行死亡序列事件同步
	 * 这个函数会在服务端和所有客户端执行
	 */
	virtual void StartDeath();

	/**
	 * 完成死亡序列
	 * 由YcGameplayAbility_ShooterDeath技能结束后调用
	 * 这个函数会在服务端和所有客户端执行
	 */
	virtual void FinishDeath();

	/**
	 * 对自身施加足以杀死Owner的伤害
	 * @param bFellOutOfWorld 是否因掉出世界而死亡
	 */
	virtual void DamageSelfDestruct(bool bFellOutOfWorld = false);
	
	/** 
	 * 健康值变化委托, 会在服务端和所有客户端执行, 在客户端上调用时instigator可能无效
	 * 服务端的发起源头在UYcShooterHealthSet::PostGameplayEffectExecute()中
	 * 客户端的发起源头在UYcShooterHealthSet::OnRep_Health()属性同步通知函数中
	 */
	UPROPERTY(BlueprintAssignable)
	FYcShooterHealth_AttributeChanged OnHealthChanged;

	/** 
	 * 最大健康值变化委托, 会在服务端和所有客户端执行, 在客户端上调用时instigator可能无效
	 * 服务端的发起源头在UYcShooterHealthSet::PostGameplayEffectExecute()中
	 * 客户端的发起源头在UYcShooterHealthSet::OnRep_MaxHealth()属性同步通知函数中
	 */
	UPROPERTY(BlueprintAssignable)
	FYcShooterHealth_AttributeChanged OnMaxHealthChanged;

	/** 死亡序列开始委托, 会在服务端和所有客户端执行 */
	UPROPERTY(BlueprintAssignable)
	FYcShooterHealth_DeathEvent OnDeathStarted;

	/** 死亡序列完成委托, 会在服务端和所有客户端执行 */
	UPROPERTY(BlueprintAssignable)
	FYcShooterHealth_DeathEvent OnDeathFinished;
	
protected:
	/**
	 * 清理死亡相关的GameplayTag
	 */
	void ClearGameplayTags();

	/**
	 * 健康值变化的回调函数, 服务端和所有客户端都会触发
	 * @param DamageInstigator 伤害发起者
	 * @param DamageCauser 伤害原因者
	 * @param DamageEffectSpec 伤害效果规格
	 * @param DamageMagnitude 伤害数值
	 * @param OldValue 旧值
	 * @param NewValue 新值
	 */
	virtual void HandleHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	
	/**
	 * 最大健康值变化的回调函数, 服务端和所有客户端都会触发
	 * @param DamageInstigator 伤害发起者
	 * @param DamageCauser 伤害原因者
	 * @param DamageEffectSpec 伤害效果规格
	 * @param DamageMagnitude 伤害数值
	 * @param OldValue 旧值
	 * @param NewValue 新值
	 */
	virtual void HandleMaxHealthChanged(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);
	
	/**
	 * 健康值归零的回调函数, 服务端和所有客户端都会触发
	 * 发送死亡游戏事件触发死亡技能，发送消灭消息供其他系统观察
	 * @param DamageInstigator 伤害发起者
	 * @param DamageCauser 伤害原因者
	 * @param DamageEffectSpec 伤害效果规格
	 * @param DamageMagnitude 伤害数值
	 * @param OldValue 旧值
	 * @param NewValue 新值
	 */
	virtual void HandleOutOfHealth(AActor* DamageInstigator, AActor* DamageCauser, const FGameplayEffectSpec* DamageEffectSpec, float DamageMagnitude, float OldValue, float NewValue);

	/**
	 * 死亡状态网络复制回调
	 * 客户端根据状态变化调用StartDeath和FinishDeath
	 * @param OldDeathState 旧死亡状态
	 */
	UFUNCTION()
	virtual void OnRep_DeathState(EYcDeathState OldDeathState);
	
	/** 此Actor使用的健康属性集 */
	UPROPERTY(VisibleAnywhere, Category= "AttributeSet")
	TObjectPtr<const UYcShooterHealthSet> HealthSet;
	
	/** 用于处理死亡状态的复制状态 */
	UPROPERTY(ReplicatedUsing = OnRep_DeathState)
	EYcDeathState DeathState;
};