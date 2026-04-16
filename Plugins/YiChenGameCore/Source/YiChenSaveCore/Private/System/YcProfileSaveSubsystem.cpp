// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcProfileSaveSubsystem.h"

#include "System/YcSaveBackend_LocalSaveGame.h"
#include "System/YcSaveDomainProvider.h"
#include "System/YcSaveDomainRegistry.h"
#include "YiChenSaveCore.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcProfileSaveSubsystem)

namespace
{
    // SaveCore 根协议版本号（用于全局快照级演进）。
    constexpr int32 CurrentSnapshotVersion = 1;

    int64 GetNowUnixTime()
    {
        const FDateTime UtcNow = FDateTime::UtcNow();
        const FDateTime UnixEpoch(1970, 1, 1);
        return (UtcNow - UnixEpoch).GetTotalSeconds();
    }
}

UYcProfileSaveSubsystem* UYcProfileSaveSubsystem::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    if (const UWorld* World = WorldContextObject->GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<UYcProfileSaveSubsystem>();
        }
    }
    return nullptr;
}

void UYcProfileSaveSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // 启动时预创建后端，避免首次读写时才懒加载。
    EnsureBackendProvider();
}

void UYcProfileSaveSubsystem::Deinitialize()
{
    FString Reason;
    // 退出前尽力落盘脏档，避免数据丢失。
    SaveDirtyProfilesSync(Reason);
    DirtyProfiles.Empty();
    ContextByProfile.Empty();
    BackendProvider = nullptr;
    Super::Deinitialize();
}

void UYcProfileSaveSubsystem::RegisterProfileContext(const FYcProfileKey& ProfileKey, UObject* ContextObject)
{
    if (!ProfileKey.IsValid() || !IsValid(ContextObject))
    {
        return;
    }

    ContextByProfile.Add(ProfileKey, ContextObject);
}

void UYcProfileSaveSubsystem::UnregisterProfileContext(const FYcProfileKey& ProfileKey, UObject* ContextObject)
{
    if (!ProfileKey.IsValid())
    {
        return;
    }

    if (const TObjectPtr<UObject>* Existing = ContextByProfile.Find(ProfileKey))
    {
        if (!ContextObject || Existing->Get() == ContextObject)
        {
            ContextByProfile.Remove(ProfileKey);
        }
    }
}

void UYcProfileSaveSubsystem::LoadProfileAsync(const FYcProfileKey& ProfileKey, const FYcOnProfileLoadCompleted& Completion)
{
    FString Reason;
    const bool bLoaded = LoadProfileSync(ProfileKey, Reason);
    Completion.ExecuteIfBound(bLoaded, Reason);
}

void UYcProfileSaveSubsystem::SaveProfileAsync(const FYcProfileKey& ProfileKey, const FYcOnProfileSaveCompleted& Completion)
{
    FString Reason;
    const bool bSaved = SaveProfileSync(ProfileKey, Reason);
    Completion.ExecuteIfBound(bSaved, Reason);
}

void UYcProfileSaveSubsystem::SaveDirtyProfilesAsync(const FYcOnProfileSaveCompleted& Completion)
{
    FString Reason;
    const bool bSaved = SaveDirtyProfilesSync(Reason);
    Completion.ExecuteIfBound(bSaved, Reason);
}

