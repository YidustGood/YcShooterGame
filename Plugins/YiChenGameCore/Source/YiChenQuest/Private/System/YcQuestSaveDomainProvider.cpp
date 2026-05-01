// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcQuestSaveDomainProvider.h"

#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "System/YcQuestSubsystem.h"
#include "YcQuestTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcQuestSaveDomainProvider)

FName UYcQuestSaveDomainProvider::DomainKey(TEXT("Quest"));

FName UYcQuestSaveDomainProvider::GetDomainKey() const
{
    return DomainKey;
}

int32 UYcQuestSaveDomainProvider::GetDomainVersion() const
{
    return 2;
}

bool UYcQuestSaveDomainProvider::CanHandleContext(const UObject* ContextObject) const
{
    return Cast<UYcQuestSubsystem>(ContextObject) != nullptr;
}

bool UYcQuestSaveDomainProvider::BuildDomainPayload(const UObject* ContextObject, TArray<uint8>& OutPayloadBytes, FString& OutReason) const
{
    const UYcQuestSubsystem* QuestSubsystem = Cast<UYcQuestSubsystem>(ContextObject);
    if (!QuestSubsystem)
    {
        OutReason = TEXT("ContextObject is not UYcQuestSubsystem.");
        return false;
    }

    FYcQuestSaveSnapshot Snapshot;
    if (!QuestSubsystem->BuildSaveSnapshot(Snapshot))
    {
        OutReason = TEXT("BuildSaveSnapshot failed.");
        return false;
    }

    OutPayloadBytes.Reset();
    FMemoryWriter MemWriter(OutPayloadBytes, true);
    FObjectAndNameAsStringProxyArchive ArWriter(MemWriter, false);
    FYcQuestSaveSnapshot::StaticStruct()->SerializeItem(ArWriter, &Snapshot, nullptr);
    if (ArWriter.IsError())
    {
        OutReason = TEXT("Serialize quest snapshot failed.");
        return false;
    }

    OutReason.Reset();
    return true;
}

bool UYcQuestSaveDomainProvider::ApplyDomainPayload(UObject* ContextObject, const TArray<uint8>& PayloadBytes, FString& OutReason) const
{
    UYcQuestSubsystem* QuestSubsystem = Cast<UYcQuestSubsystem>(ContextObject);
    if (!QuestSubsystem)
    {
        OutReason = TEXT("ContextObject is not UYcQuestSubsystem.");
        return false;
    }

    FYcQuestSaveSnapshot Snapshot;
    TArray<uint8> Buffer = PayloadBytes;
    FMemoryReader MemReader(Buffer, true);
    FObjectAndNameAsStringProxyArchive ArReader(MemReader, true);
    FYcQuestSaveSnapshot::StaticStruct()->SerializeItem(ArReader, &Snapshot, nullptr);
    if (ArReader.IsError())
    {
        OutReason = TEXT("Deserialize quest snapshot failed.");
        return false;
    }

    if (!QuestSubsystem->ApplySaveSnapshot(Snapshot))
    {
        return false;
    }

    OutReason.Reset();
    return true;
}


