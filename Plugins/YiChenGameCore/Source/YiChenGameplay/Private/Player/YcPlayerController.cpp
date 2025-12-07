// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Player/YcPlayerController.h"
#include "Player/YcPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcPlayerController)

AYcPlayerController::AYcPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
}

AYcPlayerState* AYcPlayerController::GetYcPlayerState() const
{
	return CastChecked<AYcPlayerState>(PlayerState, ECastCheckedType::NullAllowed);
}