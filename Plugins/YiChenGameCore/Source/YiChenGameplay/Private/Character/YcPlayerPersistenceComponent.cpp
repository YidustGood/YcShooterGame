// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Character/YcPlayerPersistenceComponent.h"

#include "Character/YcPersistenceMessages.h"
#include "GameModes/AsyncAction_ExperienceReady.h"
#include "Player/YcPlayerState.h"
#include "System/YcAccountIdentityLibrary.h"
#include "System/YcAccountSessionSubsystem.h"
#include "System/YcMetaInventorySubsystem.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "YcEquipmentSlotComponent.h"
#include "YcInventoryManagerComponent.h"
#include "YcQuickBarComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcPlayerPersistenceComponent)

DEFINE_LOG_CATEGORY_STATIC(LogYcPlayerPersistence, Log, All);

namespace
{
	static bool IsPlayerSnapshotEmpty(const FYcMetaPlayerSnapshot& Snapshot)
	{
		return Snapshot.InventoryItems.IsEmpty()
			&& Snapshot.EquipmentSlots.IsEmpty()
			&& Snapshot.QuickBarSlots.IsEmpty();
	}
}

UYcPlayerPersistenceComponent::UYcPlayerPersistenceComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetIsReplicatedByDefault(true);
}

void UYcPlayerPersistenceComponent::BeginPlay()
{
	Super::BeginPlay();

	ExperienceReadyTask = UAsyncAction_ExperienceReady::WaitForExperienceReady(this);
	if (IsValid(ExperienceReadyTask))
	{
		ExperienceReadyTask->OnReady.AddUniqueDynamic(this, &ThisClass::HandleExperienceReady);
		ExperienceReadyTask->Activate();
	}

	if (UYcAccountSessionSubsystem* SessionSubsystem = GetAccountSessionSubsystem())
	{
		BoundAccountSessionSubsystem = SessionSubsystem;
		SessionSubsystem->OnPlayerIdentityChanged.AddUniqueDynamic(this, &ThisClass::HandleLocalPlayerIdentityChanged);
	}

	QueueEvaluation();
}

void UYcPlayerPersistenceComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (IsValid(BoundAccountSessionSubsystem))
	{
		BoundAccountSessionSubsystem->OnPlayerIdentityChanged.RemoveDynamic(this, &ThisClass::HandleLocalPlayerIdentityChanged);
	}

	Super::EndPlay(EndPlayReason);
}

void UYcPlayerPersistenceComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	const FYcPlayerInventoryRuntime LiveRuntime = BuildRuntimeHandle();
	const bool bRuntimeChanged =
		LiveRuntime.PlayerState != CurrentRuntime.PlayerState
		|| LiveRuntime.ControlledPawn != CurrentRuntime.ControlledPawn
		|| LiveRuntime.PlayerInventory != CurrentRuntime.PlayerInventory
		|| LiveRuntime.StashInventory != CurrentRuntime.StashInventory
		|| LiveRuntime.EquipmentBridge != CurrentRuntime.EquipmentBridge
		|| LiveRuntime.QuickBarBridge != CurrentRuntime.QuickBarBridge
		|| LiveRuntime.SceneMode != CurrentRuntime.SceneMode;

	if (bRuntimeChanged)
	{
		CaptureCurrentRuntimeSnapshot();
		QueueEvaluation();
	}

	if (bEvaluationQueued)
	{
		EvaluateFlow(false);
	}
}

void UYcPlayerPersistenceComponent::InitializePersistenceFlow()
{
	QueueEvaluation();
}

void UYcPlayerPersistenceComponent::RefreshFromCurrentProfile()
{
	if (APlayerController* PlayerController = GetOwningPlayerController(); IsValid(PlayerController) && !PlayerController->HasAuthority())
	{
		ServerRefreshFromCurrentProfile();
		return;
	}

	EvaluateFlow(true);
}

void UYcPlayerPersistenceComponent::ReloadActiveProfile()
{
	RefreshFromCurrentProfile();
}

bool UYcPlayerPersistenceComponent::SaveCurrentProfile()
{
	if (APlayerController* PlayerController = GetOwningPlayerController(); IsValid(PlayerController) && !PlayerController->HasAuthority())
	{
		ServerSaveCurrentProfile();
		return true;
	}

	return ExecuteSaveCurrentProfile();
}

