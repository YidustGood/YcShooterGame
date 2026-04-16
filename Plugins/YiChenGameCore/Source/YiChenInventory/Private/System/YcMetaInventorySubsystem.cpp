// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcMetaInventorySubsystem.h"

#include "System/YcProfileSaveSubsystem.h"
#include "System/YcInventoryPersistenceExtensionProvider.h"
#include "System/YcInventoryPersistenceExtensionRegistry.h"
#include "System/YcMetaInventoryBridgeInterfaces.h"
#include "System/YcInventorySaveDomainProvider.h"
#include "System/YcMetaInventoryItemRecordCodec.h"
#include "System/YcInventorySceneContext.h"
#include "System/YcMetaInventoryVersion.h"
#include "YcInventoryItemInstance.h"
#include "YcInventoryManagerComponent.h"
#include "YcInventoryOperationTypes.h"
#include "Components/ActorComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "YiChenInventory.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcMetaInventorySubsystem)

namespace
{
	static const FName OperationStateChangedTagName(TEXT("Yc.Inventory.Message.Operation.StateChanged"));

	static bool SerializeInventorySnapshotToBytes(const FYcMetaInventoryRootSnapshot& Snapshot, TArray<uint8>& OutBytes)
	{
		OutBytes.Reset();
		FMemoryWriter MemWriter(OutBytes, true);
		FObjectAndNameAsStringProxyArchive ArWriter(MemWriter, false);
		FYcMetaInventoryRootSnapshot SnapshotCopy = Snapshot;
		FYcMetaInventoryRootSnapshot::StaticStruct()->SerializeItem(ArWriter, &SnapshotCopy, nullptr);
		return !ArWriter.IsError();
	}

	static bool DeserializeInventorySnapshotFromBytes(const TArray<uint8>& InBytes, FYcMetaInventoryRootSnapshot& OutSnapshot)
	{
		TArray<uint8> Buffer = InBytes;
		FMemoryReader MemReader(Buffer, true);
		FObjectAndNameAsStringProxyArchive ArReader(MemReader, true);
		FYcMetaInventoryRootSnapshot::StaticStruct()->SerializeItem(ArReader, &OutSnapshot, nullptr);
		return !ArReader.IsError();
	}
}

UYcMetaInventorySubsystem* UYcMetaInventorySubsystem::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	if (const UWorld* World = WorldContextObject->GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			return GameInstance->GetSubsystem<UYcMetaInventorySubsystem>();
		}
	}
	return nullptr;
}

void UYcMetaInventorySubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UWorld* World = GetWorld())
	{
		const FGameplayTag OperationStateChangedTag = FGameplayTag::RequestGameplayTag(OperationStateChangedTagName);
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(World);
		OperationStateListener = MessageSubsystem.RegisterListener<FYcInventoryOperationStateMessage>(
			OperationStateChangedTag,
			this,
			&ThisClass::OnOperationStateChanged);
	}
}

void UYcMetaInventorySubsystem::Deinitialize()
{
	SaveDirtyProfiles();
	OperationStateListener.Unregister();
	DirtyProfiles.Empty();
	UnknownItemExtensionPayloads.Empty();
	SceneContexts.Empty();
	ContextByProfileKey.Empty();

	Super::Deinitialize();
}

void UYcMetaInventorySubsystem::RegisterSceneContext(UYcInventorySceneContext* Context)
{
	if (!IsValid(Context))
	{
		return;
	}

	if (!OperationStateListener.IsValid())
	{
		if (UWorld* World = GetWorld())
		{
			const FGameplayTag OperationStateChangedTag = FGameplayTag::RequestGameplayTag(OperationStateChangedTagName);
			UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(World);
			OperationStateListener = MessageSubsystem.RegisterListener<FYcInventoryOperationStateMessage>(
				OperationStateChangedTag,
				this,
				&ThisClass::OnOperationStateChanged);
		}
	}

	SceneContexts.Remove(Context);
	SceneContexts.Add(Context);
	RegisterContextProfileKey(Context);
}

void UYcMetaInventorySubsystem::UnregisterSceneContext(UYcInventorySceneContext* Context)
{
	if (!Context)
	{
		return;
	}

	SceneContexts.Remove(Context);
	UnregisterContextProfileKey(Context);
}

bool UYcMetaInventorySubsystem::ValidateOutOfMatchContext(const UYcInventorySceneContext* Context, const TCHAR* Caller) const
{
	if (!IsValid(Context))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("%s: context is invalid."), Caller);
		return false;
	}

	if (!Context->IsValidForOutOfMatchPersistence())
	{
		UE_LOG(LogYcInventory, Warning, TEXT("%s: context is not eligible for out-of-match persistence. SceneType=%d, AccountId='%s', bRequirePersistenceCommit=%d"),
			Caller, static_cast<int32>(Context->SceneType), *Context->AccountId, Context->bRequirePersistenceCommit ? 1 : 0);
		return false;
	}

	return true;
}

bool UYcMetaInventorySubsystem::ValidateInMatchLoadoutRequest(const FString& AccountId, const FString& ProfileId, const AActor* ContextOwner, const UYcInventoryManagerComponent* InMatchPlayerInventory, const bool bRequireRuntimeObjects, const TCHAR* Caller) const
{
	if (AccountId.IsEmpty())
	{
		UE_LOG(LogYcInventory, Warning, TEXT("%s: invalid account id."), Caller);
		return false;
	}
	if (ResolveProfileId(ProfileId).IsEmpty())
	{
		UE_LOG(LogYcInventory, Warning, TEXT("%s: invalid profile id."), Caller);
		return false;
	}

	if (bRequireRuntimeObjects && (!IsValid(ContextOwner) || !IsValid(InMatchPlayerInventory)))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("%s: invalid runtime args."), Caller);
		return false;
	}

	return true;
}

FString UYcMetaInventorySubsystem::ResolveProfileId(const FString& ProfileId) const
{
	return ProfileId.IsEmpty() ? DefaultProfileId : ProfileId;
}

UYcProfileSaveSubsystem* UYcMetaInventorySubsystem::GetProfileSaveSubsystem() const
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		return GameInstance->GetSubsystem<UYcProfileSaveSubsystem>();
	}
	return nullptr;
}

void UYcMetaInventorySubsystem::RegisterContextProfileKey(UYcInventorySceneContext* Context)
{
	if (!IsValid(Context) || Context->AccountId.IsEmpty())
	{
		return;
	}

	const FYcProfileKey ProfileKey = FYcProfileKey(Context->AccountId, ResolveProfileId(Context->ProfileId));
	ContextByProfileKey.Add(ProfileKey, Context);

	if (UYcProfileSaveSubsystem* SaveSubsystem = GetProfileSaveSubsystem())
	{
		SaveSubsystem->RegisterProfileContext(ProfileKey, Context);
	}
}

