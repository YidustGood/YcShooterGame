// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Character/YcMovementSyncComponent.h"
#include "YcAbilitySystemComponent.h"
#include "YiChenGameplay.h"
#include "AbilitySystem/Attributes/YcMovementAttributeSet.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcMovementSyncComponent)

UYcMovementSyncComponent::UYcMovementSyncComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UYcMovementSyncComponent::DoInitializeWithAbilitySystem(UYcAbilitySystemComponent* ASC)
{
	Super::DoInitializeWithAbilitySystem(ASC);

	const AActor* Owner = GetOwner();
	check(Owner);
	
	// 获取CharacterMovementComponent
	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		CharacterMovementComponent = Character->GetCharacterMovement();
	}else
	{
		UE_LOG(LogYcGameplay, Error, TEXT("UYcMovementSyncComponent: Cannot initialize for owner [%s] without CharacterMovementComponent."), *GetNameSafe(Owner));
		return;
	}
	
	// 所需的MovementAttributeSet
	MovementAttributeSet = AddAttributeSet<UYcMovementAttributeSet>();
	
	MovementAttributeSet = GetAttributeSet<UYcMovementAttributeSet>();
	if (!MovementAttributeSet)
	{
		UE_LOG(LogYcGameplay, Error, TEXT("UYcMovementSyncComponent: Cannot initialize for owner [%s] with NULL MovementAttributeSet on the ability system."), *GetNameSafe(Owner));
		return;
	}

	// 注册属性变化回调
	MaxWalkSpeedChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		MovementAttributeSet->GetMaxWalkSpeedAttribute()
	).AddUObject(this, &ThisClass::OnMaxWalkSpeedChanged);

	MaxWalkSpeedMultiplierChangedHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
		MovementAttributeSet->GetMaxWalkSpeedMultiplierAttribute()
	).AddUObject(this, &ThisClass::OnMaxWalkSpeedMultiplierChanged);

	// 初始化时同步一次
	UpdateMovementSpeed();
}

void UYcMovementSyncComponent::DoUninitializeFromAbilitySystem()
{
	// 清理属性变化监听
	if (AbilitySystemComponent && MovementAttributeSet)
	{
		if (MaxWalkSpeedChangedHandle.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
				MovementAttributeSet->GetMaxWalkSpeedAttribute()
			).Remove(MaxWalkSpeedChangedHandle);
			MaxWalkSpeedChangedHandle.Reset();
		}

		if (MaxWalkSpeedMultiplierChangedHandle.IsValid())
		{
			AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(
				MovementAttributeSet->GetMaxWalkSpeedMultiplierAttribute()
			).Remove(MaxWalkSpeedMultiplierChangedHandle);
			MaxWalkSpeedMultiplierChangedHandle.Reset();
		}
	}

	MovementAttributeSet = nullptr;

	Super::DoUninitializeFromAbilitySystem();
}

void UYcMovementSyncComponent::OnMaxWalkSpeedChanged(const FOnAttributeChangeData& Data)
{
	UpdateMovementSpeed();
}

void UYcMovementSyncComponent::OnMaxWalkSpeedMultiplierChanged(const FOnAttributeChangeData& Data)
{
	UpdateMovementSpeed();
}

void UYcMovementSyncComponent::UpdateMovementSpeed()
{
	if (!CharacterMovementComponent || !MovementAttributeSet)
	{
		return;
	}

	// 计算最终速度 = 基础速度 * 倍率
	const float BaseSpeed = MovementAttributeSet->GetMaxWalkSpeed();
	const float Multiplier = MovementAttributeSet->GetMaxWalkSpeedMultiplier();
	const float FinalSpeed = BaseSpeed * Multiplier;

	// 更新CharacterMovementComponent
	CharacterMovementComponent->MaxWalkSpeed = FinalSpeed;
}