bool UYcPlayerPersistenceComponent::LoadInMatchLoadout()
{
	if (APlayerController* PlayerController = GetOwningPlayerController(); IsValid(PlayerController) && !PlayerController->HasAuthority())
	{
		ServerLoadInMatchLoadout();
		return true;
	}

	return ExecuteLoadInMatchLoadout();
}

bool UYcPlayerPersistenceComponent::CommitMatchResult(const bool bExtractionSucceeded)
{
	if (APlayerController* PlayerController = GetOwningPlayerController(); IsValid(PlayerController) && !PlayerController->HasAuthority())
	{
		ServerCommitMatchResult(bExtractionSucceeded);
		return true;
	}

	return ExecuteCommitMatchResult(bExtractionSucceeded);
}

bool UYcPlayerPersistenceComponent::CanSaveCurrentProfile() const
{
	return bHasLoadedProfile
		&& bIsPersistenceReady
		&& SceneMode == EYcPlayerPersistenceSceneMode::OutOfMatch
		&& CurrentProfileIdentity.IsValid();
}

void UYcPlayerPersistenceComponent::NotifyDefaultLoadoutApplied()
{
	ClearDefaultLoadoutRequest();
	CaptureCurrentRuntimeSnapshot();
}

void UYcPlayerPersistenceComponent::SetPersistenceEnabled(const bool bEnabled)
{
	if (bPersistenceEnabled == bEnabled)
	{
		return;
	}

	bPersistenceEnabled = bEnabled;
	QueueEvaluation();
}

void UYcPlayerPersistenceComponent::SetSceneMode(const EYcPlayerPersistenceSceneMode NewMode)
{
	if (SceneMode == NewMode)
	{
		return;
	}

	CaptureCurrentRuntimeSnapshot();
	SceneMode = NewMode;
	QueueEvaluation();
}

void UYcPlayerPersistenceComponent::SetOutOfMatchStashActor(AActor* InStashActor)
{
	if (OutOfMatchStashActor == InStashActor)
	{
		return;
	}

	OutOfMatchStashActor = InStashActor;
	QueueEvaluation();
}

void UYcPlayerPersistenceComponent::HandleExperienceReady()
{
	bExperienceReady = true;
	RequestDefaultLoadoutIfNeeded();
}

void UYcPlayerPersistenceComponent::HandleLocalPlayerIdentityChanged(FYcPlayerIdentitySnapshot PlayerIdentity)
{
	if (APlayerController* PlayerController = GetOwningPlayerController())
	{
		FYcPersistenceProfileMessage Message;
		Message.PlayerController = PlayerController;
		Message.LocalPlayer = PlayerController->GetLocalPlayer();
		Message.ReasonTag = YcPersistenceTags::Persistence_ProfileChanged;

		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
		MessageSubsystem.BroadcastMessage(YcPersistenceTags::Persistence_ProfileChanged, Message);
	}

	(void)PlayerIdentity;
	QueueEvaluation();
}

void UYcPlayerPersistenceComponent::ServerRefreshFromCurrentProfile_Implementation()
{
	EvaluateFlow(true);
}

void UYcPlayerPersistenceComponent::ServerSaveCurrentProfile_Implementation()
{
	ExecuteSaveCurrentProfile();
}

void UYcPlayerPersistenceComponent::ServerLoadInMatchLoadout_Implementation()
{
	ExecuteLoadInMatchLoadout();
}

void UYcPlayerPersistenceComponent::ServerCommitMatchResult_Implementation(bool bExtractionSucceeded)
{
	ExecuteCommitMatchResult(bExtractionSucceeded);
}

void UYcPlayerPersistenceComponent::QueueEvaluation()
{
	bEvaluationQueued = true;
}

