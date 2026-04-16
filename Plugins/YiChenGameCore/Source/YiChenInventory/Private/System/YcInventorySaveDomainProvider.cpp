// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcInventorySaveDomainProvider.h"

#include "System/YcInventorySceneContext.h"
#include "System/YcMetaInventorySubsystem.h"
#include "System/YcMetaInventoryTypes.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcInventorySaveDomainProvider)

FName UYcInventorySaveDomainProvider::DomainKey(TEXT("Inventory"));

FName UYcInventorySaveDomainProvider::GetDomainKey() const
{
    return DomainKey;
}

int32 UYcInventorySaveDomainProvider::GetDomainVersion() const
{
    return 1;
}

bool UYcInventorySaveDomainProvider::CanHandleContext(const UObject* ContextObject) const
{
    // 仅处理库存场景上下文，避免误参与其它业务域流程。
    return Cast<UYcInventorySceneContext>(ContextObject) != nullptr;
}

bool UYcInventorySaveDomainProvider::BuildDomainPayload(const UObject* ContextObject, TArray<uint8>& OutPayloadBytes, FString& OutReason) const
{
    const UYcInventorySceneContext* SceneContext = Cast<UYcInventorySceneContext>(ContextObject);
    if (!IsValid(SceneContext))
    {
        OutReason = TEXT("ContextObject is not UYcInventorySceneContext.");
        return false;
    }

    UYcMetaInventorySubsystem* MetaSubsystem = UYcMetaInventorySubsystem::Get(SceneContext);
    if (!MetaSubsystem)
    {
        OutReason = TEXT("UYcMetaInventorySubsystem is not available.");
        return false;
    }

    FYcMetaInventoryRootSnapshot Snapshot;
    if (!MetaSubsystem->BuildSnapshotFromContext(const_cast<UYcInventorySceneContext*>(SceneContext), Snapshot))
    {
        OutReason = TEXT("BuildSnapshotFromContext failed.");
        return false;
    }

    OutPayloadBytes.Reset();
    // 持久化场景必须使用“对象/名称字符串代理归档”，避免写入进程内指针值。
    FMemoryWriter MemWriter(OutPayloadBytes, true);
    FObjectAndNameAsStringProxyArchive ArWriter(MemWriter, false);
    FYcMetaInventoryRootSnapshot::StaticStruct()->SerializeItem(ArWriter, &Snapshot, nullptr);
    if (ArWriter.IsError())
    {
        OutReason = TEXT("Serialize snapshot bytes failed.");
        return false;
    }
    OutReason.Reset();
    return true;
}

bool UYcInventorySaveDomainProvider::ApplyDomainPayload(UObject* ContextObject, const TArray<uint8>& PayloadBytes, FString& OutReason) const
{
    UYcInventorySceneContext* SceneContext = Cast<UYcInventorySceneContext>(ContextObject);
    if (!IsValid(SceneContext))
    {
        OutReason = TEXT("ContextObject is not UYcInventorySceneContext.");
        return false;
    }

    UYcMetaInventorySubsystem* MetaSubsystem = UYcMetaInventorySubsystem::Get(SceneContext);
    if (!MetaSubsystem)
    {
        OutReason = TEXT("UYcMetaInventorySubsystem is not available.");
        return false;
    }

    FYcMetaInventoryRootSnapshot Snapshot;
    TArray<uint8> Buffer = PayloadBytes;
    FMemoryReader MemReader(Buffer, true);
    FObjectAndNameAsStringProxyArchive ArReader(MemReader, true);
    FYcMetaInventoryRootSnapshot::StaticStruct()->SerializeItem(ArReader, &Snapshot, nullptr);
    if (ArReader.IsError())
    {
        OutReason = TEXT("Deserialize snapshot bytes failed.");
        return false;
    }

    if (!MetaSubsystem->ApplySnapshotToContext(SceneContext, Snapshot))
    {
        OutReason = TEXT("ApplySnapshotToContext failed.");
        return false;
    }

    OutReason.Reset();
    return true;
}
