// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "GameplayCommon/YcQuestMessageTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcQuestMessageTypes)

namespace YcQuestGameplayTags
{
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Quest_Asset_BundleLoading, "Yc.Quest.Asset.BundleLoading", "Quest bundle loading started.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Quest_Asset_BundleReady, "Yc.Quest.Asset.BundleReady", "Quest bundle load completed.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Quest_Asset_BundleFailed, "Yc.Quest.Asset.BundleFailed", "Quest bundle load failed.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Quest_Accept_Rejected, "Yc.Quest.Accept.Rejected", "Quest accept request rejected.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Quest_State_Changed, "Yc.Quest.State.Changed", "Quest state changed.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Quest_Business_StateChanged, "Yc.Quest.Business.StateChanged", "Quest state changed for business layer extension.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Quest_Demo_Countdown_Started, "Yc.Quest.Demo.Countdown.Started", "Quest demo countdown started.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Quest_Demo_Countdown_Updated, "Yc.Quest.Demo.Countdown.Updated", "Quest demo countdown updated.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Quest_Demo_Countdown_Finished, "Yc.Quest.Demo.Countdown.Finished", "Quest demo countdown finished.");
    UE_DEFINE_GAMEPLAY_TAG_COMMENT(Quest_Demo_Event_CountdownFinished, "Yc.Quest.Demo.Event.CountdownFinished", "Quest demo completion event.");
}