void UYcPlayerPersistenceComponent::EvaluateFlow(const bool bForceReload)
{
	bEvaluationQueued = false;

	APlayerController* PlayerController = GetOwningPlayerController();
	if (!IsValid(PlayerController) || !PlayerController->HasAuthority() || !bPersistenceEnabled || SceneMode == EYcPlayerPersistenceSceneMode::Disabled)
	{
		RuntimeState = EYcPlayerPersistenceRuntimeState::Idle;
		UpdateReadyState(false);
		ClearDefaultLoadoutRequest();
		CurrentRuntime = FYcPlayerInventoryRuntime();
		return;
	}

	FYcProfileIdentity ResolvedProfileIdentity;
	if (!ResolveCurrentProfileIdentity(ResolvedProfileIdentity))
	{
		RuntimeState = EYcPlayerPersistenceRuntimeState::WaitingForIdentity;
		UpdateReadyState(false);
		CurrentRuntime = BuildRuntimeHandle();
		CurrentProfileIdentity = FYcProfileIdentity();
		return;
	}

	const FYcPlayerInventoryRuntime Runtime = BuildRuntimeHandle();
	const bool bRequiresOutOfMatchRuntime = SceneMode == EYcPlayerPersistenceSceneMode::OutOfMatch;
	const bool bRuntimeReady = bRequiresOutOfMatchRuntime ? Runtime.SupportsOutOfMatchPersistence() : Runtime.SupportsInMatchPersistence();
	if (!bRuntimeReady)
	{
		RuntimeState = EYcPlayerPersistenceRuntimeState::WaitingForRuntime;
		UpdateReadyState(false);
		CurrentRuntime = Runtime;
		CurrentProfileIdentity = ResolvedProfileIdentity;
		return;
	}

	CurrentProfileIdentity = ResolvedProfileIdentity;

	const bool bProfileChanged = !bHasLoadedProfile || !(LoadedProfileIdentity == ResolvedProfileIdentity) || LoadedSceneMode != SceneMode;
	const bool bPawnChanged = CurrentRuntime.ControlledPawn != Runtime.ControlledPawn;

	if (!bProfileChanged && !bForceReload)
	{
		CurrentRuntime = Runtime;
		if (bPawnChanged)
		{
			ReapplyCachedSnapshotToRuntime(Runtime);
		}

		RuntimeState = EYcPlayerPersistenceRuntimeState::Ready;
		UpdateReadyState(true);
		RequestDefaultLoadoutIfNeeded();
		return;
	}

	ExecuteHydrate(ResolvedProfileIdentity, Runtime, bForceReload);
}

bool UYcPlayerPersistenceComponent::ResolveCurrentProfileIdentity(FYcProfileIdentity& OutProfileIdentity) const
{
	OutProfileIdentity = FYcProfileIdentity();

	if (const AYcPlayerState* PlayerState = Cast<AYcPlayerState>(GetOwningPlayerController() ? GetOwningPlayerController()->PlayerState : nullptr))
	{
		FYcPlayerIdentitySnapshot ReplicatedIdentity;
		if (PlayerState->GetReplicatedPlayerIdentity(ReplicatedIdentity) && UYcAccountIdentityLibrary::HasActiveProfileIdentity(ReplicatedIdentity))
		{
			OutProfileIdentity = UYcAccountIdentityLibrary::GetActiveProfileIdentity(ReplicatedIdentity);
			return OutProfileIdentity.IsValid();
		}
	}

	if (const UYcAccountSessionSubsystem* SessionSubsystem = GetAccountSessionSubsystem())
	{
		const FYcPlayerIdentitySnapshot LocalIdentity = SessionSubsystem->GetCurrentPlayerIdentity();
		if (UYcAccountIdentityLibrary::HasActiveProfileIdentity(LocalIdentity))
		{
			OutProfileIdentity = UYcAccountIdentityLibrary::GetActiveProfileIdentity(LocalIdentity);
			return OutProfileIdentity.IsValid();
		}
	}

	return false;
}

FYcPlayerInventoryRuntime UYcPlayerPersistenceComponent::BuildRuntimeHandle() const
{
	FYcPlayerInventoryRuntime Runtime;
	Runtime.SceneMode = SceneMode;

	APlayerController* PlayerController = GetOwningPlayerController();
	Runtime.PlayerController = PlayerController;
	Runtime.PlayerState = IsValid(PlayerController) ? PlayerController->PlayerState : nullptr;
	Runtime.ControlledPawn = IsValid(PlayerController) ? PlayerController->GetPawn() : nullptr;
	Runtime.PlayerInventory = IsValid(PlayerController) ? UYcInventoryManagerComponent::FindInventoryManager(PlayerController) : nullptr;
	Runtime.QuickBarBridge = IsValid(PlayerController) ? Cast<UActorComponent>(PlayerController->GetComponentByClass(UYcQuickBarComponent::StaticClass())) : nullptr;
	Runtime.EquipmentBridge = IsValid(Runtime.ControlledPawn) ? Cast<UActorComponent>(Runtime.ControlledPawn->GetComponentByClass(UYcEquipmentSlotComponent::StaticClass())) : nullptr;
	Runtime.StashInventory = (SceneMode == EYcPlayerPersistenceSceneMode::OutOfMatch && IsValid(OutOfMatchStashActor))
		? UYcInventoryManagerComponent::FindInventoryManager(OutOfMatchStashActor)
		: nullptr;
	return Runtime;
}

