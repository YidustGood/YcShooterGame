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
    constexpr int32 CurrentSnapshotVersion = 2;

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
    EnsureBackendProvider();
}

void UYcProfileSaveSubsystem::Deinitialize()
{
    FString Reason;
    SaveDirtyProfilesSync(Reason);
    DirtyProfiles.Empty();
    ContextByProfile.Empty();
    BackendProvider = nullptr;
    Super::Deinitialize();
}

bool UYcProfileSaveSubsystem::BuildProfileKey(const FYcProfileIdentity& ProfileIdentity, FYcProfileSaveKey& OutProfileKey, FString& OutReason) const
{
    OutProfileKey = FYcProfileSaveKey(ProfileIdentity);
    if (!OutProfileKey.IsValid())
    {
        OutReason = FString::Printf(TEXT("Invalid profile identity: '%s'."), *ProfileIdentity.ToDebugString());
        return false;
    }

    OutReason.Reset();
    return true;
}

void UYcProfileSaveSubsystem::RegisterProfileContext(const FYcProfileIdentity& ProfileIdentity, UObject* ContextObject)
{
    FYcProfileSaveKey ProfileKey;
    FString Reason;
    if (!BuildProfileKey(ProfileIdentity, ProfileKey, Reason))
    {
        UE_LOG(LogYcSaveCore, Warning, TEXT("RegisterProfileContext failed: %s"), *Reason);
        return;
    }

    RegisterProfileContextByKey(ProfileKey, ContextObject);
}

void UYcProfileSaveSubsystem::UnregisterProfileContext(const FYcProfileIdentity& ProfileIdentity, UObject* ContextObject)
{
    FYcProfileSaveKey ProfileKey;
    FString Reason;
    if (!BuildProfileKey(ProfileIdentity, ProfileKey, Reason))
    {
        return;
    }

    UnregisterProfileContextByKey(ProfileKey, ContextObject);
}

void UYcProfileSaveSubsystem::LoadProfileAsync(const FYcProfileIdentity& ProfileIdentity, const FYcOnProfileLoadCompleted& Completion)
{
    FYcProfileSaveKey ProfileKey;
    FString Reason;
    if (!BuildProfileKey(ProfileIdentity, ProfileKey, Reason))
    {
        Completion.ExecuteIfBound(false, Reason);
        return;
    }

    LoadProfileAsyncByKey(ProfileKey, Completion);
}

void UYcProfileSaveSubsystem::SaveProfileAsync(const FYcProfileIdentity& ProfileIdentity, const FYcOnProfileSaveCompleted& Completion)
{
    FYcProfileSaveKey ProfileKey;
    FString Reason;
    if (!BuildProfileKey(ProfileIdentity, ProfileKey, Reason))
    {
        Completion.ExecuteIfBound(false, Reason);
        return;
    }

    SaveProfileAsyncByKey(ProfileKey, Completion);
}

void UYcProfileSaveSubsystem::SaveDirtyProfilesAsync(const FYcOnProfileSaveCompleted& Completion)
{
    SaveDirtyProfilesAsyncByKeyArray(DirtyProfiles.Array(), Completion);
}

void UYcProfileSaveSubsystem::LoadProfileRootAsync(const FYcProfileIdentity& ProfileIdentity, const FYcOnLoadProfileRoot& Completion)
{
    FYcProfileSaveKey ProfileKey;
    FString Reason;
    if (!BuildProfileKey(ProfileIdentity, ProfileKey, Reason))
    {
        Completion.ExecuteIfBound(EYcSaveBackendResult::Failed, FYcProfileSaveRoot(), Reason);
        return;
    }

    LoadProfileRootAsyncByKey(ProfileKey, Completion);
}

void UYcProfileSaveSubsystem::SaveProfileRootAsync(const FYcProfileIdentity& ProfileIdentity, const FYcProfileSaveRoot& Root, const FYcOnSaveProfileRoot& Completion)
{
    FYcProfileSaveKey ProfileKey;
    FString Reason;
    if (!BuildProfileKey(ProfileIdentity, ProfileKey, Reason))
    {
        Completion.ExecuteIfBound(false, Reason);
        return;
    }

    SaveProfileRootAsyncByKey(ProfileKey, Root, Completion);
}

bool UYcProfileSaveSubsystem::LoadProfileSync(const FYcProfileIdentity& ProfileIdentity, FString& OutReason)
{
    FYcProfileSaveKey ProfileKey;
    if (!BuildProfileKey(ProfileIdentity, ProfileKey, OutReason))
    {
        return false;
    }

    return LoadProfileSyncByKey(ProfileKey, OutReason);
}

