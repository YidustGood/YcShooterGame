// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Abilities/YcGameplayAbility.h"
#include "YcAbilitySystemComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameplayAbility)

UYcGameplayAbility::UYcGameplayAbility(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
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