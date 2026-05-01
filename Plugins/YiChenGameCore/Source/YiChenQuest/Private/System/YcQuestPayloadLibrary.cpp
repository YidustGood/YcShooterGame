// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcQuestPayloadLibrary.h"
#include "System/YcQuestInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcQuestPayloadLibrary)

FInstancedStruct UYcQuestPayloadLibrary::MakeAreaPresenceSyncPayload(const int32 PlayersInRange, const int32 ActiveSeconds)
{
    FYcQuestAreaPresenceSyncPayload Value;
    Value.PlayersInRange = FMath::Max(0, PlayersInRange);
    Value.ActiveSeconds = FMath::Max(0, ActiveSeconds);

    FInstancedStruct Payload;
    Payload.InitializeAs<FYcQuestAreaPresenceSyncPayload>(Value);
    return Payload;
}

bool UYcQuestPayloadLibrary::ReadAreaPresenceSyncPayload(const FInstancedStruct& Payload, int32& OutPlayersInRange, int32& OutActiveSeconds)
{
    OutPlayersInRange = 0;
    OutActiveSeconds = 0;

    if (!Payload.IsValid() || Payload.GetScriptStruct() != FYcQuestAreaPresenceSyncPayload::StaticStruct())
    {
        return false;
    }

    const FYcQuestAreaPresenceSyncPayload* Value = Payload.GetPtr<FYcQuestAreaPresenceSyncPayload>();
    if (!Value)
    {
        return false;
    }

    OutPlayersInRange = FMath::Max(0, Value->PlayersInRange);
    OutActiveSeconds = FMath::Max(0, Value->ActiveSeconds);
    return true;
}

bool UYcQuestPayloadLibrary::ReadAreaPresenceSyncPayloadFromQuestInstance(const UYcQuestInstance* QuestInstance, int32& OutPlayersInRange, int32& OutActiveSeconds)
{
    if (!QuestInstance)
    {
        OutPlayersInRange = 0;
        OutActiveSeconds = 0;
        return false;
    }

    return ReadAreaPresenceSyncPayload(QuestInstance->GetReplicatedPayload(), OutPlayersInRange, OutActiveSeconds);
}