bool UYcProfileSaveSubsystem::SaveProfileSync(const FYcProfileIdentity& ProfileIdentity, FString& OutReason)
{
    FYcProfileSaveKey ProfileKey;
    if (!BuildProfileKey(ProfileIdentity, ProfileKey, OutReason))
    {
        return false;
    }

    return SaveProfileSyncByKey(ProfileKey, OutReason);
}

EYcSaveBackendResult UYcProfileSaveSubsystem::LoadProfileRootSync(const FYcProfileIdentity& ProfileIdentity, FYcProfileSaveRoot& OutRoot, FString& OutReason)
{
    FYcProfileSaveKey ProfileKey;
    if (!BuildProfileKey(ProfileIdentity, ProfileKey, OutReason))
    {
        return EYcSaveBackendResult::Failed;
    }

    return LoadProfileRootSyncByKey(ProfileKey, OutRoot, OutReason);
}

bool UYcProfileSaveSubsystem::SaveProfileRootSync(const FYcProfileIdentity& ProfileIdentity, const FYcProfileSaveRoot& Root, FString& OutReason)
{
    FYcProfileSaveKey ProfileKey;
    if (!BuildProfileKey(ProfileIdentity, ProfileKey, OutReason))
    {
        return false;
    }

    return SaveProfileRootSyncByKey(ProfileKey, Root, OutReason);
}

void UYcProfileSaveSubsystem::MarkProfileDirty(const FYcProfileIdentity& ProfileIdentity)
{
    FYcProfileSaveKey ProfileKey;
    FString Reason;
    if (BuildProfileKey(ProfileIdentity, ProfileKey, Reason))
    {
        MarkProfileDirtyByKey(ProfileKey);
    }
}

void UYcProfileSaveSubsystem::ClearProfileDirty(const FYcProfileIdentity& ProfileIdentity)
{
    FYcProfileSaveKey ProfileKey;
    FString Reason;
    if (BuildProfileKey(ProfileIdentity, ProfileKey, Reason))
    {
        ClearProfileDirtyByKey(ProfileKey);
    }
}

bool UYcProfileSaveSubsystem::IsProfileDirty(const FYcProfileIdentity& ProfileIdentity) const
{
    FYcProfileSaveKey ProfileKey;
    FString Reason;
    return BuildProfileKey(ProfileIdentity, ProfileKey, Reason) && IsProfileDirtyByKey(ProfileKey);
}

void UYcProfileSaveSubsystem::RegisterProfileContextByKey(const FYcProfileSaveKey& ProfileKey, UObject* ContextObject)
{
    if (ProfileKey.IsValid() && IsValid(ContextObject))
    {
        ContextByProfile.Add(ProfileKey, ContextObject);
    }
}

void UYcProfileSaveSubsystem::LoadProfileAsyncByKey(const FYcProfileSaveKey& ProfileKey, const FYcOnProfileLoadCompleted& Completion)
{
    if (!ProfileKey.IsValid())
    {
        Completion.ExecuteIfBound(false, TEXT("Invalid profile key."));
        return;
    }

    UObject* ContextObject = ResolveContext(ProfileKey);
    if (!IsValid(ContextObject))
    {
        Completion.ExecuteIfBound(false, FString::Printf(TEXT("No context registered for profile '%s'."), *ProfileKey.ToDebugString()));
        return;
    }

    LoadProfileRootAsyncByKey(ProfileKey,
        FYcOnLoadProfileRoot::CreateWeakLambda(this, [this, ProfileKey, WeakContext = TWeakObjectPtr<UObject>(ContextObject), Completion](const EYcSaveBackendResult Result, const FYcProfileSaveRoot& Root, const FString& Reason)
        {
            if (Result == EYcSaveBackendResult::NotFound)
            {
                Completion.ExecuteIfBound(false, TEXT("Profile not found."));
                return;
            }

            if (Result != EYcSaveBackendResult::Success)
            {
                Completion.ExecuteIfBound(false, Reason.IsEmpty() ? TEXT("Backend load failed.") : Reason);
                return;
            }

            UObject* ResolvedContext = WeakContext.Get();
            if (!IsValid(ResolvedContext))
            {
                Completion.ExecuteIfBound(false, FString::Printf(TEXT("No context registered for profile '%s'."), *ProfileKey.ToDebugString()));
                return;
            }

            FString ApplyReason;
            if (!ApplyRootToContext(ProfileKey, ResolvedContext, Root, ApplyReason))
            {
                Completion.ExecuteIfBound(false, ApplyReason);
                return;
            }

            ClearProfileDirtyByKey(ProfileKey);
            Completion.ExecuteIfBound(true, FString());
        }));
}

