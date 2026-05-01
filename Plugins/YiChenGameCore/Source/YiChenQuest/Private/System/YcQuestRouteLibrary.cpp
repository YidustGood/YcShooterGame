// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcQuestRouteLibrary.h"

#include "System/YcQuestInstance.h"
#include "System/YcQuestSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcQuestRouteLibrary)

bool UYcQuestRouteLibrary::IsQuestInstanceKeyValid(const FYcQuestInstanceKey& InstanceKey)
{
    return InstanceKey.IsValid();
}

bool UYcQuestRouteLibrary::SubmitQuestEventToInstanceByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, const FYcQuestEvent& Event)
{
    if (UYcQuestSubsystem* QuestSubsystem = UYcQuestSubsystem::Get(WorldContextObject))
    {
        QuestSubsystem->ServerSubmitQuestEventToInstance(InstanceKey, Event);
        return true;
    }
    return false;
}

bool UYcQuestRouteLibrary::ServerCompleteQuestByInstanceByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, const FString& Detail)
{
    if (UYcQuestSubsystem* QuestSubsystem = UYcQuestSubsystem::Get(WorldContextObject))
    {
        return QuestSubsystem->ServerCompleteQuestByInstance(InstanceKey, Detail);
    }
    return false;
}

bool UYcQuestRouteLibrary::RequestQuestBundlesByInstanceByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, const EYcQuestPhase Phase)
{
    if (UYcQuestSubsystem* QuestSubsystem = UYcQuestSubsystem::Get(WorldContextObject))
    {
        return QuestSubsystem->RequestQuestBundlesByInstance(InstanceKey, Phase);
    }
    return false;
}

void UYcQuestRouteLibrary::ReleaseQuestBundlesByInstanceByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, const EYcQuestPhase Phase)
{
    if (UYcQuestSubsystem* QuestSubsystem = UYcQuestSubsystem::Get(WorldContextObject))
    {
        QuestSubsystem->ReleaseQuestBundlesByInstance(InstanceKey, Phase);
    }
}

bool UYcQuestRouteLibrary::ServerSetQuestReplicatedPayloadByInstanceByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, const FInstancedStruct& Payload)
{
    if (UYcQuestSubsystem* QuestSubsystem = UYcQuestSubsystem::Get(WorldContextObject))
    {
        return QuestSubsystem->ServerSetQuestReplicatedPayloadByInstance(InstanceKey, Payload);
    }
    return false;
}

bool UYcQuestRouteLibrary::GetQuestReplicatedPayloadByInstanceByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, FInstancedStruct& OutPayload)
{
    if (const UYcQuestSubsystem* QuestSubsystem = UYcQuestSubsystem::Get(WorldContextObject))
    {
        return QuestSubsystem->GetQuestReplicatedPayloadByInstance(InstanceKey, OutPayload);
    }
    OutPayload.Reset();
    return false;
}

bool UYcQuestRouteLibrary::GetQuestPublicProgressByInstanceByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, TArray<FYcQuestPublicProgress>& OutProgress)
{
    if (const UYcQuestSubsystem* QuestSubsystem = UYcQuestSubsystem::Get(WorldContextObject))
    {
        return QuestSubsystem->GetQuestPublicProgressByInstance(InstanceKey, OutProgress);
    }
    OutProgress.Reset();
    return false;
}

bool UYcQuestRouteLibrary::ServerUpdateSharedQuestMembersByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, const TArray<FString>& MemberIds)
{
    if (UYcQuestSubsystem* QuestSubsystem = UYcQuestSubsystem::Get(WorldContextObject))
    {
        return QuestSubsystem->ServerUpdateSharedQuestMembers(InstanceKey, MemberIds);
    }
    return false;
}

bool UYcQuestRouteLibrary::IsPlayerInSharedQuestByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, const FString& PlayerId)
{
    if (const UYcQuestSubsystem* QuestSubsystem = UYcQuestSubsystem::Get(WorldContextObject))
    {
        return QuestSubsystem->IsPlayerInSharedQuest(InstanceKey, PlayerId);
    }
    return false;
}

bool UYcQuestRouteLibrary::GetSharedQuestMembersByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, TArray<FString>& OutMemberIds)
{
    if (const UYcQuestSubsystem* QuestSubsystem = UYcQuestSubsystem::Get(WorldContextObject))
    {
        return QuestSubsystem->GetSharedQuestMembers(InstanceKey, OutMemberIds);
    }
    OutMemberIds.Reset();
    return false;
}

void UYcQuestRouteLibrary::GetLocalQuestInstancesByScope(UObject* WorldContextObject, const EYcQuestScope Scope, TArray<UYcQuestInstance*>& OutInstances)
{
    OutInstances.Reset();
    const UYcQuestSubsystem* QuestSubsystem = UYcQuestSubsystem::Get(WorldContextObject);
    if (!QuestSubsystem)
    {
        return;
    }

    TArray<FYcQuestRuntimeSnapshot> Snapshots;
    QuestSubsystem->GetRuntimeQuestSnapshots(Snapshots);
    for (const FYcQuestRuntimeSnapshot& Snapshot : Snapshots)
    {
        if (Snapshot.InstanceKey.Scope != Scope)
        {
            continue;
        }

        UYcQuestInstance* Instance = nullptr;
        if (QuestSubsystem->GetQuestInstanceByKey(Snapshot.InstanceKey, Instance) && Instance)
        {
            OutInstances.Add(Instance);
        }
    }
}

bool UYcQuestRouteLibrary::GetQuestInstanceByKeyByScope(UObject* WorldContextObject, const FYcQuestInstanceKey& InstanceKey, UYcQuestInstance*& OutInstance)
{
    OutInstance = nullptr;
    if (const UYcQuestSubsystem* QuestSubsystem = UYcQuestSubsystem::Get(WorldContextObject))
    {
        return QuestSubsystem->GetQuestInstanceByKey(InstanceKey, OutInstance);
    }
    return false;
}
