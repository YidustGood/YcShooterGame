// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Player/YcPlayerController.h"

#include "AbilitySystemGlobals.h"
#include "EngineUtils.h"
#include "YcAbilitySystemComponent.h"
#include "Player/YcCheatManager.h"
#include "Player/YcPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcPlayerController)

AYcPlayerController::AYcPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	CheatClass = UYcCheatManager::StaticClass();
}

AYcPlayerState* AYcPlayerController::GetYcPlayerState() const
{
	return CastChecked<AYcPlayerState>(PlayerState, ECastCheckedType::NullAllowed);
}

UYcAbilitySystemComponent* AYcPlayerController::GetYcAbilitySystemComponent() const
{
	const AYcPlayerState* PS = GetYcPlayerState();
	return (PS ? PS->GetYcAbilitySystemComponent() : nullptr);
}

void AYcPlayerController::PreProcessInput(const float DeltaTime, const bool bGamePaused)
{
	Super::PreProcessInput(DeltaTime, bGamePaused);
}

void AYcPlayerController::PostProcessInput(const float DeltaTime, const bool bGamePaused)
{
	// 调用ASC->ProcessAbilityInput处理对的技能输入
	if (UYcAbilitySystemComponent* ASC = GetYcAbilitySystemComponent())
	{
		ASC->ProcessAbilityInput(DeltaTime, bGamePaused);
	}

	Super::PostProcessInput(DeltaTime, bGamePaused);
}

void AYcPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
}

void AYcPlayerController::OnUnPossess()
{
	/**
	 * 确保被解除控制的Pawn不会继续作为我们技能系统组件（ASC）的Avatar
	 * ASC挂载在PlayerState上的生命周期比Pawn长, 所以必须处理, 否则会出现指向不受我们控制的Pawn
	 * 例如我们解除了对某个Pawn的控制, 但还没控制新的Pawn, 如果此时不清除可能会错误的影响已经不受我们操控的Pawn
	 */
	if (const APawn* PawnBeingUnpossessed = GetPawn())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState))
		{
			if (ASC->GetAvatarActor() == PawnBeingUnpossessed)
			{
				ASC->SetAvatarActor(nullptr);
			}
		}
	}
	Super::OnUnPossess();
}

void AYcPlayerController::ServerCheatAll_Implementation(const FString& Msg)
{
#if USING_CHEAT_MANAGER
	if (CheatManager)
	{
		UE_LOG(LogYcGameCheat, Warning, TEXT("ServerCheat: %s"), *Msg);
		ClientMessage(ConsoleCommand(Msg));
	}
#endif // #if USING_CHEAT_MANAGER
}

bool AYcPlayerController::ServerCheatAll_Validate(const FString& Msg)
{
	return true;
}

void AYcPlayerController::ServerCheat_Implementation(const FString& Msg)
{
#if USING_CHEAT_MANAGER
	if (CheatManager)
	{
		UE_LOG(LogYcGameCheat, Warning, TEXT("ServerCheatAll: %s"), *Msg);
		for (TActorIterator<AYcPlayerController> It(GetWorld()); It; ++It)
		{
			AYcPlayerController* YcPC = (*It);
			if (YcPC)
			{
				YcPC->ClientMessage(YcPC->ConsoleCommand(Msg));
			}
		}
	}
#endif // #if USING_CHEAT_MANAGER
}

bool AYcPlayerController::ServerCheat_Validate(const FString& Msg)
{
	return true;
}