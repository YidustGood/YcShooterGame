// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Player/YcPlayerState.h"
#include "YcAbilitySystemComponent.h"
#include "YiChenGameplay.h"
#include "Player/YcPlayerController.h"
#include "Character/YcPawnData.h"
#include "GameModes/YcExperienceManagerComponent.h"
#include "GameModes/YcGameMode.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcPlayerState)

class UYcExperienceManagerComponent;

AYcPlayerState::AYcPlayerState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	
	// AbilitySystemComponent needs to be updated at a high frequency.
	SetNetUpdateFrequency(100.0f);
}

void AYcPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;
	DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, PawnData, SharedParams);
}

AYcPlayerController* AYcPlayerState::GetYcPlayerController() const
{
	return Cast<AYcPlayerController>(GetOwner());
}
void AYcPlayerState::SetPawnData(const UYcPawnData* InPawnData)
{
	check(InPawnData);

	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	if (PawnData)
	{
		UE_LOG(LogYcGameplay, Error, TEXT("Trying to set PawnData [%s] on player state [%s] that already has valid PawnData [%s]."), *GetNameSafe(InPawnData), *GetNameSafe(this), *GetNameSafe(PawnData));
		return;
	}

	MARK_PROPERTY_DIRTY_FROM_NAME(ThisClass, PawnData, this);
	PawnData = InPawnData;
	
	// @TODO 将PawnData中的AbilitySet授予PS的ASC

	ForceNetUpdate(); //强制更新到客户端(强制立刻同步)
}

void AYcPlayerState::PreInitializeComponents()
{
	Super::PreInitializeComponents();
}

void AYcPlayerState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	const UWorld* World = GetWorld();
	if (World && World->IsGameWorld() && World->GetNetMode() != NM_Client)
	{
		const AGameStateBase* GameState = GetWorld()->GetGameState();
		check(GameState);
		UYcExperienceManagerComponent* ExperienceComponent = GameState->FindComponentByClass<UYcExperienceManagerComponent>();
		check(ExperienceComponent);
		// 向ExperienceComponent注册OnExperienceLoaded的回调, 以响应游戏体验加载完成事件
		ExperienceComponent->CallOrRegister_OnExperienceLoaded(FOnYcExperienceLoaded::FDelegate::CreateUObject(this, &ThisClass::OnExperienceLoaded));
	}
}

void AYcPlayerState::OnExperienceLoaded(const UYcExperienceDefinition* CurrentExperience)
{
	check(CurrentExperience);
	
	if (const UYcPawnData* NewPawnData = CurrentExperience->DefaultPawnData)
	{
		SetPawnData(NewPawnData);
	}
	else
	{
		UE_LOG(LogYcGameplay, Error, TEXT("AYcPlayerState::OnExperienceLoaded(): Unable to find PawnData to initialize player state [%s]!"), *GetNameSafe(this));
	}
}

void AYcPlayerState::OnRep_PawnData()
{
}