bool UYcPlayerPersistenceComponent::CaptureCurrentRuntimeSnapshot()
{
	if (!bHasLoadedProfile || !bIsPersistenceReady || !CurrentRuntime.IsRuntimeValid())
	{
		return false;
	}

	UYcMetaInventorySubsystem* MetaInventorySubsystem = UYcMetaInventorySubsystem::Get(this);
	if (!MetaInventorySubsystem)
	{
		return false;
	}

	FYcMetaPlayerSnapshot Snapshot;
	if (!MetaInventorySubsystem->BuildPlayerSnapshotFromRuntime(CurrentRuntime, Snapshot))
	{
		return false;
	}

	CachedPlayerSnapshot = MoveTemp(Snapshot);
	bHasCachedSnapshot = true;
	return true;
}

bool UYcPlayerPersistenceComponent::ReapplyCachedSnapshotToRuntime(const FYcPlayerInventoryRuntime& Runtime)
{
	if (!bHasCachedSnapshot)
	{
		return false;
	}

	UYcMetaInventorySubsystem* MetaInventorySubsystem = UYcMetaInventorySubsystem::Get(this);
	if (!MetaInventorySubsystem)
	{
		return false;
	}

	return MetaInventorySubsystem->ApplyPlayerSnapshotToRuntime(Runtime, CachedPlayerSnapshot);
}

bool UYcPlayerPersistenceComponent::ExecuteHydrate(const FYcProfileIdentity& ProfileIdentity, const FYcPlayerInventoryRuntime& Runtime, const bool bForceReload)
{
	(void)bForceReload;

	UYcMetaInventorySubsystem* MetaInventorySubsystem = UYcMetaInventorySubsystem::Get(this);
	if (!MetaInventorySubsystem)
	{
		UE_LOG(LogYcPlayerPersistence, Error, TEXT("ExecuteHydrate failed: missing MetaInventorySubsystem owner=%s"), *GetNameSafe(GetOwner()));
		RuntimeState = EYcPlayerPersistenceRuntimeState::Error;
		UpdateReadyState(false);
		return false;
	}

	UE_LOG(LogYcPlayerPersistence, Verbose,
		TEXT("ExecuteHydrate begin owner=%s sceneMode=%d profileValid=%s runtimeValid=%s outOfMatch=%s inMatch=%s"),
		*GetNameSafe(GetOwner()),
		static_cast<int32>(SceneMode),
		ProfileIdentity.IsValid() ? TEXT("true") : TEXT("false"),
		Runtime.IsRuntimeValid() ? TEXT("true") : TEXT("false"),
		Runtime.SupportsOutOfMatchPersistence() ? TEXT("true") : TEXT("false"),
		Runtime.SupportsInMatchPersistence() ? TEXT("true") : TEXT("false"));
	RuntimeState = EYcPlayerPersistenceRuntimeState::Loading;
	UpdateReadyState(false);

	const bool bLoaded = (SceneMode == EYcPlayerPersistenceSceneMode::OutOfMatch)
		? MetaInventorySubsystem->SetupOutOfMatchContextAndLoad(ProfileIdentity, Runtime)
		: MetaInventorySubsystem->LoadPlayerLoadoutToInMatch(ProfileIdentity, Runtime);
	if (!bLoaded)
	{
		UE_LOG(LogYcPlayerPersistence, Warning,
			TEXT("ExecuteHydrate failed: owner=%s sceneMode=%d profileValid=%s"),
			*GetNameSafe(GetOwner()),
			static_cast<int32>(SceneMode),
			ProfileIdentity.IsValid() ? TEXT("true") : TEXT("false"));
		RuntimeState = EYcPlayerPersistenceRuntimeState::Error;
		UpdateReadyState(false);
		return false;
	}

	UE_LOG(LogYcPlayerPersistence, Verbose,
		TEXT("ExecuteHydrate succeeded owner=%s sceneMode=%d profileValid=%s"),
		*GetNameSafe(GetOwner()),
		static_cast<int32>(SceneMode),
		ProfileIdentity.IsValid() ? TEXT("true") : TEXT("false"));
	UpdatePostHydrateState(ProfileIdentity, Runtime);
	return true;
}

