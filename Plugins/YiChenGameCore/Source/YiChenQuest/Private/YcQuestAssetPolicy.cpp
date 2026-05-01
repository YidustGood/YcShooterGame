// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcQuestAssetPolicy.h"

#include "YcQuestDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcQuestAssetPolicy)

void UYcQuestAssetPolicy::GetBundlesForPhase_Implementation(FName QuestId, const UYcQuestDefinition* QuestDefinition, EYcQuestPhase Phase, TArray<FName>& OutBundles) const
{
    (void)QuestId;
    OutBundles.Reset();

    if (QuestDefinition)
    {
        for (const FYcQuestPhaseBundleMapping& Mapping : QuestDefinition->BundlePolicy)
        {
            if (Mapping.Phase == Phase)
            {
                OutBundles.Append(Mapping.BundleNames);
            }
        }

        if (OutBundles.Num() > 0)
        {
            return;
        }
    }

    switch (Phase)
    {
    case EYcQuestPhase::OnAccepted:
        OutBundles = {TEXT("Preload"), TEXT("UI"), TEXT("Audio")};
        break;
    case EYcQuestPhase::OnStartedInMatch:
        OutBundles = {TEXT("InMatch")};
        break;
    case EYcQuestPhase::OnReturnOutOfMatch:
        OutBundles = {TEXT("OutOfMatch")};
        break;
    case EYcQuestPhase::OnCompleted:
    case EYcQuestPhase::OnFailed:
    case EYcQuestPhase::OnAborted:
        break;
    default:
        break;
    }
}

bool UYcQuestAssetPolicy::ShouldRetainBundleAfterComplete_Implementation(FName BundleName) const
{
    return BundleName == TEXT("UI");
}

