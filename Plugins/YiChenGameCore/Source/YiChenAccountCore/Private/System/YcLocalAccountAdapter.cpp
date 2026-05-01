// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcLocalAccountAdapter.h"

#include "System/YcLocalAccountSessionSaveGame.h"

#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "HAL/PlatformProcess.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/Guid.h"
#include "Misc/Parse.h"
#include "UObject/Package.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcLocalAccountAdapter)

namespace
{
    constexpr int32 SessionSaveUserIndex = 0;

    FString ResolveRequestedProfileId(const FString& RequestedProfileId)
    {
        return RequestedProfileId.IsEmpty() ? TEXT("Main") : RequestedProfileId;
    }

    FString ResolveCommandLineValue(const TCHAR* Key)
    {
        FString Value;
        return FParse::Value(FCommandLine::Get(), Key, Value) ? Value : FString();
    }

    EYcAccountEnvironment ResolveEnvironment(const UWorld* World)
    {
        if (!World)
        {
            return EYcAccountEnvironment::Offline;
        }

        if (World->WorldType == EWorldType::PIE)
        {
            return EYcAccountEnvironment::PIE;
        }

        return World->GetNetMode() == NM_Standalone ? EYcAccountEnvironment::Offline : EYcAccountEnvironment::Development;
    }

    int32 ResolvePieInstanceSlot(const UWorld* World)
    {
        if (!World)
        {
            return 0;
        }

        if (UPackage* Package = World->GetPackage())
        {
            const int32 PackagePieId = Package->GetPIEInstanceID();
            if (PackagePieId != INDEX_NONE)
            {
                return PackagePieId;
            }
        }

        if (GEngine)
        {
            if (const FWorldContext* Context = GEngine->GetWorldContextFromWorld(World))
            {
                return Context->PIEInstance != INDEX_NONE ? Context->PIEInstance : 0;
            }
        }

        return 0;
    }