bool UYcPlayerPersistenceComponent::ExecuteSaveCurrentProfile()
{
	if (!bHasLoadedProfile || SceneMode != EYcPlayerPersistenceSceneMode::OutOfMatch)
	{
		UE_LOG(LogYcPlayerPersistence, Warning,
			TEXT("ExecuteSaveCurrentProfile rejected owner=%s hasLoadedProfile=%s sceneMode=%d currentProfileValid=%s ready=%s"),
			*GetNameSafe(GetOwner()),
			bHasLoadedProfile ? TEXT("true") : TEXT("false"),
			static_cast<int32>(SceneMode),
			CurrentProfileIdentity.IsValid() ? TEXT("true") : TEXT("false"),
			bIsPersistenceReady ? TEXT("true") : TEXT("false"));
		return false;
	}

	UYcMetaInventorySubsystem* MetaInventorySubsystem = UYcMetaInventorySubsystem::Get(this);
	if (!MetaInventorySubsystem)
	{
		UE_LOG(LogYcPlayerPersistence, Error, TEXT("ExecuteSaveCurrentProfile failed: missing MetaInventorySubsystem owner=%s"), *GetNameSafe(GetOwner()));
		return false;
	}

	UE_LOG(LogYcPlayerPersistence, Verbose,
		TEXT("ExecuteSaveCurrentProfile begin owner=%s profileValid=%s playerInventory=%s stash=%s quickBar=%s equipment=%s"),
		*GetNameSafe(GetOwner()),
		CurrentProfileIdentity.IsValid() ? TEXT("true") : TEXT("false"),
		*GetNameSafe(BuildRuntimeHandle().PlayerInventory),
		*GetNameSafe(BuildRuntimeHandle().StashInventory),
		*GetNameSafe(BuildRuntimeHandle().QuickBarBridge),
		*GetNameSafe(BuildRuntimeHandle().EquipmentBridge));
	RuntimeState = EYcPlayerPersistenceRuntimeState::Saving;
	const bool bSaved = MetaInventorySubsystem->SaveOutOfMatchContext(CurrentProfileIdentity, BuildRuntimeHandle());
	RuntimeState = bSaved ? EYcPlayerPersistenceRuntimeState::Ready : EYcPlayerPersistenceRuntimeState::Error;
	UE_LOG(LogYcPlayerPersistence, Verbose,
		TEXT("ExecuteSaveCurrentProfile end owner=%s profileValid=%s saved=%s"),
		*GetNameSafe(GetOwner()),
		CurrentProfileIdentity.IsValid() ? TEXT("true") : TEXT("false"),
		bSaved ? TEXT("true") : TEXT("false"));
	if (bSaved)
	{
		CaptureCurrentRuntimeSnapshot();
		UpdateReadyState(true);
	}
	else
	{
		UpdateReadyState(false);
	}
	return bSaved;
}

bool UYcPlayerPersistenceComponent::ExecuteLoadInMatchLoadout()
{
	if (!bHasLoadedProfile)
	{
		EvaluateFlow(true);
		return bHasLoadedProfile;
	}

	return ExecuteHydrate(CurrentProfileIdentity, BuildRuntimeHandle(), true);
}

bool UYcPlayerPersistenceComponent::ExecuteCommitMatchResult(const bool bExtractionSucceeded)
{
	if (!bHasLoadedProfile || SceneMode != EYcPlayerPersistenceSceneMode::InMatch)
	{
		return false;
	}

	UYcMetaInventorySubsystem* MetaInventorySubsystem = UYcMetaInventorySubsystem::Get(this);
	if (!MetaInventorySubsystem)
	{
		return false;
	}

	RuntimeState = EYcPlayerPersistenceRuntimeState::Committing;
	const bool bCommitted = MetaInventorySubsystem->CommitInMatchPlayerLoadoutToProfile(CurrentProfileIdentity, BuildRuntimeHandle(), bExtractionSucceeded);
	RuntimeState = bCommitted ? EYcPlayerPersistenceRuntimeState::Ready : EYcPlayerPersistenceRuntimeState::Error;
	if (bCommitted)
	{
		CaptureCurrentRuntimeSnapshot();
		UpdateReadyState(true);
	}
	else
	{
		UpdateReadyState(false);
	}
	return bCommitted;
}

