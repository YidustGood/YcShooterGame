// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Character/YcSaveCoordinatorSubsystem.h"

#include "Character/YcPersistenceMessages.h"
#include "Character/YcPlayerPersistenceComponent.h"
#include "Components/ActorComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "System/YcMetaInventoryTypes.h"
#include "YcEquipmentSlotComponent.h"
#include "YcInventoryItemInstance.h"
#include "YcInventoryManagerComponent.h"
#include "YcInventoryOperationRouterComponent.h"
#include "YcQuickBarComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcSaveCoordinatorSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogYcSaveCoordinator, Log, All);

namespace
{
	double GetNowSeconds()
	{
		return FPlatformTime::Seconds();
	}
}

void UYcSaveCoordinatorSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogYcSaveCoordinator, Verbose, TEXT("Initialize world=%s"), *GetNameSafe(GetWorld()));
	EnsureMessageListenersRegistered();
}

void UYcSaveCoordinatorSubsystem::Deinitialize()
{
	UE_LOG(LogYcSaveCoordinator, Verbose, TEXT("Deinitialize world=%s playerStateCount=%d"), *GetNameSafe(GetWorld()), PlayerStates.Num());
	FlushAllTrackedPlayers(true);

	for (FGameplayMessageListenerHandle& Handle : ListenerHandles)
	{
		if (Handle.IsValid())
		{
			Handle.Unregister();
		}
	}
	ListenerHandles.Reset();
	bMessageListenersRegistered = false;
	PlayerStates.Reset();

	Super::Deinitialize();
}

bool UYcSaveCoordinatorSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return Super::ShouldCreateSubsystem(Outer);
}

void UYcSaveCoordinatorSubsystem::Tick(float DeltaTime)
{
	(void)DeltaTime;
	// @TODO 这里Tick可以优化为事件驱动, 或者Timer驱动, Tick的话频率太高了没必要
	EnsureMessageListenersRegistered();

	const double NowSeconds = GetNowSeconds();
	for (TPair<TWeakObjectPtr<APlayerController>, FYcSaveCoordinatorPlayerState>& Pair : PlayerStates)
	{
		FYcSaveCoordinatorPlayerState& PlayerState = Pair.Value;
		if (!PlayerState.PlayerController.IsValid())
		{
			continue;
		}

		if (PlayerState.bPendingFlushWhenReady)
		{
			if (UYcPlayerPersistenceComponent* PersistenceComponent = GetPersistenceComponent(PlayerState.PlayerController.Get()))
			{
				if (PersistenceComponent->CanSaveCurrentProfile())
				{
					PlayerState.bPendingFlushWhenReady = false;
					TryFlushPlayerState(PlayerState, true);
					continue;
				}
			}
		}

		if (PlayerState.bDirty && PlayerState.AutosaveDeadlineSeconds > 0.0 && NowSeconds >= PlayerState.AutosaveDeadlineSeconds)
		{
			TryFlushPlayerState(PlayerState, false);
		}
	}

	PruneDeadStates();
}

TStatId UYcSaveCoordinatorSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UYcSaveCoordinatorSubsystem, STATGROUP_Tickables);
}

bool UYcSaveCoordinatorSubsystem::IsTickable() const
{
	const UWorld* World = GetWorld();
	return IsValid(World) && !World->IsNetMode(NM_DedicatedServer);
}

UYcSaveCoordinatorSubsystem* UYcSaveCoordinatorSubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	if (UWorld* World = WorldContextObject->GetWorld())
	{
		return World->GetSubsystem<UYcSaveCoordinatorSubsystem>();
	}
	return nullptr;
}

bool UYcSaveCoordinatorSubsystem::FlushPlayer(APlayerController* PlayerController, const bool bForceIfNotDirty, const FGameplayTag ReasonTag)
{
	if (!IsValid(PlayerController))
	{
		return false;
	}

	FYcSaveCoordinatorPlayerState& PlayerState = GetOrCreateState(PlayerController);
	if (ReasonTag.IsValid())
	{
		PlayerState.LastDirtyReasonTag = ReasonTag;
	}
	return TryFlushPlayerState(PlayerState, bForceIfNotDirty);
}