    int32 ResolveStableLocalPlayerSlot(const APlayerController* PlayerController)
    {
        if (!PlayerController)
        {
            return 0;
        }

        if (const ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
        {
            const int32 LocalPlayerIndex = LocalPlayer->GetLocalPlayerIndex();
            if (LocalPlayerIndex != INDEX_NONE)
            {
                return LocalPlayerIndex;
            }

            const int32 ControllerId = LocalPlayer->GetControllerId();
            if (ControllerId != INDEX_NONE)
            {
                return ControllerId;
            }
        }

        return 0;
    }

    FString ResolveStablePlatformUserId(const UWorld* World, const APlayerController* PlayerController, const FYcAuthRequest* AuthRequest)
    {
        if (AuthRequest && !AuthRequest->PlatformUserIdHint.IsEmpty())
        {
            return AuthRequest->PlatformUserIdHint;
        }

        const FString CommandLineUserId = ResolveCommandLineValue(TEXT("YCPlatformUserId="));
        if (!CommandLineUserId.IsEmpty())
        {
            return CommandLineUserId;
        }

        if (World && World->WorldType == EWorldType::PIE)
        {
            FString ProjectName = FApp::GetProjectName();
            if (ProjectName.IsEmpty())
            {
                ProjectName = TEXT("Project");
            }

            return FString::Printf(TEXT("PIE_%s_%d_P%d"), *ProjectName, ResolvePieInstanceSlot(World), ResolveStableLocalPlayerSlot(PlayerController));
        }

        if (PlayerController)
        {
            if (const APlayerState* PlayerState = PlayerController->PlayerState)
            {
                const int32 PlayerId = PlayerState->GetPlayerId();
                if (PlayerId != 0)
                {
                    return FString::Printf(TEXT("PlayerState_%d"), PlayerId);
                }
            }
        }

        return FString::Printf(TEXT("Offline_%d"), FPlatformProcess::GetCurrentProcessId());
    }

    FString ResolveDisplayName(const APlayerController* PlayerController, const FString& PlatformUserId, const FYcAuthRequest* AuthRequest)
    {
        if (AuthRequest && !AuthRequest->DisplayNameHint.IsEmpty())
        {
            return AuthRequest->DisplayNameHint;
        }

        if (const FString CommandLineDisplayName = ResolveCommandLineValue(TEXT("YCDisplayName=")); !CommandLineDisplayName.IsEmpty())
        {
            return CommandLineDisplayName;
        }

        if (PlayerController && PlayerController->PlayerState)
        {
            const FString PlayerName = PlayerController->PlayerState->GetPlayerName();
            if (!PlayerName.IsEmpty())
            {
                return PlayerName;
            }
        }

        return PlatformUserId;
    }
}

FString UYcLocalAccountAdapter::BuildSessionSlotName(const UWorld* World, const FString& PlatformUserId)
{
    return FString::Printf(TEXT("YcAccountSession_%d_%s"), static_cast<int32>(ResolveEnvironment(World)), *PlatformUserId);
}

bool UYcLocalAccountAdapter::PersistSessionSnapshot(const UWorld* World, const FYcSessionSnapshot& SessionSnapshot)
{
    if (!SessionSnapshot.PlayerIdentity.IsAuthenticated() || !SessionSnapshot.PlayerIdentity.PlatformIdentity.IsValid())
    {
        return false;
    }

    const FString SlotName = BuildSessionSlotName(World, SessionSnapshot.PlayerIdentity.PlatformIdentity.PlatformUserId);
    UYcLocalAccountSessionSaveGame* SaveGameObject = Cast<UYcLocalAccountSessionSaveGame>(UGameplayStatics::CreateSaveGameObject(UYcLocalAccountSessionSaveGame::StaticClass()));
    if (!SaveGameObject)
    {
        return false;
    }

    SaveGameObject->SessionSnapshot = SessionSnapshot;
    SaveGameObject->PlatformUserId = SessionSnapshot.PlayerIdentity.PlatformIdentity.PlatformUserId;
    return UGameplayStatics::SaveGameToSlot(SaveGameObject, SlotName, SessionSaveUserIndex);
}

bool UYcLocalAccountAdapter::LoadPersistedSessionSnapshot(const UWorld* World, const FString& PlatformUserId, FYcSessionSnapshot& OutSessionSnapshot)
{
    OutSessionSnapshot = FYcSessionSnapshot();

    if (PlatformUserId.IsEmpty())
    {
        return false;
    }

    const FString SlotName = BuildSessionSlotName(World, PlatformUserId);
    if (!UGameplayStatics::DoesSaveGameExist(SlotName, SessionSaveUserIndex))
    {
        return false;
    }

    UYcLocalAccountSessionSaveGame* SaveGameObject = Cast<UYcLocalAccountSessionSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, SessionSaveUserIndex));
    if (!SaveGameObject)
    {
        return false;
    }

    if (SaveGameObject->PlatformUserId != PlatformUserId)
    {
        return false;
    }

    OutSessionSnapshot = SaveGameObject->SessionSnapshot;
    if (!OutSessionSnapshot.PlayerIdentity.IsAuthenticated())
    {
        OutSessionSnapshot = FYcSessionSnapshot();
        return false;
    }

    const EYcAccountEnvironment ResolvedEnvironment = ResolveEnvironment(World);
    OutSessionSnapshot.PlayerIdentity.AccountIdentity.Environment = ResolvedEnvironment;
    OutSessionSnapshot.PlayerIdentity.ActiveProfileIdentity.AccountIdentity.Environment = ResolvedEnvironment;
    for (FYcProfileIdentity& AvailableProfile : OutSessionSnapshot.AvailableProfiles)
    {
        AvailableProfile.AccountIdentity.Environment = ResolvedEnvironment;
    }

    if (OutSessionSnapshot.PlayerIdentity.HasActiveProfile())
    {
        OutSessionSnapshot.State = EYcAccountSessionState::Ready;
    }
    else
    {
        OutSessionSnapshot.State = EYcAccountSessionState::AuthenticatedNoProfile;
    }
    OutSessionSnapshot.ErrorMessage.Reset();
    OutSessionSnapshot.PlayerIdentity.bIsAuthoritative = true;
    OutSessionSnapshot.PlayerIdentity.PlatformIdentity.AuthTicket = FGuid::NewGuid().ToString(EGuidFormats::Digits);
    return true;
}

