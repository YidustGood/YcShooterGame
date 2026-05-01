// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcQuestInstance.h"

#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"
#include "Utils/CommonSimpleUtil.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcQuestInstance)

UYcQuestInstance::UYcQuestInstance(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    LastUpdatedUnixTime = YcTimeUtils::GetUtcNowUnixTimestampSeconds();
}

void UYcQuestInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    FDoRepLifetimeParams Params;
    Params.bIsPushBased = true;

    DOREPLIFETIME_WITH_PARAMS_FAST(UYcQuestInstance, InstanceKey, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(UYcQuestInstance, QuestId, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(UYcQuestInstance, State, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(UYcQuestInstance, Version, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(UYcQuestInstance, LastUpdatedUnixTime, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(UYcQuestInstance, ReplicatedPayload, Params);
}

void UYcQuestInstance::InitializeInstance(const FYcQuestInstanceKey& InInstanceKey, const FName InQuestId, const EYcQuestState InInitialState)
{
    InstanceKey = InInstanceKey;
    QuestId = InQuestId;
    State = InInitialState;
    Version = 1;
    ReplicatedPayload.Reset();
    LastUpdatedUnixTime = YcTimeUtils::GetUtcNowUnixTimestampSeconds();

    MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuestInstance, InstanceKey, this);
    MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuestInstance, QuestId, this);
    MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuestInstance, State, this);
    MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuestInstance, Version, this);
    MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuestInstance, LastUpdatedUnixTime, this);
    MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuestInstance, ReplicatedPayload, this);
}

void UYcQuestInstance::RestoreFromSnapshot(const FYcQuestInstanceKey& InInstanceKey, const FName InQuestId, const EYcQuestState InState, const int32 InVersion, const int64 InLastUpdatedUnixTime, const FInstancedStruct& InReplicatedPayload)
{
    InstanceKey = InInstanceKey;
    QuestId = InQuestId;
    State = InState;
    Version = FMath::Max(1, InVersion);
    LastUpdatedUnixTime = FMath::Max<int64>(0, InLastUpdatedUnixTime);
    ReplicatedPayload = InReplicatedPayload;

    MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuestInstance, InstanceKey, this);
    MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuestInstance, QuestId, this);
    MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuestInstance, State, this);
    MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuestInstance, Version, this);
    MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuestInstance, LastUpdatedUnixTime, this);
    MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuestInstance, ReplicatedPayload, this);
}

void UYcQuestInstance::SetState(const EYcQuestState InState, const FString& Detail)
{
    (void)Detail;
    if (State == InState)
    {
        return;
    }

    State = InState;
    UpdateTimestampAndVersion();
    MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuestInstance, State, this);
}

void UYcQuestInstance::SetReplicatedPayload(const FInstancedStruct& InPayload)
{
    ReplicatedPayload = InPayload;
    UpdateTimestampAndVersion();
    MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuestInstance, ReplicatedPayload, this);
}

void UYcQuestInstance::TouchVersion()
{
    UpdateTimestampAndVersion();
}

const FYcQuestInstanceKey& UYcQuestInstance::GetInstanceKey() const
{
    return InstanceKey;
}

FName UYcQuestInstance::GetQuestId() const
{
    return QuestId;
}

EYcQuestState UYcQuestInstance::GetState() const
{
    return State;
}

int32 UYcQuestInstance::GetVersion() const
{
    return Version;
}

int64 UYcQuestInstance::GetLastUpdatedUnixTime() const
{
    return LastUpdatedUnixTime;
}

const FInstancedStruct& UYcQuestInstance::GetReplicatedPayload() const
{
    return ReplicatedPayload;
}

bool UYcQuestInstance::IsActiveState() const
{
    return State == EYcQuestState::Accepted || State == EYcQuestState::InProgress;
}

void UYcQuestInstance::SetRuntimeRootObjective(UYcQuestObjective* InRootObjective)
{
    RuntimeRootObjective = InRootObjective;
}

UYcQuestObjective* UYcQuestInstance::GetRuntimeRootObjective() const
{
    return RuntimeRootObjective;
}

void UYcQuestInstance::SetRuntimeQuestEffects(const TArray<TObjectPtr<UYcQuestEffect>>& InQuestEffects)
{
    RuntimeQuestEffects = InQuestEffects;
}

const TArray<TObjectPtr<UYcQuestEffect>>& UYcQuestInstance::GetRuntimeQuestEffects() const
{
    return RuntimeQuestEffects;
}

void UYcQuestInstance::UpdateTimestampAndVersion()
{
    ++Version;
    LastUpdatedUnixTime = YcTimeUtils::GetUtcNowUnixTimestampSeconds();
    MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuestInstance, Version, this);
    MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuestInstance, LastUpdatedUnixTime, this);
}