void UYcMetaInventorySubsystem::UnregisterContextProfileKey(UYcInventorySceneContext* Context)
{
	if (!IsValid(Context) || Context->AccountId.IsEmpty())
	{
		return;
	}

	const FYcProfileKey ProfileKey = FYcProfileKey(Context->AccountId, ResolveProfileId(Context->ProfileId));
	if (const TObjectPtr<UYcInventorySceneContext>* Existing = ContextByProfileKey.Find(ProfileKey))
	{
		if (Existing->Get() == Context)
		{
			ContextByProfileKey.Remove(ProfileKey);
		}
	}

	if (UYcProfileSaveSubsystem* SaveSubsystem = GetProfileSaveSubsystem())
	{
		SaveSubsystem->UnregisterProfileContext(ProfileKey, Context);
	}
}

bool UYcMetaInventorySubsystem::LoadOrInitializeProfile(UYcInventorySceneContext* Context)
{
	if (!ValidateOutOfMatchContext(Context, TEXT("LoadOrInitializeProfile")))
	{
		return false;
	}

	RegisterContextProfileKey(Context);

	UYcProfileSaveSubsystem* SaveSubsystem = GetProfileSaveSubsystem();
	if (!SaveSubsystem)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("LoadOrInitializeProfile: UYcProfileSaveSubsystem not found."));
		return false;
	}

	const FYcProfileKey ProfileKey = FYcProfileKey(Context->AccountId, ResolveProfileId(Context->ProfileId));
	FString Reason;
	if (SaveSubsystem->LoadProfileSync(ProfileKey, Reason))
	{
		DirtyProfiles.Remove(ProfileKey);
		SaveSubsystem->ClearProfileDirty(ProfileKey);
		return true;
	}

	if (Reason != TEXT("Profile not found."))
	{
		const bool bIsLegacyPayloadError =
			Reason.Contains(TEXT("deserialize snapshot bytes failed"), ESearchCase::IgnoreCase) ||
			Reason.Contains(TEXT("serialize snapshot bytes failed"), ESearchCase::IgnoreCase) ||
			Reason.Contains(TEXT("version mismatch"), ESearchCase::IgnoreCase);
		if (!bIsLegacyPayloadError)
		{
			UE_LOG(LogYcInventory, Warning, TEXT("LoadOrInitializeProfile: load failed. profile='%s', reason='%s'"), *ProfileKey.ToDebugString(), *Reason);
			return false;
		}

		UE_LOG(LogYcInventory, Warning, TEXT("LoadOrInitializeProfile: detected legacy/incompatible payload. Reinitializing profile='%s'. reason='%s'"),
			*ProfileKey.ToDebugString(),
			*Reason);
	}

	FYcMetaInventoryRootSnapshot NewSnapshot = YcMetaInventoryVersion::MakeEmptySnapshot(Context->AccountId);
	if (!ApplySnapshotToContext(Context, NewSnapshot))
	{
		return false;
	}

	// 新档初始化后立即落盘，确保后续局内读档可见。
	return SaveProfile(Context);
}

bool UYcMetaInventorySubsystem::SaveProfile(UYcInventorySceneContext* Context)
{
	if (!ValidateOutOfMatchContext(Context, TEXT("SaveProfile")))
	{
		return false;
	}

	RegisterContextProfileKey(Context);
	UYcProfileSaveSubsystem* SaveSubsystem = GetProfileSaveSubsystem();
	if (!SaveSubsystem)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("SaveProfile: UYcProfileSaveSubsystem not found."));
		return false;
	}

	const FYcProfileKey ProfileKey = FYcProfileKey(Context->AccountId, ResolveProfileId(Context->ProfileId));
	FString Reason;
	const bool bSaved = SaveSubsystem->SaveProfileSync(ProfileKey, Reason);
	if (!bSaved)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("SaveProfile: save failed. profile='%s', reason='%s'"), *ProfileKey.ToDebugString(), *Reason);
		return false;
	}

	DirtyProfiles.Remove(ProfileKey);
	SaveSubsystem->ClearProfileDirty(ProfileKey);
	return true;
}

bool UYcMetaInventorySubsystem::SaveDirtyProfiles()
{
	UYcProfileSaveSubsystem* SaveSubsystem = GetProfileSaveSubsystem();
	if (!SaveSubsystem)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("SaveDirtyProfiles: UYcProfileSaveSubsystem not found."));
		return false;
	}

	bool bAllSucceeded = true;
	const TArray<FYcProfileKey> DirtyKeys = DirtyProfiles.Array();
	for (const FYcProfileKey& ProfileKey : DirtyKeys)
	{
		FString Reason;
		if (!SaveSubsystem->SaveProfileSync(ProfileKey, Reason))
		{
			bAllSucceeded = false;
			UE_LOG(LogYcInventory, Warning, TEXT("SaveDirtyProfiles: save failed. profile='%s', reason='%s'"), *ProfileKey.ToDebugString(), *Reason);
		}
		else
		{
			DirtyProfiles.Remove(ProfileKey);
		}
	}

	return bAllSucceeded;
}

bool UYcMetaInventorySubsystem::SetupOutOfMatchContextAndLoad(const FString& AccountId, AActor* ContextOwner, UYcInventoryManagerComponent* PlayerInventory, UYcInventoryManagerComponent* StashInventory)
{
	return SetupOutOfMatchContextAndLoadWithProfile(AccountId, FString(), ContextOwner, PlayerInventory, StashInventory);
}

bool UYcMetaInventorySubsystem::SetupOutOfMatchContextAndLoadWithProfile(const FString& AccountId, const FString& ProfileId, AActor* ContextOwner, UYcInventoryManagerComponent* PlayerInventory, UYcInventoryManagerComponent* StashInventory)
{
	if (AccountId.IsEmpty() || !IsValid(ContextOwner) || !IsValid(PlayerInventory) || !IsValid(StashInventory))
	{
		return false;
	}

	const FYcProfileKey ProfileKey = FYcProfileKey(AccountId, ResolveProfileId(ProfileId));
	UYcInventorySceneContext* Context = nullptr;
	if (TObjectPtr<UYcInventorySceneContext>* Existing = ContextByProfileKey.Find(ProfileKey))
	{
		Context = Existing->Get();
	}
	if (!IsValid(Context))
	{
		Context = NewObject<UYcInventorySceneContext>(this);
	}

	Context->SceneType = EYcInventorySceneType::OutOfMatch;
	Context->AccountId = AccountId;
	Context->ProfileId = ProfileKey.ProfileId;
	Context->ContextOwner = ContextOwner;
	Context->PlayerInventoryRef = PlayerInventory;
	Context->ContainerInventoryRef = StashInventory;
	Context->bRequirePersistenceCommit = true;

	RegisterSceneContext(Context);
	return LoadOrInitializeProfile(Context);
}