void UYcProfileSaveSubsystem::SaveProfileAsyncByKey(const FYcProfileSaveKey& ProfileKey, const FYcOnProfileSaveCompleted& Completion)
{
    if (!ProfileKey.IsValid())
    {
        Completion.ExecuteIfBound(false, TEXT("Invalid profile key."));
        return;
    }

    UObject* ContextObject = ResolveContext(ProfileKey);
    if (!IsValid(ContextObject))
    {
        Completion.ExecuteIfBound(false, FString::Printf(TEXT("No context registered for profile '%s'."), *ProfileKey.ToDebugString()));
        return;
    }

    FYcProfileSaveRoot Root;
    FString BuildReason;
    if (!BuildRootFromContext(ProfileKey, ContextObject, Root, BuildReason))
    {
        Completion.ExecuteIfBound(false, BuildReason);
        return;
    }

    SaveProfileRootAsyncByKey(ProfileKey, Root,
        FYcOnSaveProfileRoot::CreateWeakLambda(this, [this, ProfileKey, Completion](const bool bSuccess, const FString& Reason)
        {
            if (bSuccess)
            {
                ClearProfileDirtyByKey(ProfileKey);
            }
            Completion.ExecuteIfBound(bSuccess, Reason);
        }));
}

void UYcProfileSaveSubsystem::SaveDirtyProfilesAsyncByKeyArray(const TArray<FYcProfileSaveKey>& ProfileKeys, const FYcOnProfileSaveCompleted& Completion)
{
    struct FAsyncSaveDirtyProfilesState
    {
        TArray<FYcProfileSaveKey> Keys;
        int32 CurrentIndex = 0;
        bool bAllSucceeded = true;
        FString FirstReason;
    };

    TSharedRef<FAsyncSaveDirtyProfilesState> State = MakeShared<FAsyncSaveDirtyProfilesState>();
    State->Keys = ProfileKeys;

    TSharedRef<TFunction<void()>> Step = MakeShared<TFunction<void()>>();
    *Step = [this, State, Step, Completion]()
    {
        if (State->CurrentIndex >= State->Keys.Num())
        {
            Completion.ExecuteIfBound(State->bAllSucceeded, State->FirstReason);
            return;
        }

        const FYcProfileSaveKey ProfileKey = State->Keys[State->CurrentIndex++];
        SaveProfileAsyncByKey(ProfileKey,
            FYcOnProfileSaveCompleted::CreateWeakLambda(this, [this, State, Step, Completion, ProfileKey](const bool bSuccess, const FString& Reason)
            {
                if (!bSuccess)
                {
                    State->bAllSucceeded = false;
                    if (State->FirstReason.IsEmpty())
                    {
                        State->FirstReason = FString::Printf(TEXT("Profile '%s' save failed: %s"), *ProfileKey.ToDebugString(), *Reason);
                    }
                }

                (*Step)();
            }));
    };

    (*Step)();
}

void UYcProfileSaveSubsystem::LoadProfileRootAsyncByKey(const FYcProfileSaveKey& ProfileKey, const FYcOnLoadProfileRoot& Completion)
{
    if (!ProfileKey.IsValid())
    {
        Completion.ExecuteIfBound(EYcSaveBackendResult::Failed, FYcProfileSaveRoot(), TEXT("Invalid profile key."));
        return;
    }

    EnsureBackendProvider();
    if (!BackendProvider)
    {
        Completion.ExecuteIfBound(EYcSaveBackendResult::Failed, FYcProfileSaveRoot(), TEXT("Backend provider is null."));
        return;
    }

    BackendProvider->LoadProfileRootAsync(ProfileKey, Completion);
}

void UYcProfileSaveSubsystem::SaveProfileRootAsyncByKey(const FYcProfileSaveKey& ProfileKey, const FYcProfileSaveRoot& Root, const FYcOnSaveProfileRoot& Completion)
{
    if (!ProfileKey.IsValid())
    {
        Completion.ExecuteIfBound(false, TEXT("Invalid profile key."));
        return;
    }

    EnsureBackendProvider();
    if (!BackendProvider)
    {
        Completion.ExecuteIfBound(false, TEXT("Backend provider is null."));
        return;
    }

    BackendProvider->SaveProfileRootAsync(ProfileKey, Root, Completion);
}

void UYcProfileSaveSubsystem::UnregisterProfileContextByKey(const FYcProfileSaveKey& ProfileKey, UObject* ContextObject)
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

bool UYcProfileSaveSubsystem::LoadProfileSyncByKey(const FYcProfileSaveKey& ProfileKey, FString& OutReason)
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
    const EYcSaveBackendResult LoadResult = LoadProfileRootSyncByKey(ProfileKey, LoadedRoot, OutReason);
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

    if (!ApplyRootToContext(ProfileKey, ContextObject, LoadedRoot, OutReason))
    {
        return false;
    }

    ClearProfileDirtyByKey(ProfileKey);
    OutReason.Reset();
    return true;
}