bool UYcProfileSaveSubsystem::LoadProfileSync(const FYcProfileKey& ProfileKey, FString& OutReason)
{
    if (!ProfileKey.IsValid())
    {
        OutReason = TEXT("Invalid profile key.");
        return false;
    }

    UObject* ContextObject = ResolveContext(ProfileKey);
    if (!IsValid(ContextObject))
    {
        OutReason = FString::Printf(TEXT("No context registered for profile '%s'."), *ProfileKey.ToDebugString());
        return false;
    }

    FYcProfileSaveRoot LoadedRoot;
    // 第一步：只从后端拿根对象。
    const EYcSaveBackendResult LoadResult = LoadProfileRootSync(ProfileKey, LoadedRoot, OutReason);
    if (LoadResult == EYcSaveBackendResult::NotFound)
    {
        OutReason = TEXT("Profile not found.");
        return false;
    }

    if (LoadResult != EYcSaveBackendResult::Success)
    {
        if (OutReason.IsEmpty())
        {
            OutReason = TEXT("Backend load failed.");
        }
        return false;
    }

    // 第二步：将根对象分发给各业务域并应用到 Context。
    if (!ApplyRootToContext(ProfileKey, ContextObject, LoadedRoot, OutReason))
    {
        return false;
    }

    ClearProfileDirty(ProfileKey);
    OutReason.Reset();
    return true;
}

bool UYcProfileSaveSubsystem::SaveProfileSync(const FYcProfileKey& ProfileKey, FString& OutReason)
{
    if (!ProfileKey.IsValid())
    {
        OutReason = TEXT("Invalid profile key.");
        return false;
    }

    UObject* ContextObject = ResolveContext(ProfileKey);
    if (!IsValid(ContextObject))
    {
        OutReason = FString::Printf(TEXT("No context registered for profile '%s'."), *ProfileKey.ToDebugString());
        return false;
    }

    FYcProfileSaveRoot Root;
    // 第一步：从 Context 汇总各业务域载荷。
    if (!BuildRootFromContext(ProfileKey, ContextObject, Root, OutReason))
    {
        return false;
    }

    // 第二步：将根对象写入后端。
    if (!SaveProfileRootSync(ProfileKey, Root, OutReason))
    {
        return false;
    }

    ClearProfileDirty(ProfileKey);
    OutReason.Reset();
    return true;
}

bool UYcProfileSaveSubsystem::SaveDirtyProfilesSync(FString& OutReason)
{
    bool bAllSucceeded = true;
    FString FirstReason;

    TArray<FYcProfileKey> DirtyKeys = DirtyProfiles.Array();
    // 使用快照数组遍历，避免遍历时集合变更。
    for (const FYcProfileKey& ProfileKey : DirtyKeys)
    {
        FString LocalReason;
        if (!SaveProfileSync(ProfileKey, LocalReason))
        {
            bAllSucceeded = false;
            if (FirstReason.IsEmpty())
            {
                FirstReason = FString::Printf(TEXT("Profile '%s' save failed: %s"), *ProfileKey.ToDebugString(), *LocalReason);
            }
        }
    }

    OutReason = FirstReason;
    return bAllSucceeded;
}

EYcSaveBackendResult UYcProfileSaveSubsystem::LoadProfileRootSync(const FYcProfileKey& ProfileKey, FYcProfileSaveRoot& OutRoot, FString& OutReason)
{
    if (!ProfileKey.IsValid())
    {
        OutReason = TEXT("Invalid profile key.");
        return EYcSaveBackendResult::Failed;
    }

    EnsureBackendProvider();
    if (!BackendProvider)
    {
        OutReason = TEXT("Backend provider is null.");
        return EYcSaveBackendResult::Failed;
    }

    // 当前为“异步接口 + 同步等待回调赋值”桥接模式。
    EYcSaveBackendResult LoadResult = EYcSaveBackendResult::Failed;
    FString BackendReason;
    BackendProvider->LoadProfileRootAsync(ProfileKey,
        FYcOnLoadProfileRoot::CreateLambda([&LoadResult, &OutRoot, &BackendReason](const EYcSaveBackendResult Result, const FYcProfileSaveRoot& Root, const FString& Reason)
        {
            LoadResult = Result;
            OutRoot = Root;
            BackendReason = Reason;
        }));

    OutReason = BackendReason;
    return LoadResult;
}