bool UYcMetaInventorySubsystem::SaveOutOfMatchContext(const FString& AccountId)
{
	return SaveOutOfMatchContextWithProfile(AccountId, FString());
}

bool UYcMetaInventorySubsystem::SaveOutOfMatchContextWithProfile(const FString& AccountId, const FString& ProfileId)
{
	if (AccountId.IsEmpty())
	{
		return false;
	}

	const FYcProfileKey ProfileKey = FYcProfileKey(AccountId, ResolveProfileId(ProfileId));
	TObjectPtr<UYcInventorySceneContext>* FoundContext = ContextByProfileKey.Find(ProfileKey);
	if (!FoundContext || !IsValid(*FoundContext))
	{
		return false;
	}
	return SaveProfile(*FoundContext);
}

bool UYcMetaInventorySubsystem::LoadPlayerLoadoutToInMatch(const FString& AccountId, AActor* ContextOwner, UYcInventoryManagerComponent* InMatchPlayerInventory)
{
	return LoadPlayerLoadoutToInMatchWithProfile(AccountId, FString(), ContextOwner, InMatchPlayerInventory);
}

bool UYcMetaInventorySubsystem::LoadPlayerLoadoutToInMatchWithProfile(const FString& AccountId, const FString& ProfileId, AActor* ContextOwner, UYcInventoryManagerComponent* InMatchPlayerInventory)
{
	if (!ValidateInMatchLoadoutRequest(AccountId, ProfileId, ContextOwner, InMatchPlayerInventory, true, TEXT("LoadPlayerLoadoutToInMatchWithProfile")))
	{
		return false;
	}

	UYcProfileSaveSubsystem* SaveSubsystem = GetProfileSaveSubsystem();
	if (!SaveSubsystem)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("LoadPlayerLoadoutToInMatchWithProfile: UYcProfileSaveSubsystem not found."));
		return false;
	}

	const FYcProfileKey ProfileKey = FYcProfileKey(AccountId, ResolveProfileId(ProfileId));
	FYcProfileSaveRoot Root;
	FString Reason;
	const EYcSaveBackendResult LoadResult = SaveSubsystem->LoadProfileRootSync(ProfileKey, Root, Reason);
	if (LoadResult != EYcSaveBackendResult::Success)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("LoadPlayerLoadoutToInMatchWithProfile: load failed. profile='%s', reason='%s'"), *ProfileKey.ToDebugString(), *Reason);
		return false;
	}

	const FYcProfileDomainPayload* InventoryPayload = Root.Domains.FindByPredicate([](const FYcProfileDomainPayload& Payload)
	{
		return Payload.DomainKey == UYcInventorySaveDomainProvider::DomainKey;
	});
	if (!InventoryPayload)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("LoadPlayerLoadoutToInMatchWithProfile: inventory domain payload missing. profile='%s'"), *ProfileKey.ToDebugString());
		return false;
	}

	FYcMetaInventoryRootSnapshot Snapshot;
	if (!DeserializeInventorySnapshotFromBytes(InventoryPayload->PayloadBytes, Snapshot))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("LoadPlayerLoadoutToInMatchWithProfile: deserialize snapshot bytes failed."));
		return false;
	}
	if (!YcMetaInventoryVersion::IsSupportedVersion(Snapshot.SnapshotVersion))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("LoadPlayerLoadoutToInMatchWithProfile: unsupported snapshot version=%d"), Snapshot.SnapshotVersion);
		return false;
	}

	return ApplyPlayerSnapshot(ContextOwner, InMatchPlayerInventory, nullptr, Snapshot.Player);
}

bool UYcMetaInventorySubsystem::CommitInMatchPlayerLoadoutToProfile(const FString& AccountId, AActor* ContextOwner, UYcInventoryManagerComponent* InMatchPlayerInventory, const bool bExtractionSucceeded)
{
	return CommitInMatchPlayerLoadoutToProfileWithProfile(AccountId, FString(), ContextOwner, InMatchPlayerInventory, bExtractionSucceeded);
}

bool UYcMetaInventorySubsystem::CommitInMatchPlayerLoadoutToProfileWithProfile(const FString& AccountId, const FString& ProfileId, AActor* ContextOwner, UYcInventoryManagerComponent* InMatchPlayerInventory, const bool bExtractionSucceeded)
{
	if (!ValidateInMatchLoadoutRequest(AccountId, ProfileId, ContextOwner, InMatchPlayerInventory, bExtractionSucceeded, TEXT("CommitInMatchPlayerLoadoutToProfileWithProfile")))
	{
		return false;
	}

	UYcProfileSaveSubsystem* SaveSubsystem = GetProfileSaveSubsystem();
	if (!SaveSubsystem)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("CommitInMatchPlayerLoadoutToProfileWithProfile: UYcProfileSaveSubsystem not found."));
		return false;
	}

	const FYcProfileKey ProfileKey = FYcProfileKey(AccountId, ResolveProfileId(ProfileId));
	FYcProfileSaveRoot Root;
	FString Reason;
	const EYcSaveBackendResult LoadResult = SaveSubsystem->LoadProfileRootSync(ProfileKey, Root, Reason);
	if (LoadResult != EYcSaveBackendResult::Success)
	{
		Root = FYcProfileSaveRoot();
		Root.AccountId = ProfileKey.AccountId;
		Root.ProfileId = ProfileKey.ProfileId;
	}

	FYcMetaInventoryRootSnapshot Snapshot;
	const FYcProfileDomainPayload* InventoryPayload = Root.Domains.FindByPredicate([](const FYcProfileDomainPayload& Payload)
	{
		return Payload.DomainKey == UYcInventorySaveDomainProvider::DomainKey;
	});
	if (InventoryPayload)
	{
		if (!DeserializeInventorySnapshotFromBytes(InventoryPayload->PayloadBytes, Snapshot))
		{
			UE_LOG(LogYcInventory, Warning, TEXT("CommitInMatchPlayerLoadoutToProfileWithProfile: deserialize snapshot bytes failed, reinit profile snapshot."));
			Snapshot = YcMetaInventoryVersion::MakeEmptySnapshot(AccountId);
		}
	}
	else
	{
		Snapshot = YcMetaInventoryVersion::MakeEmptySnapshot(AccountId);
	}

	if (!YcMetaInventoryVersion::IsSupportedVersion(Snapshot.SnapshotVersion))
	{
		Snapshot = YcMetaInventoryVersion::MakeEmptySnapshot(AccountId);
	}

	if (!bExtractionSucceeded)
	{
		// 撤离失败：清空玩家侧持久化负载，仓库(Stash)保持不变。
		Snapshot.Player = FYcMetaPlayerSnapshot();
		YcMetaInventoryVersion::PrepareSnapshotForSave(AccountId, Snapshot);
	}
	else
	{
		FYcMetaPlayerSnapshot PlayerSnapshot;
		if (!BuildPlayerSnapshot(ContextOwner, InMatchPlayerInventory, nullptr, PlayerSnapshot))
		{
			return false;
		}
		Snapshot.Player = MoveTemp(PlayerSnapshot);
		YcMetaInventoryVersion::PrepareSnapshotForSave(AccountId, Snapshot);
	}

	TArray<uint8> SerializedSnapshot;
	if (!SerializeInventorySnapshotToBytes(Snapshot, SerializedSnapshot))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("CommitInMatchPlayerLoadoutToProfileWithProfile: serialize snapshot bytes failed."));
		return false;
	}

	const int32 ExistingIndex = Root.Domains.IndexOfByPredicate([](const FYcProfileDomainPayload& Payload)
	{
		return Payload.DomainKey == UYcInventorySaveDomainProvider::DomainKey;
	});
	FYcProfileDomainPayload DomainPayload;
	DomainPayload.DomainKey = UYcInventorySaveDomainProvider::DomainKey;
	DomainPayload.Version = 1;
	DomainPayload.PayloadBytes = MoveTemp(SerializedSnapshot);
	if (ExistingIndex != INDEX_NONE)
	{
		Root.Domains[ExistingIndex] = MoveTemp(DomainPayload);
	}
	else
	{
		Root.Domains.Add(MoveTemp(DomainPayload));
	}

	Root.AccountId = ProfileKey.AccountId;
	Root.ProfileId = ProfileKey.ProfileId;
	Root.SnapshotVersion = FMath::Max(Root.SnapshotVersion, 1);
	Root.LastSavedUnixTime = FDateTime::UtcNow().ToUnixTimestamp();

	if (!SaveSubsystem->SaveProfileRootSync(ProfileKey, Root, Reason))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("CommitInMatchPlayerLoadoutToProfileWithProfile: save failed. profile='%s', reason='%s'"), *ProfileKey.ToDebugString(), *Reason);
		return false;
	}
	return true;
}

