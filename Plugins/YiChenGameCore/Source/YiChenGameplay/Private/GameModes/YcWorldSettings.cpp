// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameModes/YcWorldSettings.h"

#include "YiChenGameplay.h"
#include "Engine/AssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcWorldSettings)

AYcWorldSettings::AYcWorldSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

AYcWorldSettings* AYcWorldSettings::GetYcWorldSettings(const UObject* WorldContent)
{
	if(!IsValid(WorldContent)) return nullptr;

	return Cast<AYcWorldSettings>(WorldContent->GetWorld()->GetWorldSettings());
}

FPrimaryAssetId AYcWorldSettings::GetDefaultGameplayExperience() const
{
	FPrimaryAssetId Result;
	if (!DefaultGameplayExperience.IsNull())
	{
		Result = UAssetManager::Get().GetPrimaryAssetIdForPath(DefaultGameplayExperience.ToSoftObjectPath());

		if (!Result.IsValid())
		{
			UE_LOG(LogYcGameplay, Error,
				   TEXT(
					   "%s.DefaultGameplayExperience is %s but that failed to resolve into an asset ID (you might need to add a path to the Asset Rules in your game feature plugin or project settings"
				   ),
				   *GetPathNameSafe(this), *DefaultGameplayExperience.ToString());
		}
	}
	return Result;
}

#if WITH_EDITOR
void AYcWorldSettings::CheckForErrors()
{
	Super::CheckForErrors();
	// @TODO 地图错误检查-检查地图中的APlayerStart是否为AYcPlayerStart类型
}
#endif