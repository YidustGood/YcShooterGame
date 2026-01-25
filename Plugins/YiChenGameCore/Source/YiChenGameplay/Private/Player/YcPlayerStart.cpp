// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Player/YcPlayerStart.h"

#include "GameFramework/GameModeBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcPlayerStart)

AYcPlayerStart::AYcPlayerStart(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

EYcPlayerStartLocationOccupancy AYcPlayerStart::GetLocationOccupancy(AController* const ControllerPawnToFit) const
{
	UWorld* const World = GetWorld();
	if (HasAuthority() && World)
	{
		if (AGameModeBase* AuthGameMode = World->GetAuthGameMode())
		{
			// 获取控制器对应的Pawn类
			const TSubclassOf<APawn> PawnClass = AuthGameMode->GetDefaultPawnClassForController(ControllerPawnToFit);
			const APawn* const PawnToFit = PawnClass ? GetDefault<APawn>(PawnClass) : nullptr;

			FVector ActorLocation = GetActorLocation();

			// 检查是否有阻挡几何体
			if (const FRotator ActorRotation = GetActorRotation(); !World->EncroachingBlockingGeometry(PawnToFit, ActorLocation, ActorRotation, nullptr))
			{
				return EYcPlayerStartLocationOccupancy::Empty;
			}
			// 尝试寻找传送点
			else if (World->FindTeleportSpot(PawnToFit, ActorLocation, ActorRotation))
			{
				return EYcPlayerStartLocationOccupancy::Partial;
			}
		}
	}

	return EYcPlayerStartLocationOccupancy::Full;
}

bool AYcPlayerStart::IsClaimed() const
{
	return ClaimingController != nullptr;
}

bool AYcPlayerStart::TryClaim(AController* OccupyingController)
{
	if (OccupyingController != nullptr && !IsClaimed())
	{
		ClaimingController = OccupyingController;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(ExpirationTimerHandle, FTimerDelegate::CreateUObject(this, &AYcPlayerStart::CheckUnclaimed), ExpirationCheckInterval, true);
		}
		return true;
	}
	return false;
}

void AYcPlayerStart::CheckUnclaimed()
{
	// 如果控制器有Pawn且出生点位置已空闲，则释放占用
	if (ClaimingController != nullptr && ClaimingController->GetPawn() != nullptr && GetLocationOccupancy(ClaimingController) == EYcPlayerStartLocationOccupancy::Empty)
	{
		ClaimingController = nullptr;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ExpirationTimerHandle);
		}
	}
}

