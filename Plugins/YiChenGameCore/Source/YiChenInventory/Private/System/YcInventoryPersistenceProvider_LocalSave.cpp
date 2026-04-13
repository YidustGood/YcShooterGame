// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcInventoryPersistenceProvider_LocalSave.h"

#include "System/YcMetaInventorySaveGame.h"
#include "Kismet/GameplayStatics.h"
#include "YiChenInventory.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcInventoryPersistenceProvider_LocalSave)

namespace
{
	static constexpr int32 SaveUserIndex = 0;
}

FString UYcInventoryPersistenceProvider_LocalSave::BuildSlotName(const FString& AccountId)
{
	return FString::Printf(TEXT("YcMetaInventory_%s"), AccountId.IsEmpty() ? TEXT("Default") : *AccountId);
}

bool UYcInventoryPersistenceProvider_LocalSave::LoadSnapshot(const FString& AccountId, FYcMetaInventoryRootSnapshot& OutSnapshot)
{
	const FString SlotName = BuildSlotName(AccountId);
	if (!UGameplayStatics::DoesSaveGameExist(SlotName, SaveUserIndex))
	{
		return false;
	}

	USaveGame* SaveGameObject = UGameplayStatics::LoadGameFromSlot(SlotName, SaveUserIndex);
	UYcMetaInventorySaveGame* InventorySave = Cast<UYcMetaInventorySaveGame>(SaveGameObject);
	if (!InventorySave)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("LoadSnapshot failed: slot '%s' exists but type mismatch."), *SlotName);
		return false;
	}

	OutSnapshot = InventorySave->RootSnapshot;
	if (OutSnapshot.AccountId.IsEmpty())
	{
		OutSnapshot.AccountId = AccountId;
	}
	return true;
}

bool UYcInventoryPersistenceProvider_LocalSave::SaveSnapshot(const FString& AccountId, const FYcMetaInventoryRootSnapshot& Snapshot)
{
	const FString SlotName = BuildSlotName(AccountId);

	UYcMetaInventorySaveGame* SaveObject = Cast<UYcMetaInventorySaveGame>(UGameplayStatics::CreateSaveGameObject(UYcMetaInventorySaveGame::StaticClass()));
	if (!SaveObject)
	{
		UE_LOG(LogYcInventory, Error, TEXT("SaveSnapshot failed: CreateSaveGameObject returned null."));
		return false;
	}

	SaveObject->RootSnapshot = Snapshot;
	SaveObject->RootSnapshot.AccountId = AccountId;

	const bool bSaved = UGameplayStatics::SaveGameToSlot(SaveObject, SlotName, SaveUserIndex);
	if (!bSaved)
	{
		UE_LOG(LogYcInventory, Error, TEXT("SaveSnapshot failed: slot '%s' write failed."), *SlotName);
	}
	return bSaved;
}
