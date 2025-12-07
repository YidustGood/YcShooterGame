// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Player/YcPlayerController.h"

#include "YcAbilitySystemComponent.h"
#include "Player/YcPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcPlayerController)

AYcPlayerController::AYcPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
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