// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcAccountSessionSubsystem.h"

#include "System/YcAccountAdapter.h"
#include "System/YcLocalAccountAdapter.h"
#include "YiChenAccountCore.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAccountSessionSubsystem)

UYcAccountSessionSubsystem* UYcAccountSessionSubsystem::Get(const UObject* WorldContextObject)
{
    if (WorldContextObject)
    {
        if (const UWorld* World = WorldContextObject->GetWorld())
        {
            if (UGameInstance* GameInstance = World->GetGameInstance())
            {
                return GameInstance->GetSubsystem<UYcAccountSessionSubsystem>();
            }
        }
        return nullptr;
    }

    if (!GEngine)
    {
        return nullptr;
    }

    for (const FWorldContext& Context : GEngine->GetWorldContexts())
    {
        if (!Context.World())
        {
            continue;
        }

        if (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game)
        {
            if (UGameInstance* GameInstance = Context.World()->GetGameInstance())
            {
                return GameInstance->GetSubsystem<UYcAccountSessionSubsystem>();
            }
        }
    }
    return nullptr;
}

void UYcAccountSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CurrentSession.State = EYcAccountSessionState::SignedOut;
    EnsureAdapter();
}

void UYcAccountSessionSubsystem::Deinitialize()
{
    Adapter = nullptr;
    CurrentSession = FYcSessionSnapshot();
    Super::Deinitialize();
}

bool UYcAccountSessionSubsystem::RestoreLocalSession(APlayerController* PlayerController)
{
    if (!IsValid(PlayerController))
    {
        return false;
    }

    EnsureAdapter();
    if (!Adapter)
    {
        InvalidateCurrentSession(TEXT("RestoreLocalSession failed: account adapter is null."));
        return false;
    }

    SetSessionState(EYcAccountSessionState::Refreshing);
    if (!PlayerController->HasAuthority())
    {
        return false;
    }

    FYcSessionSnapshot RestoredSession;
    if (!RestoreLocalSessionOnServer(PlayerController, RestoredSession))
    {
        SetSessionState(EYcAccountSessionState::SignedOut);
        return false;
    }

    ApplyResolvedSession(RestoredSession, true);
    return true;
}

bool UYcAccountSessionSubsystem::BeginLocalLogin(APlayerController* PlayerController, const FYcAuthRequest& Request)
{
    if (!IsValid(PlayerController))
    {
        return false;
    }

    if (CurrentSession.State == EYcAccountSessionState::Authenticating
        || CurrentSession.State == EYcAccountSessionState::Refreshing
        || CurrentSession.State == EYcAccountSessionState::ProfileSelecting)
    {
        return true;
    }

    EnsureAdapter();
    if (!Adapter)
    {
        SetSessionState(EYcAccountSessionState::Error, TEXT("BeginLocalLogin failed: account adapter is null."));
        return false;
    }

    SetSessionState(EYcAccountSessionState::Authenticating);
    if (!PlayerController->HasAuthority())
    {
        return false;
    }

    FYcSessionSnapshot ResolvedSession;
    if (!AuthenticatePlayerControllerOnServer(PlayerController, Request, ResolvedSession))
    {
        return false;
    }

    ApplyResolvedSession(ResolvedSession, true);
    return true;
}

bool UYcAccountSessionSubsystem::RefreshAvailableProfiles(APlayerController* PlayerController)
{
    if (!IsValid(PlayerController))
    {
        return false;
    }

    EnsureAdapter();
    if (!Adapter)
    {
        InvalidateCurrentSession(TEXT("RefreshAvailableProfiles failed: account adapter is null."));
        return false;
    }

    if (!CurrentSession.PlayerIdentity.IsAuthenticated())
    {
        return false;
    }

    SetSessionState(CurrentSession.PlayerIdentity.HasActiveProfile()
        ? EYcAccountSessionState::Ready
        : EYcAccountSessionState::AuthenticatedNoProfile);

    if (!PlayerController->HasAuthority())
    {
        return false;
    }

    TArray<FYcProfileIdentity> Profiles;
    if (!RefreshAvailableProfilesOnServer(PlayerController, Profiles))
    {
        return false;
    }

    CurrentSession.AvailableProfiles = MoveTemp(Profiles);
    BroadcastSessionChanged();
    return true;
}