void UYcSaveCoordinatorSubsystem::MarkPlayerDirty(APlayerController* PlayerController, const FGameplayTag ReasonTag, const bool bScheduleAutosave)
{
	if (!IsValid(PlayerController))
	{
		UE_LOG(LogYcSaveCoordinator, Warning, TEXT("MarkPlayerDirty ignored: invalid controller"));
		return;
	}

	UYcPlayerPersistenceComponent* PersistenceComponent = GetPersistenceComponent(PlayerController);
	if (!IsValid(PersistenceComponent) || !PersistenceComponent->CanSaveCurrentProfile())
	{
		UE_LOG(LogYcSaveCoordinator, Warning,
			TEXT("MarkPlayerDirty ignored: controller=%s persistence=%s canSave=%s sceneMode=%d"),
			*GetNameSafe(PlayerController),
			*GetNameSafe(PersistenceComponent),
			PersistenceComponent && PersistenceComponent->CanSaveCurrentProfile() ? TEXT("true") : TEXT("false"),
			PersistenceComponent ? static_cast<int32>(PersistenceComponent->GetCurrentSceneMode()) : -1);
		return;
	}

	FYcSaveCoordinatorPlayerState& PlayerState = GetOrCreateState(PlayerController);
	PlayerState.bDirty = true;
	PlayerState.LastDirtyReasonTag = ReasonTag;
	PlayerState.LastFailureReasonTag = FGameplayTag();
	UE_LOG(LogYcSaveCoordinator, Verbose,
		TEXT("MarkPlayerDirty controller=%s reason=%s scheduleAutosave=%s"),
		*GetNameSafe(PlayerController),
		ReasonTag.IsValid() ? *ReasonTag.ToString() : TEXT("<None>"),
		bScheduleAutosave ? TEXT("true") : TEXT("false"));

	if (bScheduleAutosave)
	{
		ScheduleAutosave(PlayerState);
	}
}

bool UYcSaveCoordinatorSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

bool UYcSaveCoordinatorSubsystem::EnsureMessageListenersRegistered()
{
	if (bMessageListenersRegistered)
	{
		return true;
	}

	UWorld* World = GetWorld();
	if (!IsValid(World) || !UGameplayMessageSubsystem::HasInstance(World))
	{
		return false;
	}

	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(World);
	ListenerHandles.Add(MessageSubsystem.RegisterListener<FYcInventoryItemChangeMessage>(TAG_Yc_Inventory_Message_StackChanged, this, &ThisClass::HandleInventoryStackChanged));
	ListenerHandles.Add(MessageSubsystem.RegisterListener<FYcEquipmentSlotChangedMessage>(TAG_Yc_EquipmentSlot_Message_SlotChanged, this, &ThisClass::HandleEquipmentSlotChanged));
	ListenerHandles.Add(MessageSubsystem.RegisterListener<FYcQuickBarSlotsChangedMessage>(TAG_Yc_QuickBar_Message_SlotsChanged, this, &ThisClass::HandleQuickBarSlotsChanged));
	ListenerHandles.Add(MessageSubsystem.RegisterListener<FYcInventoryProjectedStateChangedMessage>(TAG_Yc_Inventory_Message_ProjectedState_Changed, this, &ThisClass::HandleProjectedStateChanged));
	ListenerHandles.Add(MessageSubsystem.RegisterListener<FYcPersistenceRequestMessage>(YcPersistenceTags::Persistence_MarkDirty, this, &ThisClass::HandlePersistenceRequest));
	ListenerHandles.Add(MessageSubsystem.RegisterListener<FYcPersistenceRequestMessage>(YcPersistenceTags::Persistence_RequestAutosave, this, &ThisClass::HandlePersistenceRequest));
	ListenerHandles.Add(MessageSubsystem.RegisterListener<FYcPersistenceRequestMessage>(YcPersistenceTags::Persistence_RequestFlushSave, this, &ThisClass::HandlePersistenceRequest));
	ListenerHandles.Add(MessageSubsystem.RegisterListener<FYcPersistenceRequestMessage>(YcPersistenceTags::Persistence_RequestCommitMatchResult, this, &ThisClass::HandlePersistenceRequest));
	ListenerHandles.Add(MessageSubsystem.RegisterListener<FYcPersistenceProfileMessage>(YcPersistenceTags::Persistence_ProfileHydrated, this, &ThisClass::HandleProfileHydrated));
	ListenerHandles.Add(MessageSubsystem.RegisterListener<FYcPersistenceProfileMessage>(YcPersistenceTags::Persistence_ProfileChanged, this, &ThisClass::HandleProfileChanged));
	bMessageListenersRegistered = true;
	UE_LOG(LogYcSaveCoordinator, Verbose, TEXT("Gameplay message listeners registered for world=%s"), *GetNameSafe(World));
	return true;
}

