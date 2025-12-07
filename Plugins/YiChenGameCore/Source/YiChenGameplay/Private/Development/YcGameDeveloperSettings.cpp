// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Development/YcGameDeveloperSettings.h"

#include "Framework/Notifications/NotificationManager.h"
#include "Widgets/Notifications/SNotificationList.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameDeveloperSettings)

#define LOCTEXT_NAMESPACE "YcCheats"

UYcGameDeveloperSettings::UYcGameDeveloperSettings()
{
}

FName UYcGameDeveloperSettings::GetCategoryName() const
{
	return FApp::GetProjectName();
}

#if WITH_EDITOR
void UYcGameDeveloperSettings::OnPlayInEditorStarted() const
{
	// Show a notification toast to remind the user that there's an experience override set
	if (ExperienceOverride.IsValid())
	{
		FNotificationInfo Info(FText::Format(
			LOCTEXT("ExperienceOverrideActive", "Developer Settings Override\nExperience {0}"),
			FText::FromName(ExperienceOverride.PrimaryAssetName)
		));
		Info.ExpireDuration = 2.0f;
		FSlateNotificationManager::Get().AddNotification(Info);
	}
}

void UYcGameDeveloperSettings::ApplySettings()
{
}

void UYcGameDeveloperSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	ApplySettings();
}

void UYcGameDeveloperSettings::PostReloadConfig(FProperty* PropertyThatWasLoaded)
{
	Super::PostReloadConfig(PropertyThatWasLoaded);

	ApplySettings();
}

void UYcGameDeveloperSettings::PostInitProperties()
{
	Super::PostInitProperties();

	ApplySettings();
}
#endif

#undef LOCTEXT_NAMESPACE