bool UYcAccountSessionSubsystem::CreateProfile(APlayerController* PlayerController, const FString& RequestedProfileId, const FString& DisplayName, const bool bActivateNewProfile)
{
    if (!IsValid(PlayerController) || RequestedProfileId.IsEmpty())
    {
        return false;
    }

    if (!CurrentSession.PlayerIdentity.IsAuthenticated())
    {
        return false;
    }

    EnsureAdapter();
    if (!Adapter)
    {
        InvalidateCurrentSession(TEXT("CreateProfile failed: account adapter is null."));
        return false;
    }

    SetSessionState(EYcAccountSessionState::ProfileSelecting);
    if (!PlayerController->HasAuthority())
    {
        return false;
    }

    FYcSessionSnapshot ResolvedSession;
    if (!CreateProfileOnServer(PlayerController, RequestedProfileId, DisplayName, bActivateNewProfile, ResolvedSession))
    {
        return false;
    }

    ApplyResolvedSession(ResolvedSession, true);
    return true;
}

bool UYcAccountSessionSubsystem::SwitchActiveProfile(APlayerController* PlayerController, const FString& RequestedProfileId, const bool bCreateProfileIfMissing)
{
    if (!IsValid(PlayerController) || RequestedProfileId.IsEmpty())
    {
        return false;
    }

    if (CurrentSession.State != EYcAccountSessionState::Ready && CurrentSession.State != EYcAccountSessionState::AuthenticatedNoProfile)
    {
        return false;
    }

    EnsureAdapter();
    if (!Adapter)
    {
        SetSessionState(EYcAccountSessionState::Error, TEXT("SwitchActiveProfile failed: account adapter is null."));
        return false;
    }

    SetSessionState(EYcAccountSessionState::ProfileSelecting);
    if (!PlayerController->HasAuthority())
    {
        return false;
    }

    FYcSessionSnapshot ResolvedSession;
    if (!SwitchActiveProfileOnServer(PlayerController, RequestedProfileId, bCreateProfileIfMissing, ResolvedSession))
    {
        return false;
    }

    ApplyResolvedSession(ResolvedSession, true);
    return true;
}

bool UYcAccountSessionSubsystem::SignOut(APlayerController* PlayerController)
{
    if (!IsValid(PlayerController))
    {
        return false;
    }

    EnsureAdapter();
    if (!Adapter)
    {
        SetSessionState(EYcAccountSessionState::Error, TEXT("SignOut failed: account adapter is null."));
        return false;
    }

    SetSessionState(EYcAccountSessionState::Refreshing);
    if (!PlayerController->HasAuthority())
    {
        return false;
    }

    return SignOutPlayerOnServer(PlayerController);
}

bool UYcAccountSessionSubsystem::AuthenticatePlayerControllerOnServer(APlayerController* PlayerController, const FYcAuthRequest& Request, FYcSessionSnapshot& OutSessionSnapshot)
{
    EnsureAdapter();
    if (!Adapter || !IsValid(PlayerController))
    {
        SetSessionState(EYcAccountSessionState::Error, TEXT("AuthenticatePlayerControllerOnServer failed: invalid state."));
        return false;
    }

    bool bCompleted = false;
    OutSessionSnapshot = FYcSessionSnapshot();
    Adapter->AuthenticatePlayerControllerOnServer(PlayerController, Request,
        FYcOnAccountAuthenticationCompleted::CreateLambda([this, &OutSessionSnapshot, &bCompleted](const bool bSuccess, const FYcSessionSnapshot& SessionSnapshot, const FString& ErrorMessage)
        {
            bCompleted = bSuccess;
            if (!bSuccess)
            {
                InvalidateCurrentSession(ErrorMessage);
                return;
            }

            OutSessionSnapshot = SessionSnapshot;
        }));
    return bCompleted;
}

