// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"
#include "AbilitySystemComponent.h"
#include "Attributes/YcAttributeSet.h"
#include "YcShooterHealthSet.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_Damage);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_DamageImmunity);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_DamageSelfDestruct);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_FellOutOfWorld);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Yc_Damage_Message);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_Attribute_SetByCaller_Heal);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Gameplay_Attribute_SetByCaller_Damage);

/**
 * 射击游戏健康属性集
 * 管理角色的生命值、最大生命值、伤害和治疗等健康相关属性
 * 提供健康值变化事件委托，支持伤害免疫、自毁伤害等特殊处理
 */
UCLASS()
class YCSHOOTERCORERUNTIME_API UYcShooterHealthSet : public UYcAttributeSet
{
	GENERATED_BODY()
	public:

	UYcShooterHealthSet();

	ATTRIBUTE_ACCESSORS(UYcShooterHealthSet, Health);
	ATTRIBUTE_ACCESSORS(UYcShooterHealthSet, MaxHealth);
	ATTRIBUTE_ACCESSORS(UYcShooterHealthSet, Healing);
	ATTRIBUTE_ACCESSORS(UYcShooterHealthSet, Damage);
	
	/** 健康值变化委托，当由于伤害或治疗导致健康值改变时触发，注意：客户端上的一些信息可能会丢失 */
	mutable FYcAttributeEvent OnHealthChanged;

	/** 最大健康值变化委托 */
	mutable FYcAttributeEvent OnMaxHealthChanged;

	/** 健康值归零委托，当健康值属性达到0时广播 */
	mutable FYcAttributeEvent OnOutOfHealth;

protected:

	/**
	 * 健康值网络复制回调
	 * @param OldValue 旧值
	 */
	UFUNCTION()
	void OnRep_Health(const FGameplayAttributeData& OldValue);

	/**
	 * 最大健康值网络复制回调
	 * @param OldValue 旧值
	 */
	UFUNCTION()
	void OnRep_MaxHealth(const FGameplayAttributeData& OldValue);

	/**
	 * 游戏效果执行前的预处理
	 * 处理伤害免疫、GodMode等特殊情况
	 * @param Data 效果修改回调数据
	 * @return 是否允许继续执行效果
	 */
	virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data) override;
	
	/**
	 * 游戏效果执行后的后处理
	 * 将Damage/Healing转换为Health变化，处理健康值归零等逻辑
	 * @param Data 效果修改回调数据
	 */
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;

	/**
	 * 属性基础值变化前的预处理
	 * @param Attribute 属性
	 * @param NewValue 新值（可修改）
	 */
	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	
	/**
	 * 属性当前值变化前的预处理
	 * @param Attribute 属性
	 * @param NewValue 新值（可修改）
	 */
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	
	/**
	 * 属性变化后的后处理
	 * 确保健康值不超过最大健康值等
	 * @param Attribute 属性
	 * @param OldValue 旧值
	 * @param NewValue 新值
	 */
	virtual void PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue) override;

	/**
	 * 限制属性值在有效范围内
	 * @param Attribute 属性
	 * @param NewValue 新值（会被修改）
	 */
	void ClampAttribute(const FGameplayAttribute& Attribute, float& NewValue) const;


private:

	/** 当前健康值属性，会被最大健康值限制。Health对GE的修改器隐藏，只能通过执行（Execution）修改 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_Health, Category = "YcGameCore|Health", Meta = (HideFromModifiers, AllowPrivateAccess = true))
	FGameplayAttributeData Health;

	/** 当前最大健康值属性，可以通过游戏效果修改 */
	UPROPERTY(BlueprintReadOnly, ReplicatedUsing = OnRep_MaxHealth, Category = "YcGameCore|Health", Meta = (AllowPrivateAccess = true))
	FGameplayAttributeData MaxHealth;

	/** 用于跟踪健康值是否已归零 */
	bool bOutOfHealth;

	/** 属性变化前存储的健康值，用于计算变化量 */
	float MaxHealthBeforeAttributeChange;
	float HealthBeforeAttributeChange;

	// -------------------------------------------------------------------
	//	元属性, 由GE设置修改这些数值, 然后在PostGameplayEffectExecute()中会利用这里的数据计算Health（请将非状态性的属性保持在下方）
	// -------------------------------------------------------------------

	/** 接收到的治疗值，映射为+Health。使用Health + Healing后重置归零，在GE中对这个值进行增加 */
	UPROPERTY(BlueprintReadOnly, Category="YcGameCore|Health", Meta=(AllowPrivateAccess=true))
	FGameplayAttributeData Healing;

	/** 接收到的伤害值，映射为-Health。使用Health - Damage后重置归零，在GE中对这个值进行增加 */
	UPROPERTY(BlueprintReadOnly, Category="YcGameCore|Health", Meta=(HideFromModifiers, AllowPrivateAccess=true))
	FGameplayAttributeData Damage;
};