bool UYcMetaInventorySubsystem::IsProfileDirty(const FString& AccountId) const
{
	for (const FYcProfileKey& ProfileKey : DirtyProfiles)
	{
		if (ProfileKey.AccountId == AccountId)
		{
			return true;
		}
	}
	return false;
}

void UYcMetaInventorySubsystem::MarkProfileDirty(const FString& AccountId)
{
	if (!AccountId.IsEmpty())
	{
		const FYcProfileKey ProfileKey = FYcProfileKey(AccountId, ResolveProfileId(FString()));
		DirtyProfiles.Add(ProfileKey);
		if (UYcProfileSaveSubsystem* SaveSubsystem = GetProfileSaveSubsystem())
		{
			SaveSubsystem->MarkProfileDirty(ProfileKey);
		}
	}
}

void UYcMetaInventorySubsystem::ClearProfileDirty(const FString& AccountId)
{
	if (!AccountId.IsEmpty())
	{
		TArray<FYcProfileKey> ToClear;
		for (const FYcProfileKey& ProfileKey : DirtyProfiles)
		{
			if (ProfileKey.AccountId == AccountId)
			{
				ToClear.Add(ProfileKey);
			}
		}
		for (const FYcProfileKey& ProfileKey : ToClear)
		{
			DirtyProfiles.Remove(ProfileKey);
		}
		if (UYcProfileSaveSubsystem* SaveSubsystem = GetProfileSaveSubsystem())
		{
			for (const FYcProfileKey& ProfileKey : ToClear)
			{
				SaveSubsystem->ClearProfileDirty(ProfileKey);
			}
		}
	}
}

bool UYcMetaInventorySubsystem::BuildSnapshotFromContext(UYcInventorySceneContext* Context, FYcMetaInventoryRootSnapshot& OutSnapshot) const
{
	if (!ValidateOutOfMatchContext(Context, TEXT("BuildSnapshotFromContext")))
	{
		return false;
	}

	OutSnapshot = YcMetaInventoryVersion::MakeEmptySnapshot(Context->AccountId);

	if (!BuildPlayerSnapshot(Context->ContextOwner, Context->PlayerInventoryRef, Context->ContainerInventoryRef, OutSnapshot.Player))
	{
		return false;
	}
	if (!BuildInventoryRecords(Context->ContainerInventoryRef, OutSnapshot.Stash.InventoryItems, OutSnapshot.Stash.InventoryExtensions))
	{
		return false;
	}

	return true;
}

bool UYcMetaInventorySubsystem::ApplySnapshotToContext(UYcInventorySceneContext* Context, const FYcMetaInventoryRootSnapshot& Snapshot)
{
	if (!ValidateOutOfMatchContext(Context, TEXT("ApplySnapshotToContext")))
	{
		return false;
	}

	TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>> StashItemMap;
	if (!RestoreInventoryRecords(Context->ContainerInventoryRef, Snapshot.Stash.InventoryItems, Snapshot.Stash.InventoryExtensions, StashItemMap))
	{
		return false;
	}

	if (!ApplyPlayerSnapshot(Context->ContextOwner, Context->PlayerInventoryRef, &StashItemMap, Snapshot.Player))
	{
		return false;
	}

	return true;
}