void UYcSaveCoordinatorSubsystem::HandleInventoryStackChanged(FGameplayTag Channel, const FYcInventoryItemChangeMessage& Message)
{
	(void)Channel;
	if (!IsValid(Message.InventoryOwner))
	{
		UE_LOG(LogYcSaveCoordinator, Verbose, TEXT("HandleInventoryStackChanged ignored: invalid inventory owner"));
		return;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
		{
			continue;
		}

		if (IsPersistenceManagedChange(PlayerController, Message.InventoryOwner))
		{
			UE_LOG(LogYcSaveCoordinator, Verbose,
				TEXT("StackChanged matched controller=%s inventoryOwner=%s item=%s delta=%d newCount=%d"),
				*GetNameSafe(PlayerController),
				*GetNameSafe(Message.InventoryOwner),
				Message.ItemInstance ? TEXT("Valid") : TEXT("None"),
				Message.Delta,
				Message.NewCount);
			MarkPlayerDirty(PlayerController, YcPersistenceTags::Persistence_MarkDirty, true);
		}
	}
}

void UYcSaveCoordinatorSubsystem::HandleEquipmentSlotChanged(FGameplayTag Channel, const FYcEquipmentSlotChangedMessage& Message)
{
	(void)Channel;
	if (!IsValid(Message.Owner))
	{
		return;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
		{
			continue;
		}

		if (IsPersistenceManagedChange(PlayerController, Message.Owner))
		{
			UE_LOG(LogYcSaveCoordinator, Verbose,
				TEXT("EquipmentSlotChanged matched controller=%s owner=%s slot=%s item=%s"),
				*GetNameSafe(PlayerController),
				*GetNameSafe(Message.Owner),
				Message.SlotTag.IsValid() ? *Message.SlotTag.ToString() : TEXT("<None>"),
				Message.ItemInstance ? TEXT("Valid") : TEXT("None"));
			MarkPlayerDirty(PlayerController, YcPersistenceTags::Persistence_MarkDirty, true);
		}
	}
}

void UYcSaveCoordinatorSubsystem::HandleQuickBarSlotsChanged(FGameplayTag Channel, const FYcQuickBarSlotsChangedMessage& Message)
{
	(void)Channel;
	if (!IsValid(Message.Owner))
	{
		return;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
		{
			continue;
		}

		if (IsPersistenceManagedChange(PlayerController, Message.Owner))
		{
			UE_LOG(LogYcSaveCoordinator, Verbose,
				TEXT("QuickBarSlotsChanged matched controller=%s owner=%s"),
				*GetNameSafe(PlayerController),
				*GetNameSafe(Message.Owner));
			MarkPlayerDirty(PlayerController, YcPersistenceTags::Persistence_MarkDirty, true);
		}
	}
}

void UYcSaveCoordinatorSubsystem::HandleProjectedStateChanged(FGameplayTag Channel, const FYcInventoryProjectedStateChangedMessage& Message)
{
	(void)Channel;

	if (Message.Event != EYcInventoryOperationEvent::Acked || Message.Operation.OpType != TEXT("Inventory.SwapGrid"))
	{
		return;
	}

	UE_LOG(LogYcSaveCoordinator, Verbose,
		TEXT("ProjectedStateChanged Acked opType=%s source=%s target=%s item=%s opId=%lld"),
		*Message.Operation.OpType.ToString(),
		*GetNameSafe(Message.Operation.SourceInventory),
		*GetNameSafe(Message.Operation.TargetInventory),
		Message.Operation.ItemInstance ? TEXT("Valid") : TEXT("None"),
		Message.Operation.OpId);

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!IsValid(PlayerController) || !PlayerController->IsLocalController())
		{
			continue;
		}

		if ((IsValid(Message.Operation.SourceInventory) && IsPersistenceManagedChange(PlayerController, Message.Operation.SourceInventory)) ||
			(IsValid(Message.Operation.TargetInventory) && IsPersistenceManagedChange(PlayerController, Message.Operation.TargetInventory)))
		{
			UE_LOG(LogYcSaveCoordinator, Verbose,
				TEXT("ProjectedStateChanged matched controller=%s for Inventory.SwapGrid"),
				*GetNameSafe(PlayerController));
			MarkPlayerDirty(PlayerController, YcPersistenceTags::Persistence_MarkDirty, true);
		}
	}
}