void UYcPlayerPersistenceComponent::UpdateReadyState(const bool bNewReady)
{
	if (bIsPersistenceReady == bNewReady)
	{
		return;
	}

	bIsPersistenceReady = bNewReady;
	OnPersistenceReadyChanged.Broadcast(bIsPersistenceReady);
}

void UYcPlayerPersistenceComponent::UpdatePostHydrateState(const FYcProfileIdentity& ProfileIdentity, const FYcPlayerInventoryRuntime& Runtime)
{
	UE_LOG(LogYcPlayerPersistence, Verbose,
		TEXT("UpdatePostHydrateState owner=%s profileValid=%s sceneMode=%d snapshotReady(before)=%s"),
		*GetNameSafe(GetOwner()),
		ProfileIdentity.IsValid() ? TEXT("true") : TEXT("false"),
		static_cast<int32>(SceneMode),
		bHasCachedSnapshot ? TEXT("true") : TEXT("false"));
	CurrentRuntime = Runtime;
	CurrentProfileIdentity = ProfileIdentity;
	LoadedProfileIdentity = ProfileIdentity;
	LoadedSceneMode = SceneMode;
	bHasLoadedProfile = true;
	RuntimeState = EYcPlayerPersistenceRuntimeState::Ready;
	UpdateReadyState(true);
	CaptureCurrentRuntimeSnapshot();
	RequestDefaultLoadoutIfNeeded();
	OnPersistenceHydrated.Broadcast(ProfileIdentity);

	if (APlayerController* PlayerController = GetOwningPlayerController())
	{
		FYcPersistenceProfileMessage Message;
		Message.PlayerController = PlayerController;
		Message.LocalPlayer = PlayerController->GetLocalPlayer();
		Message.ReasonTag = YcPersistenceTags::Persistence_ProfileHydrated;

		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
		MessageSubsystem.BroadcastMessage(YcPersistenceTags::Persistence_ProfileHydrated, Message);
	}
}

void UYcPlayerPersistenceComponent::RequestDefaultLoadoutIfNeeded()
{
	if (!bExperienceReady || !bIsPersistenceReady || !bHasCachedSnapshot || !IsCachedSnapshotEmpty())
	{
		UE_LOG(LogYcPlayerPersistence, Verbose,
			TEXT("RequestDefaultLoadoutIfNeeded skipped owner=%s experienceReady=%s persistenceReady=%s hasCachedSnapshot=%s cachedEmpty=%s"),
			*GetNameSafe(GetOwner()),
			bExperienceReady ? TEXT("true") : TEXT("false"),
			bIsPersistenceReady ? TEXT("true") : TEXT("false"),
			bHasCachedSnapshot ? TEXT("true") : TEXT("false"),
			IsCachedSnapshotEmpty() ? TEXT("true") : TEXT("false"));
		return;
	}

	if (bDefaultLoadoutRequested)
	{
		UE_LOG(LogYcPlayerPersistence, Verbose, TEXT("RequestDefaultLoadoutIfNeeded skipped: already requested owner=%s"), *GetNameSafe(GetOwner()));
		return;
	}

	bDefaultLoadoutRequested = true;
	UE_LOG(LogYcPlayerPersistence, Verbose, TEXT("RequestDefaultLoadoutIfNeeded broadcast owner=%s"), *GetNameSafe(GetOwner()));
	OnDefaultLoadoutRequested.Broadcast();
}

void UYcPlayerPersistenceComponent::ClearDefaultLoadoutRequest()
{
	bDefaultLoadoutRequested = false;
}

bool UYcPlayerPersistenceComponent::IsCachedSnapshotEmpty() const
{
	return !bHasCachedSnapshot || IsPlayerSnapshotEmpty(CachedPlayerSnapshot);
}

APlayerController* UYcPlayerPersistenceComponent::GetOwningPlayerController() const
{
	return Cast<APlayerController>(GetOwner());
}

UYcAccountSessionSubsystem* UYcPlayerPersistenceComponent::GetAccountSessionSubsystem() const
{
	return UYcAccountSessionSubsystem::Get(this);
}
