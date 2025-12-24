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
	// 在 Project Settings 中使用工程名作为该设置页的分类名称
	return FApp::GetProjectName();
}

#if WITH_EDITOR
void UYcGameDeveloperSettings::OnPlayInEditorStarted() const
{
	// 当设置了体验覆盖时，在进入 PIE 时弹出提示通知，提醒开发者当前使用的是覆盖体验
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
	// TODO: 在此处根据配置同步和应用对应的控制台变量或运行时设置
}

void UYcGameDeveloperSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// 编辑器中修改配置后立即应用设置
	ApplySettings();
}

void UYcGameDeveloperSettings::PostReloadConfig(FProperty* PropertyThatWasLoaded)
{
	Super::PostReloadConfig(PropertyThatWasLoaded);

	// 从配置文件重新加载后应用设置
	ApplySettings();
}

void UYcGameDeveloperSettings::PostInitProperties()
{
	Super::PostInitProperties();

	// 对象初始化完成后应用一次当前配置
	ApplySettings();
}
#endif

#undef LOCTEXT_NAMESPACE