void UYcLocalAccountAdapter::DeletePersistedSessionSnapshot(const UWorld* World, const FString& PlatformUserId)
{
    if (PlatformUserId.IsEmpty())
    {
        return;
    }

    UGameplayStatics::DeleteGameInSlot(BuildSessionSlotName(World, PlatformUserId), SessionSaveUserIndex);
}

FYcSessionSnapshot UYcLocalAccountAdapter::BuildSessionSnapshot(const UWorld* World, const FString& PlatformUserId, const FString& DisplayName, const FString& RequestedProfileId)
{
    FYcSessionSnapshot SessionSnapshot;
    SessionSnapshot.State = EYcAccountSessionState::Ready;

    SessionSnapshot.PlayerIdentity.PlatformIdentity.Source = FCommandLine::Get()[0] != 0
        ? EYcPlatformAccountSource::CommandLine
        : EYcPlatformAccountSource::LocalOffline;
    SessionSnapshot.PlayerIdentity.PlatformIdentity.PlatformUserId = PlatformUserId;
    SessionSnapshot.PlayerIdentity.PlatformIdentity.DisplayName = DisplayName;
    SessionSnapshot.PlayerIdentity.PlatformIdentity.AuthTicket = FGuid::NewGuid().ToString(EGuidFormats::Digits);

    SessionSnapshot.PlayerIdentity.AccountIdentity.Environment = ResolveEnvironment(World);
    SessionSnapshot.PlayerIdentity.AccountIdentity.AccountId = ResolveCommandLineValue(TEXT("YCAccountId="));
    if (SessionSnapshot.PlayerIdentity.AccountIdentity.AccountId.IsEmpty())
    {
        SessionSnapshot.PlayerIdentity.AccountIdentity.AccountId = FString::Printf(TEXT("OFFLINE_%s"), *PlatformUserId);
    }

    SessionSnapshot.PlayerIdentity.ActiveProfileIdentity.AccountIdentity = SessionSnapshot.PlayerIdentity.AccountIdentity;
    SessionSnapshot.PlayerIdentity.ActiveProfileIdentity.ProfileId = ResolveRequestedProfileId(RequestedProfileId);
    SessionSnapshot.PlayerIdentity.ActiveProfileIdentity.DisplayName = SessionSnapshot.PlayerIdentity.ActiveProfileIdentity.ProfileId;
    SessionSnapshot.PlayerIdentity.bIsAuthoritative = true;

    SessionSnapshot.AvailableProfiles.Add(SessionSnapshot.PlayerIdentity.ActiveProfileIdentity);
    if (!SessionSnapshot.PlayerIdentity.ActiveProfileIdentity.ProfileId.Equals(TEXT("Main"), ESearchCase::CaseSensitive))
    {
        FYcProfileIdentity MainProfile = SessionSnapshot.PlayerIdentity.ActiveProfileIdentity;
        MainProfile.ProfileId = TEXT("Main");
        MainProfile.DisplayName = TEXT("Main");
        SessionSnapshot.AvailableProfiles.Insert(MainProfile, 0);
    }

    return SessionSnapshot;
}

