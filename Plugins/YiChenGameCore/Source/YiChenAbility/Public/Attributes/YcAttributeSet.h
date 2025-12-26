// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "AttributeSet.h"
#include "YcAttributeSet.generated.h"

class AActor;
class UYcAbilitySystemComponent;
class UObject;
class UWorld;
struct FGameplayEffectSpec;

/**
 * 定义一组用于访问和初始化属性的辅助函数宏
 *
 * 使用示例：
 *		ATTRIBUTE_ACCESSORS(UYcShooterHealthSet, Health)
 * 将创建以下函数：
 *		static FGameplayAttribute GetHealthAttribute();
 *		float GetHealth() const;
 *		void SetHealth(float NewVal);
 *		void InitHealth(float NewVal);
 * @param ClassName 属性集类名
 * @param PropertyName 属性名称
 */
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/** 
 * 用于广播属性事件的委托
 * 注意：在客户端上，某些参数可能为null
 * @param EffectInstigator	触发此事件的原始Actor
 * @param EffectCauser		导致属性变化的物理Actor
 * @param EffectSpec		此次变化的完整效果规格
 * @param EffectMagnitude	原始数值大小（在限制之前）
 * @param OldValue			属性变化前的值
 * @param NewValue			属性变化后的值
 */
DECLARE_MULTICAST_DELEGATE_SixParams(FYcAttributeEvent, AActor* /*EffectInstigator*/, AActor* /*EffectCauser*/, const FGameplayEffectSpec* /*EffectSpec*/, float /*EffectMagnitude*/, float /*OldValue*/, float /*NewValue*/);

/**
 * 插件提供的基础属性集基类
 * 所有自定义属性集应继承自此类，提供统一的属性管理和访问接口
 */
UCLASS(BlueprintType, Const)
class YICHENABILITY_API UYcAttributeSet : public UAttributeSet
{
	GENERATED_BODY()
public:
	UYcAttributeSet();

	/**
	 * 获取属性集所属的世界对象（覆盖父类）
	 * 通过获取外部对象（通常是AbilitySystemComponent）的世界对象来实现
	 * @return 属性集所属的世界对象，如果外部对象无效则可能返回nullptr
	 */
	virtual UWorld* GetWorld() const override;

	/**
	 * 获取属性集所属的技能系统组件
	 * 将父类的AbilitySystemComponent转换为UYcAbilitySystemComponent类型
	 * @return 技能系统组件，如果不存在或类型不匹配则返回nullptr
	 */
	UYcAbilitySystemComponent* GetYcAbilitySystemComponent() const;
};