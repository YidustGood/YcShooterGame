// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Character/YcPersistenceLibrary.h"

#include "Character/YcPersistenceMessages.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcPersistenceLibrary)

namespace
{
	static bool BroadcastPersistenceRequest(APlayerController* PlayerController, const FGameplayTag Channel, const FGameplayTag ReasonTag, const bool bImmediate, const bool bForceIfNotDirty, const bool bAllowDebounce, const bool bExtractionSucceeded)
	{
		if (!IsValid(PlayerController))
		{
			return false;
		}

		FYcPersistenceRequestMessage Message;
		Message.PlayerController = PlayerController;
		Message.LocalPlayer = PlayerController->GetLocalPlayer();
		Message.ReasonTag = ReasonTag;
		Message.bImmediate = bImmediate;
		Message.bForceIfNotDirty = bForceIfNotDirty;
		Message.bAllowDebounce = bAllowDebounce;
		Message.bExtractionSucceeded = bExtractionSucceeded;

		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(PlayerController);
		MessageSubsystem.BroadcastMessage(Channel, Message);
		return true;
	}
}

bool UYcPersistenceLibrary::MarkDirty(APlayerController* PlayerController, const FGameplayTag ReasonTag)
{
	return BroadcastPersistenceRequest(PlayerController, YcPersistenceTags::Persistence_MarkDirty, ReasonTag, false, false, true, false);
}

bool UYcPersistenceLibrary::RequestAutosave(APlayerController* PlayerController, const FGameplayTag ReasonTag, const bool bAllowDebounce)
{
	return BroadcastPersistenceRequest(PlayerController, YcPersistenceTags::Persistence_RequestAutosave, ReasonTag, false, false, bAllowDebounce, false);
}

bool UYcPersistenceLibrary::RequestFlushSave(APlayerController* PlayerController, const FGameplayTag ReasonTag, const bool bForceIfNotDirty)
{
	return BroadcastPersistenceRequest(PlayerController, YcPersistenceTags::Persistence_RequestFlushSave, ReasonTag, true, bForceIfNotDirty, false, false);
}

bool UYcPersistenceLibrary::RequestCommitMatchResult(APlayerController* PlayerController, const bool bExtractionSucceeded, const FGameplayTag ReasonTag)
{
	return BroadcastPersistenceRequest(PlayerController, YcPersistenceTags::Persistence_RequestCommitMatchResult, ReasonTag, true, true, false, bExtractionSucceeded);
}

bool UYcPersistenceLibrary::RequestCommitMatchResultSimple(APlayerController* PlayerController, const bool bExtractionSucceeded)
{
	return RequestCommitMatchResult(PlayerController, bExtractionSucceeded, FGameplayTag());
}