void UYcSaveCoordinatorSubsystem::HandlePersistenceRequest(FGameplayTag Channel, const FYcPersistenceRequestMessage& Message)
{
	APlayerController* PlayerController = Message.PlayerController.Get();
	if (!IsValid(PlayerController))
	{
		if (const ULocalPlayer* LocalPlayer = Message.LocalPlayer.Get())
		{
			PlayerController = LocalPlayer->GetPlayerController(GetWorld());
		}
	}
	if (!IsValid(PlayerController))
	{
		UE_LOG(LogYcSaveCoordinator, Warning,
			TEXT("PersistenceRequest ignored: unresolved controller channel=%s reason=%s"),
			Channel.IsValid() ? *Channel.ToString() : TEXT("<None>"),
			Message.ReasonTag.IsValid() ? *Message.ReasonTag.ToString() : TEXT("<None>"));
		return;
	}

	UE_LOG(LogYcSaveCoordinator, Verbose,
		TEXT("PersistenceRequest channel=%s controller=%s reason=%s immediate=%s forceIfNotDirty=%s allowDebounce=%s extractionSucceeded=%s"),
		Channel.IsValid() ? *Channel.ToString() : TEXT("<None>"),
		*GetNameSafe(PlayerController),
		Message.ReasonTag.IsValid() ? *Message.ReasonTag.ToString() : TEXT("<None>"),
		Message.bImmediate ? TEXT("true") : TEXT("false"),
		Message.bForceIfNotDirty ? TEXT("true") : TEXT("false"),
		Message.bAllowDebounce ? TEXT("true") : TEXT("false"),
		Message.bExtractionSucceeded ? TEXT("true") : TEXT("false"));

	FYcSaveCoordinatorPlayerState& PlayerState = GetOrCreateState(PlayerController);
	if (Message.ReasonTag.IsValid())
	{
		PlayerState.LastDirtyReasonTag = Message.ReasonTag;
	}

	if (Channel == YcPersistenceTags::Persistence_MarkDirty)
	{
		MarkPlayerDirty(PlayerController, Message.ReasonTag, true);
		return;
	}

	if (Channel == YcPersistenceTags::Persistence_RequestAutosave)
	{
		MarkPlayerDirty(PlayerController, Message.ReasonTag, Message.bAllowDebounce);
		if (!Message.bAllowDebounce)
		{
			TryFlushPlayerState(PlayerState, Message.bForceIfNotDirty);
		}
		return;
	}

	if (Channel == YcPersistenceTags::Persistence_RequestFlushSave)
	{
		if (!TryFlushPlayerState(PlayerState, Message.bForceIfNotDirty))
		{
			PlayerState.bPendingFlushWhenReady = true;
		}
		return;
	}

	if (Channel == YcPersistenceTags::Persistence_RequestCommitMatchResult)
	{
		if (UYcPlayerPersistenceComponent* PersistenceComponent = GetPersistenceComponent(PlayerController))
		{
			PersistenceComponent->CommitMatchResult(Message.bExtractionSucceeded);
		}
	}
}

void UYcSaveCoordinatorSubsystem::HandleProfileHydrated(FGameplayTag Channel, const FYcPersistenceProfileMessage& Message)
{
	(void)Channel;
	HandleProfileLifecycleEvent(Message);
}

void UYcSaveCoordinatorSubsystem::HandleProfileChanged(FGameplayTag Channel, const FYcPersistenceProfileMessage& Message)
{
	(void)Channel;
	HandleProfileLifecycleEvent(Message);
}

void UYcSaveCoordinatorSubsystem::HandleProfileLifecycleEvent(const FYcPersistenceProfileMessage& Message)
{
	APlayerController* PlayerController = Message.PlayerController.Get();
	if (!IsValid(PlayerController))
	{
		if (const ULocalPlayer* LocalPlayer = Message.LocalPlayer.Get())
		{
			PlayerController = LocalPlayer->GetPlayerController(GetWorld());
		}
	}
	if (!IsValid(PlayerController))
	{
		return;
	}

	FYcSaveCoordinatorPlayerState& PlayerState = GetOrCreateState(PlayerController);
	PlayerState.bDirty = false;
	PlayerState.AutosaveDeadlineSeconds = 0.0;
	PlayerState.LastFailureReasonTag = FGameplayTag();
	if (Message.ReasonTag.IsValid())
	{
		PlayerState.LastDirtyReasonTag = Message.ReasonTag;
	}
}

