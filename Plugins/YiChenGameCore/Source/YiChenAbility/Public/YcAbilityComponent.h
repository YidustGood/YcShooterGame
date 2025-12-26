// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YiChenAbility.h"
#include "YcAbilitySystemComponent.h"
#include "Components/GameFrameworkComponent.h"
#include "YcAbilityComponent.generated.h"

struct FAbilitySystemLifeCycleMessage;
class UYcAbilitySystemComponent;
class UYcAttributeSet;

/**
 * 技能系统组件基类
 * 
 * 这是一个基础组件，用于简化所有依赖AbilitySystemComponent的功能组件的实现。
 * 它自动处理ASC生命周期管理和属性集的创建/释放逻辑，子类只需专注于业务逻辑实现。
 * 
 * 设计特点：
 * - 通过GameplayMessageSubsystem监听ASC生命周期消息，实现解耦设计
 * - 提供模板函数AddAttributeSet<T>()用于类型安全的属性集创建
 * - 支持通过AttributeSetsClasses数组在编辑器中预设属性集
 * - 提供虚函数CreateAttributeSets()供子类重写以自定义属性集创建逻辑
 * - 自动管理属性集的生命周期，防止内存泄漏
 * 
 * 使用方式：
 * 1. 继承此组件并重写OnInitializeWithAbilitySystem()来创建所需的属性集
 * 2. 可以通过AddAttributeSet<T>()模板函数创建属性集
 * 3. 也可以通过AttributeSetsClasses数组在编辑器中预设属性集
 * 4. 蓝图可以通过实现K2_OnInitializeWithAbilitySystem事件来扩展功能
 * 
 * 示例：
 * @code
 * void UMyComponent::OnInitializeWithAbilitySystem(...)
 * {
 *     Super::OnInitializeWithAbilitySystem(...);
 *     MyAttributeSet = AddAttributeSet<UMyAttributeSet>();
 *     // 注册属性变化委托等...
 * }
 * @endcode
 * 更详细的案例可以参考YcShooterCore的UYcShooterHealthComponent.cpp
 */
UCLASS(ClassGroup=(YiChenAbility), meta=(BlueprintSpawnableComponent))
class YICHENABILITY_API UYcAbilityComponent : public UGameFrameworkComponent
{
	GENERATED_BODY()
public:
	/**
	 * 构造函数
	 * @param ObjectInitializer 对象初始化器
	 */
	UYcAbilityComponent(const FObjectInitializer& ObjectInitializer);
	
	virtual void BeginPlay() override;
	virtual void OnUnregister() override;
	
	/**
	 * 蓝图可实现的初始化事件
	 * 当组件与AbilitySystemComponent完成初始化后，此事件会在C++的OnInitializeWithAbilitySystem()函数中被调用
	 * 
	 * 蓝图可以通过重写此事件来扩展初始化逻辑，例如：
	 * - 创建额外的属性集
	 * - 注册属性变化委托
	 * - 初始化组件特定的数据
	 * 
	 * @note 此函数在C++的OnInitializeWithAbilitySystem()中自动调用，无需手动触发
	 * @note 如果需要在C++中扩展，应该重写OnInitializeWithAbilitySystem()虚函数
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "YcGameCore|Ability", DisplayName = "OnInitializeWithAbilitySystem")
	void K2_OnInitializeWithAbilitySystem();
	
	/**
	 * 蓝图可实现的取消初始化事件
	 * 当组件与AbilitySystemComponent解除关联时，此事件会在C++的OnUninitializeFromAbilitySystem()函数中被调用
	 * 
	 * 蓝图可以通过重写此事件来扩展清理逻辑，例如：
	 * - 移除属性变化委托
	 * - 清理组件特定的数据
	 * - 重置组件状态
	 * 
	 * @note 此函数在C++的OnUninitializeFromAbilitySystem()中自动调用，无需手动触发
	 * @note 如果需要在C++中扩展，应该重写OnUninitializeFromAbilitySystem()虚函数
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "YcGameCore|Ability", DisplayName = "OnUninitializeFromAbilitySystem")
	void K2_OnUninitializeFromAbilitySystem();
    
	/**
	 * 通过类对象创建并添加属性集（蓝图友好版本）
	 * 此函数会自动创建属性集实例并添加到ASC中，同时保存引用以防止垃圾回收
	 * 
	 * @param AttributeSetClass 要创建的属性集类
	 * @return 添加的属性集指针，如果创建失败或已存在则返回已存在的实例
	 * 
	 * @note 如果属性集已存在，此函数不会重复创建，而是返回已存在的实例
	 * @note 如果AbilitySystemComponent为null或创建失败，会记录错误日志并返回nullptr
	 * @note 此函数是AddAttributeSet<T>()的蓝图友好版本，适用于运行时动态创建属性集
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Attributes")
	const UYcAttributeSet* AddAttributeSetByClass(const TSubclassOf<UYcAttributeSet>& AttributeSetClass);
	
	/**
	 * 获取指定类型的属性集
	 * @tparam T 属性集类型
	 * @return 属性集指针，如果不存在则返回nullptr
	 */
	template<typename T>
	const T* GetAttributeSet()
	{
		return AbilitySystemComponent ? AbilitySystemComponent->GetSet<T>() : nullptr;
	}
	
protected:
	/**
	 * 使用技能系统组件初始化此组件
	 * 从ASC生命周期消息中获取ASC并设置属性集，注册属性变化委托
	 * @param Channel 消息通道标签
	 * @param Message ASC生命周期消息
	 */
	virtual void OnInitializeWithAbilitySystem(FGameplayTag Channel, const FAbilitySystemLifeCycleMessage& Message);
	