bool UYcAccountSessionSubsystem::RestoreLocalSessionOnServer(APlayerController* PlayerController, FYcSessionSnapshot& OutSessionSnapshot)
{
    EnsureAdapter();
    if (!Adapter || !IsValid(PlayerController))
    {
        InvalidateCurrentSession(TEXT("RestoreLocalSessionOnServer failed: invalid state."));
        return false;
    }

    bool bCompleted = false;
    OutSessionSnapshot = FYcSessionSnapshot();
    Adapter->RestoreLocalSession(PlayerController,
        FYcOnAccountAuthenticationCompleted::CreateLambda([this, &OutSessionSnapshot, &bCompleted](const bool bSuccess, const FYcSessionSnapshot& SessionSnapshot, const FString& ErrorMessage)
        {
            bCompleted = bSuccess;
            if (!bSuccess)
            {
                return;
            }

            OutSessionSnapshot = SessionSnapshot;
        }));
    return bCompleted;
}

bool UYcAccountSessionSubsystem::RefreshAvailableProfilesOnServer(APlayerController* PlayerController, TArray<FYcProfileIdentity>& OutProfiles)
{
    EnsureAdapter();
    if (!Adapter || !IsValid(PlayerController))
    {
        InvalidateCurrentSession(TEXT("RefreshAvailableProfilesOnServer failed: invalid state."));
        return false;
    }

    bool bCompleted = false;
    OutProfiles.Reset();
    Adapter->QueryAvailableProfiles(PlayerController, CurrentSession,
        FYcOnAccountProfileQueryCompleted::CreateLambda([this, &OutProfiles, &bCompleted](const bool bSuccess, const TArray<FYcProfileIdentity>& Profiles, const FString& ErrorMessage)
        {
            bCompleted = bSuccess;
            if (!bSuccess)
            {
                InvalidateCurrentSession(ErrorMessage);
                return;
            }

            OutProfiles = Profiles;
        }));
    return bCompleted;
}

bool UYcAccountSessionSubsystem::CreateProfileOnServer(APlayerController* PlayerController, const FString& RequestedProfileId, const FString& DisplayName, const bool bActivateNewProfile, FYcSessionSnapshot& OutSessionSnapshot)
{
    EnsureAdapter();
    if (!Adapter || !IsValid(PlayerController))
    {
        InvalidateCurrentSession(TEXT("CreateProfileOnServer failed: invalid state."));
        return false;
    }

    bool bCompleted = false;
    OutSessionSnapshot = FYcSessionSnapshot();
    Adapter->CreateProfile(PlayerController, CurrentSession, RequestedProfileId, DisplayName, bActivateNewProfile,
        FYcOnAccountProfileActivationCompleted::CreateLambda([this, &OutSessionSnapshot, &bCompleted](const bool bSuccess, const FYcSessionSnapshot& SessionSnapshot, const FString& ErrorMessage)
        {
            bCompleted = bSuccess;
            if (!bSuccess)
            {
                InvalidateCurrentSession(ErrorMessage);
                return;
            }

            OutSessionSnapshot = SessionSnapshot;
        }));
    return bCompleted;
}

bool UYcAccountSessionSubsystem::SwitchActiveProfileOnServer(APlayerController* PlayerController, const FString& RequestedProfileId, const bool bCreateProfileIfMissing, FYcSessionSnapshot& OutSessionSnapshot)
{
    EnsureAdapter();
    if (!Adapter || !IsValid(PlayerController))
    {
        SetSessionState(EYcAccountSessionState::Error, TEXT("SwitchActiveProfileOnServer failed: invalid state."));
        return false;
    }

    bool bCompleted = false;
    OutSessionSnapshot = FYcSessionSnapshot();
    Adapter->ActivateProfile(PlayerController, CurrentSession, RequestedProfileId, bCreateProfileIfMissing,
        FYcOnAccountProfileActivationCompleted::CreateLambda([this, &OutSessionSnapshot, &bCompleted](const bool bSuccess, const FYcSessionSnapshot& SessionSnapshot, const FString& ErrorMessage)
        {
            bCompleted = bSuccess;
            if (!bSuccess)
            {
                InvalidateCurrentSession(ErrorMessage);
                return;
            }

            OutSessionSnapshot = SessionSnapshot;
        }));
    return bCompleted;
}