bool UYcSaveCoordinatorSubsystem::IsPersistenceManagedChange(APlayerController* PlayerController, const UObject* SourceObject) const
{
	if (!IsValid(PlayerController) || !IsValid(SourceObject))
	{
		return false;
	}

	if (UYcPlayerPersistenceComponent* PersistenceComponent = GetPersistenceComponent(PlayerController))
	{
		return MatchesRuntimeObject(PersistenceComponent, SourceObject);
	}
	return false;
}

bool UYcSaveCoordinatorSubsystem::MatchesRuntimeObject(UYcPlayerPersistenceComponent* PersistenceComponent, const UObject* SourceObject) const
{
	if (!IsValid(PersistenceComponent) || !IsValid(SourceObject) || !PersistenceComponent->CanSaveCurrentProfile())
	{
		return false;
	}

	const FYcPlayerInventoryRuntime Runtime = PersistenceComponent->GetCurrentRuntimeHandle();
	if (!Runtime.SupportsOutOfMatchPersistence())
	{
		return false;
	}

	if (SourceObject == Runtime.PlayerInventory || SourceObject == Runtime.StashInventory || SourceObject == Runtime.QuickBarBridge || SourceObject == Runtime.EquipmentBridge)
	{
		return true;
	}

	if (const UActorComponent* Component = Cast<UActorComponent>(SourceObject))
	{
		const AActor* Owner = Component->GetOwner();
		return Owner == Runtime.PlayerController.Get() || Owner == Runtime.PlayerState.Get() || Owner == Runtime.ControlledPawn.Get();
	}

	if (const AActor* Actor = Cast<AActor>(SourceObject))
	{
		return Actor == Runtime.PlayerController.Get() || Actor == Runtime.PlayerState.Get() || Actor == Runtime.ControlledPawn.Get();
	}

	return false;
}

UYcPlayerPersistenceComponent* UYcSaveCoordinatorSubsystem::GetPersistenceComponent(APlayerController* PlayerController) const
{
	if (!IsValid(PlayerController))
	{
		return nullptr;
	}

	return Cast<UYcPlayerPersistenceComponent>(PlayerController->GetComponentByClass(UYcPlayerPersistenceComponent::StaticClass()));
}

void UYcSaveCoordinatorSubsystem::ScheduleAutosave(FYcSaveCoordinatorPlayerState& PlayerState)
{
	PlayerState.AutosaveDeadlineSeconds = GetNowSeconds() + FMath::Max(0.1f, AutosaveDebounceSeconds);
	UE_LOG(LogYcSaveCoordinator, Verbose,
		TEXT("ScheduleAutosave controller=%s deadline=%.3f debounce=%.3f"),
		*GetNameSafe(PlayerState.PlayerController.Get()),
		PlayerState.AutosaveDeadlineSeconds,
		AutosaveDebounceSeconds);
}