bool UYcProfileSaveSubsystem::SaveProfileRootSync(const FYcProfileKey& ProfileKey, const FYcProfileSaveRoot& Root, FString& OutReason)
{
    if (!ProfileKey.IsValid())
    {
        OutReason = TEXT("Invalid profile key.");
        return false;
    }

    EnsureBackendProvider();
    if (!BackendProvider)
    {
        OutReason = TEXT("Backend provider is null.");
        return false;
    }

    // 当前为“异步接口 + 同步等待回调赋值”桥接模式。
    bool bSaveSuccess = false;
    FString SaveReason;
    BackendProvider->SaveProfileRootAsync(ProfileKey, Root,
        FYcOnSaveProfileRoot::CreateLambda([&bSaveSuccess, &SaveReason](const bool bSuccess, const FString& Reason)
        {
            bSaveSuccess = bSuccess;
            SaveReason = Reason;
        }));

    OutReason = SaveReason;
    return bSaveSuccess;
}

void UYcProfileSaveSubsystem::MarkProfileDirty(const FYcProfileKey& ProfileKey)
{
    if (ProfileKey.IsValid())
    {
        DirtyProfiles.Add(ProfileKey);
    }
}

void UYcProfileSaveSubsystem::ClearProfileDirty(const FYcProfileKey& ProfileKey)
{
    if (ProfileKey.IsValid())
    {
        DirtyProfiles.Remove(ProfileKey);
    }
}

bool UYcProfileSaveSubsystem::IsProfileDirty(const FYcProfileKey& ProfileKey) const
{
    return DirtyProfiles.Contains(ProfileKey);
}

UObject* UYcProfileSaveSubsystem::ResolveContext(const FYcProfileKey& ProfileKey) const
{
    if (const TObjectPtr<UObject>* Found = ContextByProfile.Find(ProfileKey))
    {
        return Found->Get();
    }
    return nullptr;
}

void UYcProfileSaveSubsystem::EnsureBackendProvider()
{
    if (BackendProvider)
    {
        return;
    }

    UClass* BackendClass = nullptr;
    if (BackendProviderClass.IsValid())
    {
        BackendClass = BackendProviderClass.Get();
    }
    else if (!BackendProviderClass.IsNull())
    {
        BackendClass = BackendProviderClass.LoadSynchronous();
    }

    if (!BackendClass)
    {
        // 未配置时回退到本地 SaveGame 后端。
        BackendClass = UYcSaveBackend_LocalSaveGame::StaticClass();
    }

    BackendProvider = NewObject<UYcSaveBackendProvider>(this, BackendClass);
    if (!BackendProvider)
    {
        UE_LOG(LogYcSaveCore, Error, TEXT("Failed to create backend provider '%s'."), *GetNameSafe(BackendClass));
    }
}

bool UYcProfileSaveSubsystem::BuildRootFromContext(const FYcProfileKey& ProfileKey, UObject* ContextObject, FYcProfileSaveRoot& OutRoot, FString& OutReason) const
{
    OutRoot = FYcProfileSaveRoot();
    OutRoot.AccountId = ProfileKey.AccountId;
    OutRoot.ProfileId = ProfileKey.ProfileId;
    OutRoot.SnapshotVersion = CurrentSnapshotVersion;
    OutRoot.LastSavedUnixTime = GetNowUnixTime();

    TArray<const UYcSaveDomainProvider*> Providers;
    GatherDomainProviders(Providers);

    for (const UYcSaveDomainProvider* Provider : Providers)
    {
        if (!Provider)
        {
            continue;
        }
        if (!Provider->CanHandleContext(ContextObject))
        {
            // 仅让“可处理当前 Context”的域参与构建。
            continue;
        }

        TArray<uint8> Bytes;
        FString Reason;
        if (!Provider->BuildDomainPayload(ContextObject, Bytes, Reason))
        {
            OutReason = FString::Printf(TEXT("Domain '%s' build failed: %s"), *Provider->GetDomainKey().ToString(), *Reason);
            return false;
        }

        FYcProfileDomainPayload Payload;
        Payload.DomainKey = Provider->GetDomainKey();
        Payload.Version = Provider->GetDomainVersion();
        Payload.PayloadBytes = MoveTemp(Bytes);
        OutRoot.Domains.Add(MoveTemp(Payload));
    }

    OutRoot.Domains.Sort([](const FYcProfileDomainPayload& A, const FYcProfileDomainPayload& B)
    {
        // 固定顺序，避免同内容快照顺序抖动。
        return A.DomainKey.LexicalLess(B.DomainKey);
    });

    OutReason.Reset();
    return true;
}