bool UYcAccountSessionSubsystem::SignOutPlayerOnServer(APlayerController* PlayerController)
{
    EnsureAdapter();
    if (!Adapter || !IsValid(PlayerController))
    {
        SetSessionState(EYcAccountSessionState::Error, TEXT("SignOutPlayerOnServer failed: invalid state."));
        return false;
    }

    bool bCompleted = false;
    Adapter->Logout(PlayerController, CurrentSession,
        FYcOnAccountLogoutCompleted::CreateLambda([this, &bCompleted](const bool bSuccess, const FString& ErrorMessage)
        {
            bCompleted = bSuccess;
            if (!bSuccess)
            {
                InvalidateCurrentSession(ErrorMessage);
                return;
            }

            CurrentSession = FYcSessionSnapshot();
            CurrentSession.State = EYcAccountSessionState::SignedOut;
            BroadcastSessionChanged();
        }));
    return bCompleted;
}

void UYcAccountSessionSubsystem::AdoptReplicatedPlayerIdentity(const FYcPlayerIdentitySnapshot& PlayerIdentitySnapshot)
{
    if (!PlayerIdentitySnapshot.IsReady())
    {
        return;
    }

    CurrentSession.PlayerIdentity = PlayerIdentitySnapshot;
    CurrentSession.State = EYcAccountSessionState::Ready;
    if (!CurrentSession.AvailableProfiles.ContainsByPredicate([&PlayerIdentitySnapshot](const FYcProfileIdentity& Profile)
    {
        return Profile == PlayerIdentitySnapshot.ActiveProfileIdentity;
    }))
    {
        CurrentSession.AvailableProfiles.Add(PlayerIdentitySnapshot.ActiveProfileIdentity);
    }
    BroadcastSessionChanged();
}

void UYcAccountSessionSubsystem::InvalidateCurrentSession(const FString& ErrorMessage, const EYcAccountSessionState ErrorState)
{
    CurrentSession = FYcSessionSnapshot();
    CurrentSession.State = ErrorState;
    CurrentSession.ErrorMessage = ErrorMessage;
    BroadcastSessionChanged();
}

void UYcAccountSessionSubsystem::EnsureAdapter()
{
    if (Adapter)
    {
        return;
    }

    UClass* AdapterUClass = nullptr;
    if (AdapterClass.IsValid())
    {
        AdapterUClass = AdapterClass.Get();
    }
    else if (!AdapterClass.IsNull())
    {
        AdapterUClass = AdapterClass.LoadSynchronous();
    }

    if (!AdapterUClass)
    {
        AdapterUClass = UYcLocalAccountAdapter::StaticClass();
    }

    Adapter = NewObject<UYcAccountAdapter>(this, AdapterUClass);
    if (!Adapter)
    {
        UE_LOG(LogYcAccountCore, Error, TEXT("EnsureAdapter failed: create adapter failed. class='%s'"), *GetNameSafe(AdapterUClass));
    }
}

void UYcAccountSessionSubsystem::BroadcastSessionChanged()
{
    OnAccountSessionChanged.Broadcast(CurrentSession);
    OnPlayerIdentityChanged.Broadcast(CurrentSession.PlayerIdentity);
}

void UYcAccountSessionSubsystem::SetSessionState(const EYcAccountSessionState NewState, const FString& ErrorMessage)
{
    CurrentSession.State = NewState;
    CurrentSession.ErrorMessage = ErrorMessage;
    BroadcastSessionChanged();
}

void UYcAccountSessionSubsystem::ApplyResolvedSession(const FYcSessionSnapshot& SessionSnapshot, const bool bBroadcastIdentity)
{
    CurrentSession = SessionSnapshot;
    if (CurrentSession.PlayerIdentity.IsReady())
    {
        CurrentSession.State = EYcAccountSessionState::Ready;
    }
    else if (CurrentSession.PlayerIdentity.IsAuthenticated())
    {
        CurrentSession.State = EYcAccountSessionState::AuthenticatedNoProfile;
    }

    if (bBroadcastIdentity)
    {
        BroadcastSessionChanged();
    }
}