bool UYcMetaInventorySubsystem::BuildPlayerSnapshot(const AActor* ContextOwner, UYcInventoryManagerComponent* PlayerInventory, UYcInventoryManagerComponent* StashInventory, FYcMetaPlayerSnapshot& OutPlayerSnapshot) const
{
	if (!IsValid(PlayerInventory))
	{
		return false;
	}

	OutPlayerSnapshot = FYcMetaPlayerSnapshot();
	if (!BuildInventoryRecords(PlayerInventory, OutPlayerSnapshot.InventoryItems, OutPlayerSnapshot.InventoryExtensions))
	{
		return false;
	}

	BuildEquipmentRecords(ContextOwner, PlayerInventory, StashInventory, OutPlayerSnapshot.EquipmentSlots);
	BuildQuickBarRecords(ContextOwner, PlayerInventory, StashInventory, OutPlayerSnapshot.QuickBarSlots);

	// 兜底：装备槽/快捷栏中可能存在不在PlayerInventory中的托管物品。
	TSet<FYcItemInstanceId> PlayerItemIds;
	for (const FYcMetaInventoryItemRecord& Record : OutPlayerSnapshot.InventoryItems)
	{
		if (Record.ItemInstId.IsValid())
		{
			PlayerItemIds.Add(Record.ItemInstId);
		}
	}

	auto MakeUniquePlayerItemId = [&](const FYcItemInstanceId& BaseId) -> FYcItemInstanceId
	{
		(void)BaseId;
		for (int32 Suffix = 0; Suffix < 1000; ++Suffix)
		{
			const FYcItemInstanceId Candidate = FYcItemInstanceId::NewId();
			if (!PlayerItemIds.Contains(Candidate))
			{
				return Candidate;
			}
		}
		return FYcItemInstanceId();
	};

	auto TryAddDetachedPlayerOwnedItem = [&](UYcInventoryItemInstance* ItemInstance, const TFunction<void(const FYcItemInstanceId&, const FYcItemInstanceId&)>& OnRemapSlotRef)
	{
		if (!IsValid(ItemInstance))
		{
			return;
		}

		const FYcItemInstanceId RawItemInstId = ItemInstance->GetItemInstId();
		if (!RawItemInstId.IsValid())
		{
			return;
		}

		UYcInventoryManagerComponent* ItemOwnerInventory = UYcInventoryManagerComponent::FindInventoryManagerByItem(ItemInstance);
		if (ItemOwnerInventory && ItemOwnerInventory != PlayerInventory)
		{
			return;
		}

		FYcItemInstanceId SavedItemInstId = RawItemInstId;
		if (PlayerItemIds.Contains(SavedItemInstId))
		{
			SavedItemInstId = MakeUniquePlayerItemId(RawItemInstId);
			OnRemapSlotRef(RawItemInstId, SavedItemInstId);
		}

		FYcMetaInventoryItemRecord NewRecord;
		NewRecord.ItemInstId = SavedItemInstId;
		NewRecord.ItemRegistryId = ItemInstance->GetItemRegistryId();
		NewRecord.StackCount = 1;
		const TArray<FYcMetaItemExtensionPayload>* UnknownPayloads = UnknownItemExtensionPayloads.Find(RawItemInstId);
		YcMetaInventoryItemRecordCodec::ExportFromItem(*ItemInstance, UnknownPayloads, NewRecord);
		OutPlayerSnapshot.InventoryItems.Add(NewRecord);
		PlayerItemIds.Add(SavedItemInstId);
	};

	if (UActorComponent* EquipmentComp = FindComponentAcrossOwnerChainByInterface(ContextOwner, UYcMetaInventoryEquipmentBridge::StaticClass()))
	{
		TArray<FGameplayTag> OccupiedSlots;
		IYcMetaInventoryEquipmentBridge::Execute_GetMetaOccupiedSlots(EquipmentComp, OccupiedSlots);
		for (const FGameplayTag& SlotTag : OccupiedSlots)
		{
			UYcInventoryItemInstance* SlotItem = IYcMetaInventoryEquipmentBridge::Execute_GetMetaItemInSlot(EquipmentComp, SlotTag);
			const FGameplayTag LocalSlotTag = SlotTag;
			TryAddDetachedPlayerOwnedItem(SlotItem,
				[&](const FYcItemInstanceId& OldId, const FYcItemInstanceId& NewId)
				{
					for (FYcMetaEquipmentSlotRecord& SlotRecord : OutPlayerSnapshot.EquipmentSlots)
					{
						if (SlotRecord.SlotTag == LocalSlotTag && SlotRecord.ItemInstId == OldId)
						{
							SlotRecord.ItemInstId = NewId;
						}
					}
				});
		}
	}

	if (UActorComponent* QuickBarComp = FindComponentAcrossOwnerChainByInterface(ContextOwner, UYcMetaInventoryQuickBarBridge::StaticClass()))
	{
		TArray<UYcInventoryItemInstance*> Slots;
		IYcMetaInventoryQuickBarBridge::Execute_GetMetaQuickBarSlots(QuickBarComp, Slots);
		for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
		{
			UYcInventoryItemInstance* SlotItem = Slots[SlotIndex];
			const int32 LocalSlotIndex = SlotIndex;
			TryAddDetachedPlayerOwnedItem(SlotItem,
				[&](const FYcItemInstanceId& OldId, const FYcItemInstanceId& NewId)
				{
					for (FYcMetaQuickBarSlotRecord& SlotRecord : OutPlayerSnapshot.QuickBarSlots)
					{
						if (SlotRecord.SlotIndex == LocalSlotIndex && SlotRecord.ItemInstId == OldId)
						{
							SlotRecord.ItemInstId = NewId;
						}
					}
				});
		}
	}

	OutPlayerSnapshot.InventoryItems.Sort([](const FYcMetaInventoryItemRecord& A, const FYcMetaInventoryItemRecord& B)
	{
		return A.ItemInstId.ToString() < B.ItemInstId.ToString();
	});

	return true;
}

bool UYcMetaInventorySubsystem::ApplyPlayerSnapshot(const AActor* ContextOwner, UYcInventoryManagerComponent* PlayerInventory, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>* StashItemMap, const FYcMetaPlayerSnapshot& PlayerSnapshot)
{
	if (!IsValid(PlayerInventory))
	{
		return false;
	}

	ClearEquipment(ContextOwner);
	ClearQuickBar(ContextOwner);

	TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>> PlayerItemMap;
	// 玩家库存恢复时，先只恢复物品本体；网格扩展落位需要等装备恢复后（装备可能提供额外区域）。
	if (!RestoreInventoryRecords(PlayerInventory, PlayerSnapshot.InventoryItems, TArray<FYcMetaInventoryExtensionPayload>(), PlayerItemMap))
	{
		return false;
	}

	const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>> EmptyStashMap;
	const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& EffectiveStashItemMap = StashItemMap ? *StashItemMap : EmptyStashMap;
	if (!RestoreEquipment(ContextOwner, PlayerSnapshot.EquipmentSlots, PlayerItemMap, EffectiveStashItemMap))
	{
		return false;
	}
	if (!RestoreQuickBar(ContextOwner, PlayerSnapshot.QuickBarSlots, PlayerItemMap, EffectiveStashItemMap))
	{
		return false;
	}
	if (!ApplyInventoryExtensions(PlayerInventory, PlayerItemMap, PlayerSnapshot.InventoryExtensions))
	{
		return false;
	}

	// 强制广播一次当前槽位状态，确保跨场景持久化UI能立即刷新图标。
	if (UActorComponent* EquipmentComp = FindComponentAcrossOwnerChainByInterface(ContextOwner, UYcMetaInventoryEquipmentBridge::StaticClass()))
	{
		IYcMetaInventoryEquipmentBridge::Execute_MetaNotifySlotsUpdated(EquipmentComp);
	}
	if (UActorComponent* QuickBarComp = FindComponentAcrossOwnerChainByInterface(ContextOwner, UYcMetaInventoryQuickBarBridge::StaticClass()))
	{
		IYcMetaInventoryQuickBarBridge::Execute_MetaNotifyQuickBarSlotsUpdated(QuickBarComp);
	}

	return true;
}

void UYcMetaInventorySubsystem::OnOperationStateChanged(FGameplayTag ActualTag, const FYcInventoryOperationStateMessage& Message)
{
	(void)ActualTag;

	if (Message.Event != EYcInventoryOperationEvent::Acked)
	{
		return;
	}

	for (int32 Index = SceneContexts.Num() - 1; Index >= 0; --Index)
	{
		UYcInventorySceneContext* Context = SceneContexts[Index];
		if (!IsValid(Context))
		{
			SceneContexts.RemoveAtSwap(Index);
			continue;
		}

		if (!Context->IsValidForOutOfMatchPersistence())
		{
			continue;
		}

		const bool bTouchesPlayer = (Message.Operation.SourceInventory == Context->PlayerInventoryRef || Message.Operation.TargetInventory == Context->PlayerInventoryRef);
		const bool bTouchesContainer = (Message.Operation.SourceInventory == Context->ContainerInventoryRef || Message.Operation.TargetInventory == Context->ContainerInventoryRef);
		if (bTouchesPlayer || bTouchesContainer)
		{
			const FYcProfileKey ProfileKey = FYcProfileKey(Context->AccountId, ResolveProfileId(Context->ProfileId));
			DirtyProfiles.Add(ProfileKey);
			if (UYcProfileSaveSubsystem* SaveSubsystem = GetProfileSaveSubsystem())
			{
				SaveSubsystem->MarkProfileDirty(ProfileKey);
			}
		}
	}
}

void UYcMetaInventorySubsystem::GatherPersistenceExtensionProviders(TArray<const UYcInventoryPersistenceExtensionProvider*>& OutProviders) const
{
	OutProviders.Empty();

	// 从全局注册表取出所有 Provider 类型，并转为 CDO 供只读调用。
	const TArray<TSubclassOf<UYcInventoryPersistenceExtensionProvider>> ProviderClasses = YcInventoryPersistenceExtensionRegistry::GetRegisteredProviderClasses();
	for (const TSubclassOf<UYcInventoryPersistenceExtensionProvider> ProviderClass : ProviderClasses)
	{
		if (!ProviderClass)
		{
			continue;
		}

		if (const UYcInventoryPersistenceExtensionProvider* ProviderCDO = Cast<UYcInventoryPersistenceExtensionProvider>(ProviderClass->GetDefaultObject()))
		{
			OutProviders.Add(ProviderCDO);
		}
	}
}

bool UYcMetaInventorySubsystem::BuildInventoryRecords(UYcInventoryManagerComponent* Inventory, TArray<FYcMetaInventoryItemRecord>& OutItems, TArray<FYcMetaInventoryExtensionPayload>& OutExtensions) const
{
	if (!IsValid(Inventory))
	{
		return false;
	}

	OutItems.Empty();
	OutExtensions.Empty();
	const TArray<UYcInventoryItemInstance*> Items = Inventory->GetAllItemInstance();
	for (UYcInventoryItemInstance* Item : Items)
	{
		if (!IsValid(Item))
		{
			continue;
		}

		FYcMetaInventoryItemRecord Record;
		Record.ItemInstId = Item->GetItemInstId();
		Record.ItemRegistryId = Item->GetItemRegistryId();
		Record.StackCount = Inventory->GetStackCountByItemInstance(Item);

		const TArray<FYcMetaItemExtensionPayload>* UnknownPayloads = UnknownItemExtensionPayloads.Find(Record.ItemInstId);
		YcMetaInventoryItemRecordCodec::ExportFromItem(*Item, UnknownPayloads, Record);
		OutItems.Add(Record);
	}

	OutItems.Sort([](const FYcMetaInventoryItemRecord& A, const FYcMetaInventoryItemRecord& B)
	{
		return A.ItemInstId.ToString() < B.ItemInstId.ToString();
	});

	return BuildInventoryExtensions(Inventory, OutExtensions);
}

bool UYcMetaInventorySubsystem::BuildInventoryExtensions(const UYcInventoryManagerComponent* Inventory, TArray<FYcMetaInventoryExtensionPayload>& OutExtensions) const
{
	OutExtensions.Empty();
	if (!IsValid(Inventory))
	{
		return false;
	}

	TArray<const UYcInventoryPersistenceExtensionProvider*> Providers;
	GatherPersistenceExtensionProviders(Providers);
	for (const UYcInventoryPersistenceExtensionProvider* Provider : Providers)
	{
		// 仅让能够处理该库存类型的 Provider 参与构建。
		if (!Provider || !Provider->CanHandleInventory(Inventory))
		{
			continue;
		}

		FInstancedStruct Payload;
		FString Reason;
		if (!Provider->BuildInventoryExtensionPayload(Inventory, Payload, Reason))
		{
			UE_LOG(LogYcInventory, Warning, TEXT("BuildInventoryExtensions: provider '%s' failed for inventory '%s', reason='%s'."),
				*Provider->GetExtensionKey().ToString(),
				*GetNameSafe(Inventory),
				*Reason);
			return false;
		}

		FYcMetaInventoryExtensionPayload Extension;
		Extension.ExtensionKey = Provider->GetExtensionKey();
		Extension.Version = Provider->GetExtensionVersion();
		Extension.Payload = MoveTemp(Payload);
		OutExtensions.Add(MoveTemp(Extension));
	}

	// 固定顺序，避免快照输出顺序抖动。
	OutExtensions.Sort([](const FYcMetaInventoryExtensionPayload& A, const FYcMetaInventoryExtensionPayload& B)
	{
		return A.ExtensionKey.LexicalLess(B.ExtensionKey);
	});

	return true;
}

bool UYcMetaInventorySubsystem::RestoreInventoryRecords(UYcInventoryManagerComponent* Inventory, const TArray<FYcMetaInventoryItemRecord>& InItems, const TArray<FYcMetaInventoryExtensionPayload>& InExtensions, TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& OutItemMap)
{
	if (!IsValid(Inventory))
	{
		return false;
	}

	OutItemMap.Empty();

	// 先清空已有物品，再按快照重建实例。
	const TArray<UYcInventoryItemInstance*> Existing = Inventory->GetAllItemInstance();
	for (UYcInventoryItemInstance* Item : Existing)
	{
		if (IsValid(Item))
		{
			UnknownItemExtensionPayloads.Remove(Item->GetItemInstId());
			Inventory->RemoveItemInstance(Item);
		}
	}

	for (const FYcMetaInventoryItemRecord& Record : InItems)
	{
		if (!Record.ItemRegistryId.IsValid() || Record.StackCount <= 0)
		{
			continue;
		}

		UYcInventoryItemInstance* CreatedItem = Inventory->AddItemWithInstanceId(Record.ItemRegistryId, Record.ItemInstId, Record.StackCount);
		if (!CreatedItem)
		{
			UE_LOG(LogYcInventory, Warning, TEXT("RestoreInventoryRecords: failed add %s (%s)."), *Record.ItemInstId.ToString(), *Record.ItemRegistryId.ToString());
			continue;
		}

		TArray<FYcMetaItemExtensionPayload> UnknownPayloads;
		YcMetaInventoryItemRecordCodec::ImportToItem(*CreatedItem, Record, UnknownPayloads);
		if (UnknownPayloads.IsEmpty())
		{
			UnknownItemExtensionPayloads.Remove(Record.ItemInstId);
		}
		else
		{
			UnknownItemExtensionPayloads.Add(Record.ItemInstId, MoveTemp(UnknownPayloads));
		}

		OutItemMap.Add(Record.ItemInstId, CreatedItem);
	}

	return ApplyInventoryExtensions(Inventory, OutItemMap, InExtensions);
}