bool UYcProfileSaveSubsystem::ApplyRootToContext(const FYcProfileKey& ProfileKey, UObject* ContextObject, const FYcProfileSaveRoot& Root, FString& OutReason) const
{
    if (Root.AccountId != ProfileKey.AccountId || Root.ProfileId != ProfileKey.ProfileId)
    {
        OutReason = FString::Printf(TEXT("Profile key mismatch. expected='%s', root='%s::%s'"), *ProfileKey.ToDebugString(), *Root.AccountId, *Root.ProfileId);
        return false;
    }

    TArray<const UYcSaveDomainProvider*> Providers;
    GatherDomainProviders(Providers);

    TMap<FName, const UYcSaveDomainProvider*> ProviderByDomain;
    for (const UYcSaveDomainProvider* Provider : Providers)
    {
        if (Provider && Provider->CanHandleContext(ContextObject))
        {
            ProviderByDomain.Add(Provider->GetDomainKey(), Provider);
        }
    }

    for (const FYcProfileDomainPayload& DomainPayload : Root.Domains)
    {
        const UYcSaveDomainProvider* const* ProviderPtr = ProviderByDomain.Find(DomainPayload.DomainKey);
        if (!ProviderPtr || !(*ProviderPtr))
        {
            const FString MissingReason = FString::Printf(TEXT("Missing provider for domain '%s'."), *DomainPayload.DomainKey.ToString());
            if (UnknownDomainPolicy == EYcSaveUnknownDomainPolicy::Strict)
            {
                // 严格模式：未知域直接失败。
                OutReason = MissingReason;
                return false;
            }

            // 宽松模式：跳过未知域，保证主流程可继续。
            UE_LOG(LogYcSaveCore, Warning, TEXT("ApplyRootToContext: %s profile='%s' policy=Lenient. Domain skipped."),
                *MissingReason, *ProfileKey.ToDebugString());
            continue;
        }

        const UYcSaveDomainProvider* Provider = *ProviderPtr;
        if (DomainPayload.Version != Provider->GetDomainVersion())
        {
            OutReason = FString::Printf(TEXT("Domain '%s' version mismatch: payload=%d provider=%d"),
                *DomainPayload.DomainKey.ToString(),
                DomainPayload.Version,
                Provider->GetDomainVersion());
            return false;
        }

        FString ApplyReason;
        if (!Provider->ApplyDomainPayload(ContextObject, DomainPayload.PayloadBytes, ApplyReason))
        {
            OutReason = FString::Printf(TEXT("Domain '%s' apply failed: %s"), *DomainPayload.DomainKey.ToString(), *ApplyReason);
            return false;
        }
    }

    OutReason.Reset();
    return true;
}

void UYcProfileSaveSubsystem::GatherDomainProviders(TArray<const UYcSaveDomainProvider*>& OutProviders) const
{
    OutProviders.Empty();

    // 从注册表读取 Provider 类型，并转为 CDO 供只读调用。
    const TArray<TSubclassOf<UYcSaveDomainProvider>> ProviderClasses = YcSaveDomainRegistry::GetRegisteredProviderClasses();
    for (const TSubclassOf<UYcSaveDomainProvider> ProviderClass : ProviderClasses)
    {
        if (!ProviderClass)
        {
            continue;
        }

        if (const UYcSaveDomainProvider* ProviderCDO = Cast<UYcSaveDomainProvider>(ProviderClass->GetDefaultObject()))
        {
            OutProviders.Add(ProviderCDO);
        }
    }
}
