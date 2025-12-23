// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcGamePhaseAbility.h"

#include "AbilitySystemComponent.h"
#include "Misc/DataValidation.h"
#include "YcGamePhaseSubsystem.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGamePhaseAbility)

#define LOCTEXT_NAMESPACE "UYcGamePhaseAbility"

UYcGamePhaseAbility::UYcGamePhaseAbility(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	ReplicationPolicy = EGameplayAbilityReplicationPolicy::ReplicateNo;
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerInitiated;
	NetSecurityPolicy = EGameplayAbilityNetSecurityPolicy::ServerOnly;
}

void UYcGamePhaseAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (ActorInfo->IsNetAuthority())
	{
		UWorld* World = ActorInfo->AbilitySystemComponent->GetWorld();
		UYcGamePhaseSubsystem* PhaseSubsystem = UWorld::GetSubsystem<UYcGamePhaseSubsystem>(World);
		PhaseSubsystem->OnBeginPhase(this, Handle);
	}

	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UYcGamePhaseAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	if (ActorInfo->IsNetAuthority())
	{
		UWorld* World = ActorInfo->AbilitySystemComponent->GetWorld();
		UYcGamePhaseSubsystem* PhaseSubsystem = UWorld::GetSubsystem<UYcGamePhaseSubsystem>(World);
		PhaseSubsystem->OnEndPhase(this, Handle);
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


#if WITH_EDITOR
EDataValidationResult UYcGamePhaseAbility::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = CombineDataValidationResults(Super::IsDataValid(Context), EDataValidationResult::Valid);

	if (!GamePhaseTag.IsValid())
	{
		Result = EDataValidationResult::Invalid;
		Context.AddError(LOCTEXT("GamePhaseTagNotSet", "GamePhaseTag must be set to a tag representing the current phase."));
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE