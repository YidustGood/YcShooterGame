// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Abilities/YcGameplayAbility.h"
#include "YcAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameplayAbility)

UYcGameplayAbility::UYcGameplayAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
}

void UYcGameplayAbility::OnGiveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	Super::OnGiveAbility(ActorInfo, Spec);

	K2_OnAbilityAdded();// 转发调用蓝图实现的函数

	TryActivateAbilityOnSpawn(ActorInfo, Spec); // 技能添加后调用尝试激活, 内部会进行条件判断
}

void UYcGameplayAbility::OnRemoveAbility(const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilitySpec& Spec)
{
	K2_OnAbilityRemoved();// 转发调用蓝图实现的函数

	Super::OnRemoveAbility(ActorInfo, Spec);
}

UYcAbilitySystemComponent* UYcGameplayAbility::GetYcAbilitySystemComponentFromActorInfo() const
{
	return (CurrentActorInfo ? Cast<UYcAbilitySystemComponent>(CurrentActorInfo->AbilitySystemComponent.Get()) : nullptr);
}

AController* UYcGameplayAbility::GetControllerFromActorInfo() const
{
	if (!CurrentActorInfo) return nullptr;
	
	if (AController* PC = CurrentActorInfo->PlayerController.Get())
	{
		return PC;
	}

	// 在所有者链中寻找玩家控制器或Pawn
	AActor* TestActor = CurrentActorInfo->OwnerActor.Get();
	while (TestActor)
	{
		if (AController* C = Cast<AController>(TestActor))
		{
			return C;
		}

		if (const APawn* Pawn = Cast<APawn>(TestActor))
		{
			return Pawn->GetController();
		}
		
		TestActor = TestActor->GetOwner();
	}

	return nullptr;
}

void UYcGameplayAbility::OnPawnAvatarSet()
{
	K2_OnPawnAvatarSet();
}

void UYcGameplayAbility::TryActivateAbilityOnSpawn(const FGameplayAbilityActorInfo* ActorInfo,
                                                   const FGameplayAbilitySpec& Spec) const
{
	const bool bIsPredicting = (Spec.ActivationInfo.ActivationMode == EGameplayAbilityActivationMode::Predicting);

	// Try to activate if activation policy is on spawn.如果激活策略是OnSpawn尝试激活
	if (!ActorInfo || Spec.IsActive() || bIsPredicting || ActivationPolicy != EYcAbilityActivationPolicy::OnSpawn) return;
	
	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	const AActor* AvatarActor = ActorInfo->AvatarActor.Get();

	// If avatar actor is torn off or about to die, don't try to activate until we get the new one.
	// //如果avatar actor被GC标记或即将死亡，在我们得到新的actor之前不要尝试激活。
	if (ASC && AvatarActor && !AvatarActor->GetTearOff() && (AvatarActor->GetLifeSpan() <= 0.0f))
	{
		const bool bIsLocalExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalPredicted) || (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::LocalOnly);
		const bool bIsServerExecution = (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerOnly) || (NetExecutionPolicy == EGameplayAbilityNetExecutionPolicy::ServerInitiated);

		const bool bClientShouldActivate = ActorInfo->IsLocallyControlled() && bIsLocalExecution;
		const bool bServerShouldActivate = ActorInfo->IsNetAuthority() && bIsServerExecution;

		if (bClientShouldActivate || bServerShouldActivate)
		{
			ASC->TryActivateAbility(Spec.Handle);
		}
	}
}

void UYcGameplayAbility::ExternalEndAbility()
{
	check(CurrentActorInfo);

	bool bReplicateEndAbility = true;
	bool bWasCancelled = false;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEndAbility, bWasCancelled);
}