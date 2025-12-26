// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcAbilityComponent.h"

#include "YcAbilitySystemComponent.h"
#include "YiChenAbility.h"
#include "Attributes/YcAttributeSet.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameplayCommon/GameplayMessageTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAbilityComponent)

UYcAbilityComponent::UYcAbilityComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bStartWithTickEnabled = false;
	PrimaryComponentTick.bCanEverTick = false;
	AbilitySystemComponent = nullptr;
}

void UYcAbilityComponent::BeginPlay()
{
	Super::BeginPlay();
	// 监听ASC生命周期消息，以便进行初始化和清理操作
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
	MessageSubsystem.RegisterListener(YcGameplayTags::TAG_Ability_State_AbilitySystemInitialized, this, &ThisClass::OnInitializeWithAbilitySystem);
	MessageSubsystem.RegisterListener(YcGameplayTags::TAG_Ability_State_AbilitySystemUninitialized, this, &ThisClass::OnUninitializeFromAbilitySystem);
}

void UYcAbilityComponent::OnUnregister()
{
	OnUninitializeFromAbilitySystem(FGameplayTag(), FAbilitySystemLifeCycleMessage());
	Super::OnUnregister();
}

void UYcAbilityComponent::OnInitializeWithAbilitySystem(FGameplayTag Channel,
                                                        const FAbilitySystemLifeCycleMessage& Message)
{
	const AActor* Owner = GetOwner();
	check(Owner);
	
	// 防止重复初始化
	if (AbilitySystemComponent)
	{
		UE_LOG(LogYcAbilitySystem, Error, TEXT("UYcAbilityComponent: ability component for owner [%s] has already been initialized with an ability system."), *GetNameSafe(Owner));
		return;
	}
	
	// 从消息中获取ASC组件对象
	AbilitySystemComponent = Cast<UYcAbilitySystemComponent>(Message.ASC);
	check(AbilitySystemComponent);
	
	// 创建属性集（子类可以重写CreateAttributeSets()来自定义创建逻辑）
	CreateAttributeSets();
	
	// 调用蓝图版本, 以支持蓝图扩展这个事件的响应逻辑
	K2_OnInitializeWithAbilitySystem();
}

void UYcAbilityComponent::OnUninitializeFromAbilitySystem(FGameplayTag Channel,
	const FAbilitySystemLifeCycleMessage& Message)
{
	// 清空引用
	AbilitySystemComponent = nullptr;
	AttributeSets.Empty();
	
	K2_OnUninitializeFromAbilitySystem();
}

void UYcAbilityComponent::CreateAttributeSets()
{
	// 默认实现：遍历AttributeSetsClasses数组创建所有预设的属性集
	// 子类可以重写此函数来完全自定义属性集创建逻辑
	InitializeAllAttributeSets();
}

void UYcAbilityComponent::InitializeAllAttributeSets()
{
	if (!AbilitySystemComponent)
	{
		UE_LOG(LogYcAbilitySystem, Error, TEXT("AbilitySystemComponent is null! Cannot initialize AttributeSets."));
		return;
	}
	
	AttributeSets.Reset(AttributeSetsClasses.Num());
    
	// 遍历并创建所有预设的 AttributeSet
	for (const auto& AttributeSetClass : AttributeSetsClasses)
	{
		if (AttributeSetClass)
		{
			AddAttributeSetByClass(AttributeSetClass);
		}
		else
		{
			UE_LOG(LogYcAbilitySystem, Warning, TEXT("Found null AttributeSet class in AttributeSetsClasses array!"));
		}
	}
    
	UE_LOG(LogYcAbilitySystem, Log, TEXT("Initialized %d AttributeSets"), AttributeSets.Num());
}

const UYcAttributeSet* UYcAbilityComponent::AddAttributeSetByClass(const TSubclassOf<UYcAttributeSet>& AttributeSetClass)
{
	if (!AbilitySystemComponent || !AttributeSetClass)
	{
		return nullptr;
	}
	
	// 检查是否已经存在该类型的属性集
	if (const UYcAttributeSet* ExistingSet = Cast<const UYcAttributeSet>(AbilitySystemComponent->GetAttributeSet(AttributeSetClass)))
	{
		// 如果ASC中有但我们的数组中没有，添加到数组中
		if (!AttributeSets.Contains(ExistingSet))
		{
			AttributeSets.Add(ExistingSet);
		}
		UE_LOG(LogYcAbilitySystem, Warning, TEXT("UYcAbilityComponent::AddAttributeSetByClass: AttributeSet of type %s already exists in ASC. Returning existing instance."), *AttributeSetClass->GetName());
		return ExistingSet;
	}
    
	// 创建新的 YcAttributeSet 实例
	UYcAttributeSet* AttributeSet = NewObject<UYcAttributeSet>(
		AbilitySystemComponent->GetOwner(),
		AttributeSetClass
	);
    
	if (AttributeSet)
	{
		AbilitySystemComponent->AddAttributeSetSubobject(AttributeSet);
		AttributeSets.Add(AttributeSet);
		
		UE_LOG(LogYcAbilitySystem, Log, TEXT("UYcAbilityComponent::AddAttributeSetByClass: Successfully added AttributeSet of type %s"), *AttributeSetClass->GetName());
	}
	else
	{
		UE_LOG(LogYcAbilitySystem, Error, TEXT("UYcAbilityComponent::AddAttributeSetByClass: Failed to create AttributeSet of type %s"), *GetNameSafe(AttributeSetClass));
	}
    
	return AttributeSet;
}