bool UYcMetaInventorySubsystem::ApplyInventoryExtensions(UYcInventoryManagerComponent* Inventory, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& ItemMap, const TArray<FYcMetaInventoryExtensionPayload>& InExtensions) const
{
	if (!IsValid(Inventory))
	{
		return false;
	}

	if (InExtensions.IsEmpty())
	{
		return true;
	}

	TArray<const UYcInventoryPersistenceExtensionProvider*> Providers;
	GatherPersistenceExtensionProviders(Providers);

	TMap<FName, const UYcInventoryPersistenceExtensionProvider*> ProviderByKey;
	for (const UYcInventoryPersistenceExtensionProvider* Provider : Providers)
	{
		// 建立扩展键到 Provider 的路由表。
		if (!Provider || !Provider->CanHandleInventory(Inventory))
		{
			continue;
		}

		ProviderByKey.Add(Provider->GetExtensionKey(), Provider);
	}

	for (const FYcMetaInventoryExtensionPayload& Extension : InExtensions)
	{
		// 按扩展键把载荷分发给对应 Provider。
		if (const UYcInventoryPersistenceExtensionProvider* const* ProviderPtr = ProviderByKey.Find(Extension.ExtensionKey))
		{
			FString Reason;
			if (!(*ProviderPtr)->ApplyInventoryExtensionPayload(Inventory, Extension.Payload, ItemMap, Reason))
			{
				UE_LOG(LogYcInventory, Warning, TEXT("ApplyInventoryExtensions: provider '%s' failed for inventory '%s', reason='%s'."),
					*Extension.ExtensionKey.ToString(),
					*GetNameSafe(Inventory),
					*Reason);
				return false;
			}
		}
		else
		{
			UE_LOG(LogYcInventory, Verbose, TEXT("ApplyInventoryExtensions: missing provider for extension '%s' on inventory '%s'."),
				*Extension.ExtensionKey.ToString(),
				*GetNameSafe(Inventory));
		}
	}

	return true;
}

void UYcMetaInventorySubsystem::BuildEquipmentRecords(const AActor* ContextOwner, UYcInventoryManagerComponent* PlayerInventory, UYcInventoryManagerComponent* StashInventory, TArray<FYcMetaEquipmentSlotRecord>& OutSlots) const
{
	OutSlots.Empty();
	UActorComponent* EquipmentComp = FindComponentAcrossOwnerChainByInterface(ContextOwner, UYcMetaInventoryEquipmentBridge::StaticClass());
	if (!EquipmentComp)
	{
		return;
	}

	TArray<FGameplayTag> OccupiedSlots;
	IYcMetaInventoryEquipmentBridge::Execute_GetMetaOccupiedSlots(EquipmentComp, OccupiedSlots);
	for (const FGameplayTag& SlotTag : OccupiedSlots)
	{
		UYcInventoryItemInstance* SlotItem = IYcMetaInventoryEquipmentBridge::Execute_GetMetaItemInSlot(EquipmentComp, SlotTag);
		if (!IsValid(SlotItem))
		{
			continue;
		}
		FYcMetaEquipmentSlotRecord SlotRecord;
		SlotRecord.SlotTag = SlotTag;
		SlotRecord.ItemInstId = SlotItem->GetItemInstId();
		const UYcInventoryManagerComponent* ItemOwnerInventory = UYcInventoryManagerComponent::FindInventoryManagerByItem(SlotItem);
		if (ItemOwnerInventory == StashInventory)
		{
			SlotRecord.SourceScope = EYcMetaItemSourceScope::StashInventory;
		}
		else if (ItemOwnerInventory == nullptr || ItemOwnerInventory == PlayerInventory)
		{
			SlotRecord.SourceScope = EYcMetaItemSourceScope::PlayerInventory;
		}
		else
		{
			SlotRecord.SourceScope = EYcMetaItemSourceScope::Unknown;
		}
		OutSlots.Add(SlotRecord);
	}
}

void UYcMetaInventorySubsystem::BuildQuickBarRecords(const AActor* ContextOwner, UYcInventoryManagerComponent* PlayerInventory, UYcInventoryManagerComponent* StashInventory, TArray<FYcMetaQuickBarSlotRecord>& OutSlots) const
{
	OutSlots.Empty();
	UActorComponent* QuickBarComp = FindComponentAcrossOwnerChainByInterface(ContextOwner, UYcMetaInventoryQuickBarBridge::StaticClass());
	if (!QuickBarComp)
	{
		return;
	}

	TArray<UYcInventoryItemInstance*> Slots;
	IYcMetaInventoryQuickBarBridge::Execute_GetMetaQuickBarSlots(QuickBarComp, Slots);
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		UYcInventoryItemInstance* SlotItem = Slots[SlotIndex];
		if (!IsValid(SlotItem))
		{
			continue;
		}
		FYcMetaQuickBarSlotRecord SlotRecord;
		SlotRecord.SlotIndex = SlotIndex;
		SlotRecord.ItemInstId = SlotItem->GetItemInstId();
		const UYcInventoryManagerComponent* ItemOwnerInventory = UYcInventoryManagerComponent::FindInventoryManagerByItem(SlotItem);
		if (ItemOwnerInventory == StashInventory)
		{
			SlotRecord.SourceScope = EYcMetaItemSourceScope::StashInventory;
		}
		else if (ItemOwnerInventory == nullptr || ItemOwnerInventory == PlayerInventory)
		{
			SlotRecord.SourceScope = EYcMetaItemSourceScope::PlayerInventory;
		}
		else
		{
			SlotRecord.SourceScope = EYcMetaItemSourceScope::Unknown;
		}
		OutSlots.Add(SlotRecord);
	}
}

void UYcMetaInventorySubsystem::ClearEquipment(const AActor* ContextOwner) const
{
	UActorComponent* EquipmentComp = FindComponentAcrossOwnerChainByInterface(ContextOwner, UYcMetaInventoryEquipmentBridge::StaticClass());
	if (!EquipmentComp)
	{
		return;
	}

	TArray<FGameplayTag> OccupiedSlots;
	IYcMetaInventoryEquipmentBridge::Execute_GetMetaOccupiedSlots(EquipmentComp, OccupiedSlots);
	for (const FGameplayTag& SlotTag : OccupiedSlots)
	{
		IYcMetaInventoryEquipmentBridge::Execute_MetaUnequipSlot(EquipmentComp, SlotTag);
	}
}