void UYcLocalAccountAdapter::RestoreLocalSession(const UObject* WorldContextObject, const FYcOnAccountAuthenticationCompleted& Completion)
{
    const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
    const FString PlatformUserId = ResolveStablePlatformUserId(World, nullptr, nullptr);

    FYcSessionSnapshot RestoredSession;
    if (!LoadPersistedSessionSnapshot(World, PlatformUserId, RestoredSession))
    {
        Completion.ExecuteIfBound(false, FYcSessionSnapshot(), TEXT("RestoreLocalSession failed: no cached local session."));
        return;
    }

    Completion.ExecuteIfBound(true, RestoredSession, FString());
}

void UYcLocalAccountAdapter::AuthenticateLocalPlayer(const UObject* WorldContextObject, const FYcAuthRequest& Request, const FYcOnAccountAuthenticationCompleted& Completion)
{
    const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
    const FString PlatformUserId = ResolveStablePlatformUserId(World, nullptr, &Request);
    const FString DisplayName = ResolveDisplayName(nullptr, PlatformUserId, &Request);
    const FYcSessionSnapshot SessionSnapshot = BuildSessionSnapshot(World, PlatformUserId, DisplayName, Request.RequestedProfileId);
    PersistSessionSnapshot(World, SessionSnapshot);
    Completion.ExecuteIfBound(true, SessionSnapshot, FString());
}

void UYcLocalAccountAdapter::AuthenticatePlayerControllerOnServer(const APlayerController* PlayerController, const FYcAuthRequest& Request, const FYcOnAccountAuthenticationCompleted& Completion)
{
    const UWorld* World = PlayerController ? PlayerController->GetWorld() : nullptr;
    const FString PlatformUserId = ResolveStablePlatformUserId(World, PlayerController, &Request);
    const FString DisplayName = ResolveDisplayName(PlayerController, PlatformUserId, &Request);
    const FYcSessionSnapshot SessionSnapshot = BuildSessionSnapshot(World, PlatformUserId, DisplayName, Request.RequestedProfileId);
    PersistSessionSnapshot(World, SessionSnapshot);
    Completion.ExecuteIfBound(true, SessionSnapshot, FString());
}

void UYcLocalAccountAdapter::QueryAvailableProfiles(const UObject* WorldContextObject, const FYcSessionSnapshot& CurrentSession, const FYcOnAccountProfileQueryCompleted& Completion)
{
    const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
    TArray<FYcProfileIdentity> Profiles = CurrentSession.AvailableProfiles;

    if (Profiles.IsEmpty() && CurrentSession.PlayerIdentity.IsAuthenticated())
    {
        FYcSessionSnapshot RestoredSession;
        const FString PlatformUserId = CurrentSession.PlayerIdentity.PlatformIdentity.PlatformUserId;
        if (LoadPersistedSessionSnapshot(World, PlatformUserId, RestoredSession))
        {
            Profiles = RestoredSession.AvailableProfiles;
        }
    }

    if (Profiles.IsEmpty() && CurrentSession.PlayerIdentity.HasActiveProfile())
    {
        Profiles.Add(CurrentSession.PlayerIdentity.ActiveProfileIdentity);
    }

    Completion.ExecuteIfBound(CurrentSession.PlayerIdentity.IsAuthenticated(), Profiles, CurrentSession.PlayerIdentity.IsAuthenticated() ? FString() : TEXT("QueryAvailableProfiles failed: current session is not authenticated."));
}