	/**
	 * 取消初始化组件，清除对技能系统的所有引用
	 * @param Channel 消息通道标签
	 * @param Message ASC生命周期消息
	 */
	virtual void OnUninitializeFromAbilitySystem(FGameplayTag Channel, const FAbilitySystemLifeCycleMessage& Message);
	
	/**
	 * 创建属性集的虚函数钩子
	 * 子类可以重写此函数来自定义属性集的创建逻辑
	 * 默认实现会遍历AttributeSetsClasses数组并创建所有预设的属性集
	 * 
	 * 如果子类需要完全自定义属性集创建逻辑，可以重写此函数而不调用Super
	 * 如果子类只需要在默认行为基础上添加额外的属性集，应该调用Super然后添加自己的属性集
	 */
	virtual void CreateAttributeSets();
	
	/** 遍历 AttributeSetsClasses 创建所有 AttributeSet */
	void InitializeAllAttributeSets();
	
	/** 此组件要创建的AttributeSet类数组 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<TSubclassOf<UYcAttributeSet>> AttributeSetsClasses;
	
	/** 此组件使用的技能系统组件 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TObjectPtr<UYcAbilitySystemComponent> AbilitySystemComponent;
	
	/** 存储的属性集实例的数组，防止被垃圾回收 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly)
	TArray<TObjectPtr<const UYcAttributeSet>> AttributeSets;
	
public:
	/**
	 * 添加属性集到ASC的模板函数
	 * 此函数会自动创建属性集实例并添加到ASC中，同时保存引用以防止垃圾回收
	 * 
	 * @tparam T 属性集类型，必须是UYcAttributeSet的子类
	 * @return 添加的属性集指针，如果创建失败则返回nullptr
	 * 
	 * @note 如果属性集已存在，此函数不会重复创建，而是返回已存在的实例
	 * @note 如果AbilitySystemComponent为null或创建失败，会记录错误日志并返回nullptr
	 */
	template<typename T>
	const T* AddAttributeSet()
	{
		if (!AbilitySystemComponent)
		{
			UE_LOG(LogYcAbilitySystem, Error, TEXT("UYcAbilityComponent::AddAttributeSet: AbilitySystemComponent is null! Cannot add AttributeSet."));
			return nullptr;
		}
		
		// 检查是否已经存在该类型的属性集
		if (const T* ExistingSet = GetAttributeSet<T>())
		{
			// 如果ASC中有但我们的数组中没有，添加到数组中
			if (!AttributeSets.Contains(ExistingSet))
			{
				AttributeSets.Add(ExistingSet);
			}
			UE_LOG(LogYcAbilitySystem, Warning, TEXT("UYcAbilityComponent::AddAttributeSet: AttributeSet of type %s already exists. Returning existing instance."), *T::StaticClass()->GetName());
			return ExistingSet;
		}
    
		// Outer要指定为Actor
		T* AttributeSet = NewObject<T>(AbilitySystemComponent->GetOwner());
		if (!AttributeSet)
		{
			UE_LOG(LogYcAbilitySystem, Error, TEXT("UYcAbilityComponent::AddAttributeSet: Failed to create AttributeSet of type %s!"), *T::StaticClass()->GetName());
			return nullptr;
		}
    
		AbilitySystemComponent->AddAttributeSetSubobject(AttributeSet);
		AttributeSets.Add(AttributeSet);
		
		UE_LOG(LogYcAbilitySystem, Log, TEXT("UYcAbilityComponent::AddAttributeSet: Successfully added AttributeSet of type %s"), *T::StaticClass()->GetName());
		return AttributeSet;
	}
};