void UYcMetaInventorySubsystem::ClearQuickBar(const AActor* ContextOwner) const
{
	UActorComponent* QuickBarComp = FindComponentAcrossOwnerChainByInterface(ContextOwner, UYcMetaInventoryQuickBarBridge::StaticClass());
	if (!QuickBarComp)
	{
		return;
	}

	TArray<UYcInventoryItemInstance*> Slots;
	IYcMetaInventoryQuickBarBridge::Execute_GetMetaQuickBarSlots(QuickBarComp, Slots);
	for (int32 SlotIndex = 0; SlotIndex < Slots.Num(); ++SlotIndex)
	{
		if (Slots[SlotIndex] == nullptr)
		{
			continue;
		}
		IYcMetaInventoryQuickBarBridge::Execute_MetaRemoveQuickBarSlot(QuickBarComp, SlotIndex);
	}
}

bool UYcMetaInventorySubsystem::RestoreEquipment(const AActor* ContextOwner, const TArray<FYcMetaEquipmentSlotRecord>& InSlots, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& PlayerItemMap, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& StashItemMap) const
{
	UActorComponent* EquipmentComp = FindComponentAcrossOwnerChainByInterface(ContextOwner, UYcMetaInventoryEquipmentBridge::StaticClass());
	if (!EquipmentComp)
	{
		return true;
	}

	bool bAllSucceeded = true;

	for (const FYcMetaEquipmentSlotRecord& Slot : InSlots)
	{
		const TObjectPtr<UYcInventoryItemInstance>* FoundItem = nullptr;
		switch (Slot.SourceScope)
		{
		case EYcMetaItemSourceScope::PlayerInventory:
			FoundItem = PlayerItemMap.Find(Slot.ItemInstId);
			break;
		case EYcMetaItemSourceScope::StashInventory:
			FoundItem = StashItemMap.Find(Slot.ItemInstId);
			break;
		case EYcMetaItemSourceScope::Unknown:
		default:
			FoundItem = PlayerItemMap.Find(Slot.ItemInstId);
			if (!FoundItem)
			{
				FoundItem = StashItemMap.Find(Slot.ItemInstId);
			}
			break;
		}
		if (!FoundItem || !IsValid(*FoundItem))
		{
			bAllSucceeded = false;
			UE_LOG(LogYcInventory, Warning, TEXT("RestoreEquipment: item not found for slot '%s', sourceScope=%d, itemId=%s"),
				*Slot.SlotTag.ToString(), static_cast<int32>(Slot.SourceScope), *Slot.ItemInstId.ToString());
			continue;
		}

		if (!IYcMetaInventoryEquipmentBridge::Execute_MetaEquipItem(EquipmentComp, *FoundItem))
		{
			bAllSucceeded = false;
			UE_LOG(LogYcInventory, Warning, TEXT("RestoreEquipment: bridge equip failed for slot '%s', itemId=%s"),
				*Slot.SlotTag.ToString(), *Slot.ItemInstId.ToString());
		}
	}

	return bAllSucceeded;
}

bool UYcMetaInventorySubsystem::RestoreQuickBar(const AActor* ContextOwner, const TArray<FYcMetaQuickBarSlotRecord>& InSlots, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& PlayerItemMap, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& StashItemMap) const
{
	UActorComponent* QuickBarComp = FindComponentAcrossOwnerChainByInterface(ContextOwner, UYcMetaInventoryQuickBarBridge::StaticClass());
	if (!QuickBarComp)
	{
		return true;
	}

	bool bAllSucceeded = true;

	for (const FYcMetaQuickBarSlotRecord& Slot : InSlots)
	{
		const TObjectPtr<UYcInventoryItemInstance>* FoundItem = nullptr;
		switch (Slot.SourceScope)
		{
		case EYcMetaItemSourceScope::PlayerInventory:
			FoundItem = PlayerItemMap.Find(Slot.ItemInstId);
			break;
		case EYcMetaItemSourceScope::StashInventory:
			FoundItem = StashItemMap.Find(Slot.ItemInstId);
			break;
		case EYcMetaItemSourceScope::Unknown:
		default:
			FoundItem = PlayerItemMap.Find(Slot.ItemInstId);
			if (!FoundItem)
			{
				FoundItem = StashItemMap.Find(Slot.ItemInstId);
			}
			break;
		}
		if (!FoundItem || !IsValid(*FoundItem))
		{
			bAllSucceeded = false;
			UE_LOG(LogYcInventory, Warning, TEXT("RestoreQuickBar: item not found for slotIndex=%d, sourceScope=%d, itemId=%s"),
				Slot.SlotIndex, static_cast<int32>(Slot.SourceScope), *Slot.ItemInstId.ToString());
			continue;
		}

		if (!IYcMetaInventoryQuickBarBridge::Execute_MetaAddQuickBarSlot(QuickBarComp, Slot.SlotIndex, *FoundItem))
		{
			bAllSucceeded = false;
			UE_LOG(LogYcInventory, Warning, TEXT("RestoreQuickBar: bridge add failed for slotIndex=%d, itemId=%s"),
				Slot.SlotIndex, *Slot.ItemInstId.ToString());
		}
	}

	return bAllSucceeded;
}

UActorComponent* UYcMetaInventorySubsystem::FindComponentAcrossOwnerChainByInterface(const AActor* Owner, const UClass* InterfaceClass)
{
	if (!Owner || !InterfaceClass)
	{
		return nullptr;
	}

	auto FindByInterface = [InterfaceClass](const AActor* Actor) -> UActorComponent*
	{
		if (!Actor)
		{
			return nullptr;
		}

		TInlineComponentArray<UActorComponent*> Components;
		Actor->GetComponents(Components);
		for (UActorComponent* Component : Components)
		{
			if (Component && Component->GetClass() && Component->GetClass()->ImplementsInterface(InterfaceClass))
			{
				return Component;
			}
		}
		return nullptr;
	};

	if (UActorComponent* Found = FindByInterface(Owner))
	{
		return Found;
	}

	if (const APawn* Pawn = Cast<APawn>(Owner))
	{
		if (UActorComponent* Found = FindByInterface(Pawn->GetController()))
		{
			return Found;
		}
		if (UActorComponent* Found = FindByInterface(Pawn->GetPlayerState()))
		{
			return Found;
		}
	}

	if (const AController* Controller = Cast<AController>(Owner))
	{
		if (UActorComponent* Found = FindByInterface(Controller->GetPawn()))
		{
			return Found;
		}
		if (UActorComponent* Found = FindByInterface(Controller->PlayerState))
		{
			return Found;
		}
	}

	if (const APlayerState* PlayerState = Cast<APlayerState>(Owner))
	{
		if (UActorComponent* Found = FindByInterface(PlayerState->GetPawn()))
		{
			return Found;
		}
		if (UActorComponent* Found = FindByInterface(Cast<AController>(PlayerState->GetOwner())))
		{
			return Found;
		}
	}

	return nullptr;
}