void UYcLocalAccountAdapter::CreateProfile(const UObject* WorldContextObject, const FYcSessionSnapshot& CurrentSession, const FString& RequestedProfileId, const FString& DisplayName, const bool bActivateNewProfile, const FYcOnAccountProfileActivationCompleted& Completion)
{
    if (!CurrentSession.PlayerIdentity.AccountIdentity.IsValid())
    {
        Completion.ExecuteIfBound(false, FYcSessionSnapshot(), TEXT("CreateProfile failed: current session is not authenticated."));
        return;
    }

    const FString EffectiveProfileId = ResolveRequestedProfileId(RequestedProfileId);
    if (CurrentSession.AvailableProfiles.ContainsByPredicate([&EffectiveProfileId](const FYcProfileIdentity& Profile)
    {
        return Profile.ProfileId.Equals(EffectiveProfileId, ESearchCase::CaseSensitive);
    }))
    {
        Completion.ExecuteIfBound(false, FYcSessionSnapshot(), FString::Printf(TEXT("CreateProfile failed: profile '%s' already exists."), *EffectiveProfileId));
        return;
    }

    FYcSessionSnapshot NextSession = CurrentSession;
    NextSession.State = EYcAccountSessionState::Ready;
    NextSession.ErrorMessage.Reset();

    FYcProfileIdentity NewProfile;
    NewProfile.AccountIdentity = CurrentSession.PlayerIdentity.AccountIdentity;
    NewProfile.ProfileId = EffectiveProfileId;
    NewProfile.DisplayName = DisplayName.IsEmpty() ? EffectiveProfileId : DisplayName;
    NextSession.AvailableProfiles.Add(NewProfile);

    if (bActivateNewProfile || !NextSession.PlayerIdentity.ActiveProfileIdentity.IsValid())
    {
        NextSession.PlayerIdentity.ActiveProfileIdentity = NewProfile;
    }

    PersistSessionSnapshot(WorldContextObject ? WorldContextObject->GetWorld() : nullptr, NextSession);
    Completion.ExecuteIfBound(true, NextSession, FString());
}

void UYcLocalAccountAdapter::ActivateProfile(const UObject* WorldContextObject, const FYcSessionSnapshot& CurrentSession, const FString& RequestedProfileId, const bool bCreateProfileIfMissing, const FYcOnAccountProfileActivationCompleted& Completion)
{
    if (!CurrentSession.PlayerIdentity.AccountIdentity.IsValid())
    {
        Completion.ExecuteIfBound(false, FYcSessionSnapshot(), TEXT("ActivateProfile failed: current session is not authenticated."));
        return;
    }

    const FString EffectiveProfileId = ResolveRequestedProfileId(RequestedProfileId);
    const FYcProfileIdentity* ExistingProfile = CurrentSession.AvailableProfiles.FindByPredicate([&EffectiveProfileId](const FYcProfileIdentity& Profile)
    {
        return Profile.ProfileId.Equals(EffectiveProfileId, ESearchCase::CaseSensitive);
    });

    if (!ExistingProfile && !bCreateProfileIfMissing)
    {
        Completion.ExecuteIfBound(false, FYcSessionSnapshot(), FString::Printf(TEXT("ActivateProfile failed: profile '%s' is not available."), *EffectiveProfileId));
        return;
    }

    FYcSessionSnapshot NextSession = CurrentSession;
    NextSession.State = EYcAccountSessionState::Ready;
    NextSession.ErrorMessage.Reset();
    if (ExistingProfile)
    {
        NextSession.PlayerIdentity.ActiveProfileIdentity = *ExistingProfile;
    }
    else
    {
        FYcProfileIdentity NewProfile;
        NewProfile.AccountIdentity = CurrentSession.PlayerIdentity.AccountIdentity;
        NewProfile.ProfileId = EffectiveProfileId;
        NewProfile.DisplayName = EffectiveProfileId;
        NextSession.PlayerIdentity.ActiveProfileIdentity = NewProfile;
        NextSession.AvailableProfiles.Add(NewProfile);
    }

    PersistSessionSnapshot(WorldContextObject ? WorldContextObject->GetWorld() : nullptr, NextSession);
    Completion.ExecuteIfBound(true, NextSession, FString());
}

void UYcLocalAccountAdapter::Logout(const UObject* WorldContextObject, const FYcSessionSnapshot& CurrentSession, const FYcOnAccountLogoutCompleted& Completion)
{
    const UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
    DeletePersistedSessionSnapshot(World, CurrentSession.PlayerIdentity.PlatformIdentity.PlatformUserId);
    Completion.ExecuteIfBound(true, FString());
}