bool UYcSaveCoordinatorSubsystem::TryFlushPlayerState(FYcSaveCoordinatorPlayerState& PlayerState, const bool bForceIfNotDirty)
{
	APlayerController* PlayerController = PlayerState.PlayerController.Get();
	if (!IsValid(PlayerController))
	{
		UE_LOG(LogYcSaveCoordinator, Warning, TEXT("TryFlushPlayerState failed: invalid controller"));
		return false;
	}

	UYcPlayerPersistenceComponent* PersistenceComponent = GetPersistenceComponent(PlayerController);
	if (!IsValid(PersistenceComponent))
	{
		UE_LOG(LogYcSaveCoordinator, Warning, TEXT("TryFlushPlayerState failed: missing persistence component controller=%s"), *GetNameSafe(PlayerController));
		return false;
	}

	if (PlayerState.bSaveInFlight)
	{
		PlayerState.bPendingFlushAfterCurrentSave = true;
		UE_LOG(LogYcSaveCoordinator, Verbose, TEXT("TryFlushPlayerState deferred: save in flight controller=%s"), *GetNameSafe(PlayerController));
		return false;
	}

	if (!PersistenceComponent->CanSaveCurrentProfile())
	{
		PlayerState.bPendingFlushWhenReady = true;
		UE_LOG(LogYcSaveCoordinator, Warning,
			TEXT("TryFlushPlayerState blocked: canSave=false controller=%s sceneMode=%d dirty=%s force=%s"),
			*GetNameSafe(PlayerController),
			static_cast<int32>(PersistenceComponent->GetCurrentSceneMode()),
			PlayerState.bDirty ? TEXT("true") : TEXT("false"),
			bForceIfNotDirty ? TEXT("true") : TEXT("false"));
		return false;
	}

	if (!PlayerState.bDirty && !bForceIfNotDirty)
	{
		PlayerState.AutosaveDeadlineSeconds = 0.0;
		UE_LOG(LogYcSaveCoordinator, Verbose, TEXT("TryFlushPlayerState skipped: not dirty controller=%s"), *GetNameSafe(PlayerController));
		return true;
	}

	UE_LOG(LogYcSaveCoordinator, Verbose,
		TEXT("TryFlushPlayerState begin controller=%s dirty=%s force=%s reason=%s"),
		*GetNameSafe(PlayerController),
		PlayerState.bDirty ? TEXT("true") : TEXT("false"),
		bForceIfNotDirty ? TEXT("true") : TEXT("false"),
		PlayerState.LastDirtyReasonTag.IsValid() ? *PlayerState.LastDirtyReasonTag.ToString() : TEXT("<None>"));
	PlayerState.bSaveInFlight = true;
	const bool bSaved = PersistenceComponent->SaveCurrentProfile();
	PlayerState.bSaveInFlight = false;
	UE_LOG(LogYcSaveCoordinator, Verbose,
		TEXT("TryFlushPlayerState end controller=%s saved=%s"),
		*GetNameSafe(PlayerController),
		bSaved ? TEXT("true") : TEXT("false"));

	if (bSaved)
	{
		PlayerState.bDirty = false;
		PlayerState.AutosaveDeadlineSeconds = 0.0;
		PlayerState.LastFailureReasonTag = FGameplayTag();
		PlayerState.LastSuccessfulSaveTimeSeconds = GetNowSeconds();
	}
	else
	{
		PlayerState.bDirty = true;
		PlayerState.AutosaveDeadlineSeconds = 0.0;
		PlayerState.LastFailureReasonTag = PlayerState.LastDirtyReasonTag;
		PlayerState.bPendingFlushWhenReady = true;
	}

	if (PlayerState.bPendingFlushAfterCurrentSave)
	{
		PlayerState.bPendingFlushAfterCurrentSave = false;
		if (!bSaved || PlayerState.bDirty)
		{
			PlayerState.bPendingFlushWhenReady = true;
		}
		else
		{
			TryFlushPlayerState(PlayerState, true);
		}
	}

	return bSaved;
}

void UYcSaveCoordinatorSubsystem::FlushAllTrackedPlayers(const bool bForceIfNotDirty)
{
	for (TPair<TWeakObjectPtr<APlayerController>, FYcSaveCoordinatorPlayerState>& Pair : PlayerStates)
	{
		FYcSaveCoordinatorPlayerState& PlayerState = Pair.Value;
		if (PlayerState.PlayerController.IsValid())
		{
			TryFlushPlayerState(PlayerState, bForceIfNotDirty);
		}
	}
}

void UYcSaveCoordinatorSubsystem::PruneDeadStates()
{
	TArray<TWeakObjectPtr<APlayerController>> DeadKeys;
	for (const TPair<TWeakObjectPtr<APlayerController>, FYcSaveCoordinatorPlayerState>& Pair : PlayerStates)
	{
		if (!Pair.Key.IsValid())
		{
			DeadKeys.Add(Pair.Key);
		}
	}

	for (const TWeakObjectPtr<APlayerController>& DeadKey : DeadKeys)
	{
		PlayerStates.Remove(DeadKey);
	}
}

FYcSaveCoordinatorPlayerState& UYcSaveCoordinatorSubsystem::GetOrCreateState(APlayerController* PlayerController)
{
	TWeakObjectPtr<APlayerController> Key(PlayerController);
	FYcSaveCoordinatorPlayerState& PlayerState = PlayerStates.FindOrAdd(Key);
	PlayerState.PlayerController = PlayerController;
	return PlayerState;
}
