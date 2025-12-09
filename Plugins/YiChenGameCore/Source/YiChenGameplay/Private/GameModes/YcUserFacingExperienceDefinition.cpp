// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameModes/YcUserFacingExperienceDefinition.h"

#include "CommonSessionSubsystem.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcUserFacingExperienceDefinition)

UCommonSession_HostSessionRequest* UYcUserFacingExperienceDefinition::CreateHostingRequest() const
{
	const FString ExperienceName = ExperienceID.PrimaryAssetName.ToString();
	const FString UserFacingExperienceName = GetPrimaryAssetId().PrimaryAssetName.ToString();
	UCommonSession_HostSessionRequest* Result = NewObject<UCommonSession_HostSessionRequest>();
	Result->OnlineMode = ECommonSessionOnlineMode::Online;
	Result->bUseLobbies = true;
	Result->MapID = MapID;
	Result->ModeNameForAdvertisement = UserFacingExperienceName;
	Result->ExtraArgs = ExtraArgs;
	Result->ExtraArgs.Add(TEXT("Experience"), ExperienceName);
	Result->MaxPlayerCount = MaxPlayerCount;

	return Result;
}