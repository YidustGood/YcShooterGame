// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameModes/YcExperienceManagerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcExperienceManagerSubsystem)

#if WITH_EDITOR

void UYcExperienceManagerSubsystem::OnPlayInEditorBegun()
{
	ensure(GameFeaturePluginRequestCountMap.IsEmpty());
	GameFeaturePluginRequestCountMap.Empty();
}

void UYcExperienceManagerSubsystem::NotifyOfPluginActivation(const FString PluginURL)
{
	/*如果我们在编辑器中，则为true。 请注意，使用“在编辑器中播放”时仍然如此。 在这种情况下要确定游戏是否开始，您可能需要使用 GWorld->HasBegunPlay。*/
	if (GIsEditor)
	{
		UYcExperienceManagerSubsystem* ExperienceManagerSubsystem = GEngine->GetEngineSubsystem<UYcExperienceManagerSubsystem>();
		check(ExperienceManagerSubsystem);

		// Track the number of requesters who activate this plugin. Multiple load/activation requests are always allowed because concurrent requests are handled.
		// 跟踪激活此插件的请求者数量。 由于会处理并发请求，因此始终允许多个加载/激活请求。
		int32& Count = ExperienceManagerSubsystem->GameFeaturePluginRequestCountMap.FindOrAdd(PluginURL);
		++Count;
	}
}

bool UYcExperienceManagerSubsystem::RequestToDeactivatePlugin(const FString PluginURL)
{
	if (GIsEditor)
	{
		UYcExperienceManagerSubsystem* ExperienceManagerSubsystem = GEngine->GetEngineSubsystem<UYcExperienceManagerSubsystem>();
		check(ExperienceManagerSubsystem);

		// Only let the last requester to get this far deactivate the plugin
		// 只让最后一个请求者走到这一步停用插件
		int32& Count = ExperienceManagerSubsystem->GameFeaturePluginRequestCountMap.FindChecked(PluginURL);
		--Count;

		if (Count == 0)
		{
			ExperienceManagerSubsystem->GameFeaturePluginRequestCountMap.Remove(PluginURL);
			return true;
		}

		return false;
	}

	return true;
}

#endif