bool UYcProfileSaveSubsystem::SaveProfileSyncByKey(const FYcProfileSaveKey& ProfileKey, FString& OutReason)
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
    if (!BuildRootFromContext(ProfileKey, ContextObject, Root, OutReason))
    {
        return false;
    }

    if (!SaveProfileRootSyncByKey(ProfileKey, Root, OutReason))
    {
        return false;
    }

    ClearProfileDirtyByKey(ProfileKey);
    OutReason.Reset();
    return true;
}

bool UYcProfileSaveSubsystem::SaveDirtyProfilesSync(FString& OutReason)
{
    bool bAllSucceeded = true;
    FString FirstReason;
    const TArray<FYcProfileSaveKey> DirtyKeys = DirtyProfiles.Array();
    for (const FYcProfileSaveKey& ProfileKey : DirtyKeys)
    {
        FString LocalReason;
        if (!SaveProfileSyncByKey(ProfileKey, LocalReason))
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

EYcSaveBackendResult UYcProfileSaveSubsystem::LoadProfileRootSyncByKey(const FYcProfileSaveKey& ProfileKey, FYcProfileSaveRoot& OutRoot, FString& OutReason)
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

    return BackendProvider->LoadProfileRootSync(ProfileKey, OutRoot, OutReason);
}

bool UYcProfileSaveSubsystem::SaveProfileRootSyncByKey(const FYcProfileSaveKey& ProfileKey, const FYcProfileSaveRoot& Root, FString& OutReason)
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

    return BackendProvider->SaveProfileRootSync(ProfileKey, Root, OutReason);
}

void UYcProfileSaveSubsystem::MarkProfileDirtyByKey(const FYcProfileSaveKey& ProfileKey)
{
    if (ProfileKey.IsValid())
    {
        DirtyProfiles.Add(ProfileKey);
    }
}

void UYcProfileSaveSubsystem::ClearProfileDirtyByKey(const FYcProfileSaveKey& ProfileKey)
{
    if (ProfileKey.IsValid())
    {
        DirtyProfiles.Remove(ProfileKey);
    }
}

bool UYcProfileSaveSubsystem::IsProfileDirtyByKey(const FYcProfileSaveKey& ProfileKey) const
{
    return DirtyProfiles.Contains(ProfileKey);
}

UObject* UYcProfileSaveSubsystem::ResolveContext(const FYcProfileSaveKey& ProfileKey) const
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
        BackendClass = UYcSaveBackend_LocalSaveGame::StaticClass();
    }

    BackendProvider = NewObject<UYcSaveBackendProvider>(this, BackendClass);
    if (!BackendProvider)
    {
        UE_LOG(LogYcSaveCore, Error, TEXT("Failed to create backend provider '%s'."), *GetNameSafe(BackendClass));
    }
}

bool UYcProfileSaveSubsystem::BuildRootFromContext(const FYcProfileSaveKey& ProfileKey, UObject* ContextObject, FYcProfileSaveRoot& OutRoot, FString& OutReason) const
{
    OutRoot = FYcProfileSaveRoot();
    OutRoot.Environment = ProfileKey.Environment;
    OutRoot.AccountId = ProfileKey.AccountId;
    OutRoot.ProfileId = ProfileKey.ProfileId;
    OutRoot.SnapshotVersion = CurrentSnapshotVersion;
    OutRoot.LastSavedUnixTime = GetNowUnixTime();

    TArray<const UYcSaveDomainProvider*> Providers;
    GatherDomainProviders(Providers);
    for (const UYcSaveDomainProvider* Provider : Providers)
    {
        if (!Provider || !Provider->CanHandleContext(ContextObject))
        {
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
        return A.DomainKey.LexicalLess(B.DomainKey);
    });

    OutReason.Reset();
    return true;
}

bool UYcProfileSaveSubsystem::ApplyRootToContext(const FYcProfileSaveKey& ProfileKey, UObject* ContextObject, const FYcProfileSaveRoot& Root, FString& OutReason) const
{
    if (Root.Environment != ProfileKey.Environment || Root.AccountId != ProfileKey.AccountId || Root.ProfileId != ProfileKey.ProfileId)
    {
        OutReason = FString::Printf(TEXT("Profile key mismatch. expected='%s', root='%d::%s::%s'"),
            *ProfileKey.ToDebugString(),
            static_cast<int32>(Root.Environment),
            *Root.AccountId,
            *Root.ProfileId);
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
                OutReason = MissingReason;
                return false;
            }

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
