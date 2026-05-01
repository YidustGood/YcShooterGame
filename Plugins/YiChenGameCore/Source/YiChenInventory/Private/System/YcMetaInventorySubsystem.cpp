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
#include "Utils/CommonSimpleUtil.h"
#include "YiChenInventory.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcMetaInventorySubsystem)

namespace
{
	static const FName OperationStateChangedTagName(TEXT("Yc.Inventory.Message.Operation.StateChanged"));

	static bool IsPlayerSnapshotEmpty(const FYcMetaPlayerSnapshot& Snapshot)
	{
		return Snapshot.InventoryItems.IsEmpty()
			&& Snapshot.EquipmentSlots.IsEmpty()
			&& Snapshot.QuickBarSlots.IsEmpty();
	}

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

	static bool DoesComponentImplement(const UActorComponent* Component, const UClass* InterfaceClass)
	{
		return IsValid(Component)
			&& IsValid(InterfaceClass)
			&& Component->GetClass()->ImplementsInterface(InterfaceClass);
	}

	static bool InventoryActuallyContainsItem(const UYcInventoryManagerComponent* Inventory, const UYcInventoryItemInstance* ItemInstance)
	{
		return IsValid(Inventory)
			&& IsValid(ItemInstance)
			&& Inventory->GetStackCountByItemInstance(ItemInstance) > 0;
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
		UE_LOG(LogYcInventory, Warning, TEXT("%s: context is not eligible for out-of-match persistence. SceneType=%d, Profile='%s'."),
			Caller, static_cast<int32>(Context->SceneType), *Context->ProfileIdentity.ToDebugString());
		return false;
	}

	return true;
}

bool UYcMetaInventorySubsystem::ValidateRuntimeRequest(const FYcProfileIdentity& ProfileIdentity, const FYcPlayerInventoryRuntime& Runtime, const bool bRequireOutOfMatchRuntime, const TCHAR* Caller) const
{
	if (!ProfileIdentity.IsValid())
	{
		UE_LOG(LogYcInventory, Warning, TEXT("%s: invalid profile identity '%s'."), Caller, *ProfileIdentity.ToDebugString());
		return false;
	}

	if (!Runtime.IsRuntimeValid())
	{
		UE_LOG(LogYcInventory, Warning, TEXT("%s: runtime is incomplete."), Caller);
		return false;
	}

	if (!DoesComponentImplement(Runtime.QuickBarBridge, UYcMetaInventoryQuickBarBridge::StaticClass()))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("%s: quickbar bridge does not implement meta bridge."), Caller);
		return false;
	}

	if (IsValid(Runtime.EquipmentBridge) && !DoesComponentImplement(Runtime.EquipmentBridge, UYcMetaInventoryEquipmentBridge::StaticClass()))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("%s: equipment bridge does not implement meta bridge."), Caller);
		return false;
	}

	if (bRequireOutOfMatchRuntime && !Runtime.SupportsOutOfMatchPersistence())
	{
		UE_LOG(LogYcInventory, Warning, TEXT("%s: runtime does not support out-of-match persistence."), Caller);
		return false;
	}

	return true;
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
	if (!IsValid(Context) || !Context->ProfileIdentity.IsValid())
	{
		return;
	}

	const FYcProfileSaveKey ProfileKey(Context->ProfileIdentity);
	ContextByProfileKey.Add(ProfileKey, Context);

	if (UYcProfileSaveSubsystem* SaveSubsystem = GetProfileSaveSubsystem())
	{
		SaveSubsystem->RegisterProfileContext(Context->ProfileIdentity, Context);
	}
}

void UYcMetaInventorySubsystem::UnregisterContextProfileKey(UYcInventorySceneContext* Context)
{
	if (!IsValid(Context) || !Context->ProfileIdentity.IsValid())
	{
		return;
	}

	const FYcProfileSaveKey ProfileKey(Context->ProfileIdentity);
	if (const TObjectPtr<UYcInventorySceneContext>* Existing = ContextByProfileKey.Find(ProfileKey))
	{
		if (Existing->Get() == Context)
		{
			ContextByProfileKey.Remove(ProfileKey);
		}
	}

	if (UYcProfileSaveSubsystem* SaveSubsystem = GetProfileSaveSubsystem())
	{
		SaveSubsystem->UnregisterProfileContext(Context->ProfileIdentity, Context);
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

	const FYcProfileSaveKey ProfileKey(Context->ProfileIdentity);
	FString Reason;
	if (SaveSubsystem->LoadProfileSync(Context->ProfileIdentity, Reason))
	{
		DirtyProfiles.Remove(ProfileKey);
		SaveSubsystem->ClearProfileDirty(Context->ProfileIdentity);
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

	FYcMetaInventoryRootSnapshot NewSnapshot = YcMetaInventoryVersion::MakeEmptySnapshot(Context->ProfileIdentity);
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

	const FYcProfileSaveKey ProfileKey(Context->ProfileIdentity);
	FString Reason;
	const bool bSaved = SaveSubsystem->SaveProfileSync(Context->ProfileIdentity, Reason);
	if (!bSaved)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("SaveProfile: save failed. profile='%s', reason='%s'"), *ProfileKey.ToDebugString(), *Reason);
		return false;
	}

	DirtyProfiles.Remove(ProfileKey);
	SaveSubsystem->ClearProfileDirty(Context->ProfileIdentity);
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
	const TArray<FYcProfileSaveKey> DirtyKeys = DirtyProfiles.Array();
	for (const FYcProfileSaveKey& ProfileKey : DirtyKeys)
	{
		const TObjectPtr<UYcInventorySceneContext>* ContextPtr = ContextByProfileKey.Find(ProfileKey);
		if (!ContextPtr || !IsValid(*ContextPtr) || !(*ContextPtr)->ProfileIdentity.IsValid())
		{
			bAllSucceeded = false;
			UE_LOG(LogYcInventory, Warning, TEXT("SaveDirtyProfiles: missing valid context for profile='%s'"), *ProfileKey.ToDebugString());
			continue;
		}

		FString Reason;
		if (!SaveSubsystem->SaveProfileSync((*ContextPtr)->ProfileIdentity, Reason))
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

bool UYcMetaInventorySubsystem::SetupOutOfMatchContextAndLoad(const FYcProfileIdentity& ProfileIdentity, const FYcPlayerInventoryRuntime& Runtime)
{
	if (!ValidateRuntimeRequest(ProfileIdentity, Runtime, true, TEXT("SetupOutOfMatchContextAndLoad")))
	{
		return false;
	}

	const FYcProfileSaveKey ProfileKey(ProfileIdentity);
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
	Context->ProfileIdentity = ProfileIdentity;
	Context->Runtime = Runtime;

	RegisterSceneContext(Context);
	return LoadOrInitializeProfile(Context);
}

bool UYcMetaInventorySubsystem::SaveOutOfMatchContext(const FYcProfileIdentity& ProfileIdentity, const FYcPlayerInventoryRuntime& Runtime)
{
	if (!ValidateRuntimeRequest(ProfileIdentity, Runtime, true, TEXT("SaveOutOfMatchContext")))
	{
		return false;
	}

	const FYcProfileSaveKey ProfileKey(ProfileIdentity);
	TObjectPtr<UYcInventorySceneContext>* FoundContext = ContextByProfileKey.Find(ProfileKey);
	if (!FoundContext || !IsValid(*FoundContext))
	{
		return false;
	}
	(*FoundContext)->Runtime = Runtime;
	return SaveProfile(*FoundContext);
}

bool UYcMetaInventorySubsystem::LoadPlayerLoadoutToInMatch(const FYcProfileIdentity& ProfileIdentity, const FYcPlayerInventoryRuntime& Runtime)
{
	if (!ValidateRuntimeRequest(ProfileIdentity, Runtime, false, TEXT("LoadPlayerLoadoutToInMatch")))
	{
		return false;
	}

	UYcProfileSaveSubsystem* SaveSubsystem = GetProfileSaveSubsystem();
	if (!SaveSubsystem)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("LoadPlayerLoadoutToInMatch: UYcProfileSaveSubsystem not found."));
		return false;
	}

	const FYcProfileSaveKey ProfileKey(ProfileIdentity);
	FYcProfileSaveRoot Root;
	FString Reason;
	const EYcSaveBackendResult LoadResult = SaveSubsystem->LoadProfileRootSync(ProfileIdentity, Root, Reason);
	const FString EffectiveReason = Reason.IsEmpty()
		? FString::Printf(TEXT("backend result=%d"), static_cast<int32>(LoadResult))
		: Reason;
	if (LoadResult == EYcSaveBackendResult::NotFound)
	{
		// 首次进入局内且无局外档案时，保留运行时默认初始化负载（例如默认武器），不做覆盖清空。
		UE_LOG(LogYcInventory, Verbose, TEXT("LoadPlayerLoadoutToInMatch: profile not found. keep runtime loadout. profile='%s'"), *ProfileKey.ToDebugString());
		return true;
	}
	if (LoadResult != EYcSaveBackendResult::Success)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("LoadPlayerLoadoutToInMatch: load failed. profile='%s', result=%d, reason='%s'"), *ProfileKey.ToDebugString(), static_cast<int32>(LoadResult), *EffectiveReason);
		return false;
	}

	const FYcProfileDomainPayload* InventoryPayload = Root.Domains.FindByPredicate([](const FYcProfileDomainPayload& Payload)
	{
		return Payload.DomainKey == UYcInventorySaveDomainProvider::DomainKey;
	});
	if (!InventoryPayload)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("LoadPlayerLoadoutToInMatch: inventory domain payload missing. keep runtime loadout. profile='%s'"), *ProfileKey.ToDebugString());
		return true;
	}

	FYcMetaInventoryRootSnapshot Snapshot;
	if (!DeserializeInventorySnapshotFromBytes(InventoryPayload->PayloadBytes, Snapshot))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("LoadPlayerLoadoutToInMatch: deserialize snapshot bytes failed."));
		return false;
	}
	if (!YcMetaInventoryVersion::IsSupportedVersion(Snapshot.SnapshotVersion))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("LoadPlayerLoadoutToInMatch: unsupported snapshot version=%d"), Snapshot.SnapshotVersion);
		return false;
	}

	if (IsPlayerSnapshotEmpty(Snapshot.Player))
	{
		// 历史空快照或撤离失败回写后的空玩家快照：局内保持默认初始化负载，避免清空默认武器。
		UE_LOG(LogYcInventory, Verbose, TEXT("LoadPlayerLoadoutToInMatch: player snapshot is empty. keep runtime loadout. profile='%s'"), *ProfileKey.ToDebugString());
		return true;
	}

	return ApplyPlayerSnapshot(Runtime, nullptr, Snapshot.Player);
}

bool UYcMetaInventorySubsystem::CommitInMatchPlayerLoadoutToProfile(const FYcProfileIdentity& ProfileIdentity, const FYcPlayerInventoryRuntime& Runtime, const bool bExtractionSucceeded)
{
	if (!ValidateRuntimeRequest(ProfileIdentity, Runtime, false, TEXT("CommitInMatchPlayerLoadoutToProfile")))
	{
		return false;
	}

	UYcProfileSaveSubsystem* SaveSubsystem = GetProfileSaveSubsystem();
	if (!SaveSubsystem)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("CommitInMatchPlayerLoadoutToProfile: UYcProfileSaveSubsystem not found."));
		return false;
	}

	const FYcProfileSaveKey ProfileKey(ProfileIdentity);
	FYcProfileSaveRoot Root;
	FString Reason;
	const EYcSaveBackendResult LoadResult = SaveSubsystem->LoadProfileRootSync(ProfileIdentity, Root, Reason);
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
			UE_LOG(LogYcInventory, Warning, TEXT("CommitInMatchPlayerLoadoutToProfile: deserialize snapshot bytes failed, reinit profile snapshot."));
			Snapshot = YcMetaInventoryVersion::MakeEmptySnapshot(ProfileIdentity);
		}
	}
	else
	{
		Snapshot = YcMetaInventoryVersion::MakeEmptySnapshot(ProfileIdentity);
	}

	if (!YcMetaInventoryVersion::IsSupportedVersion(Snapshot.SnapshotVersion))
	{
		Snapshot = YcMetaInventoryVersion::MakeEmptySnapshot(ProfileIdentity);
	}

	if (!bExtractionSucceeded)
	{
		// 撤离失败：清空玩家侧持久化负载，仓库(Stash)保持不变。
		Snapshot.Player = FYcMetaPlayerSnapshot();
		YcMetaInventoryVersion::PrepareSnapshotForSave(ProfileIdentity, Snapshot);
	}
	else
	{
		FYcMetaPlayerSnapshot PlayerSnapshot;
		if (!BuildPlayerSnapshot(Runtime, PlayerSnapshot))
		{
			return false;
		}
		Snapshot.Player = MoveTemp(PlayerSnapshot);
		YcMetaInventoryVersion::PrepareSnapshotForSave(ProfileIdentity, Snapshot);
	}

	TArray<uint8> SerializedSnapshot;
	if (!SerializeInventorySnapshotToBytes(Snapshot, SerializedSnapshot))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("CommitInMatchPlayerLoadoutToProfile: serialize snapshot bytes failed."));
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
	Root.Environment = ProfileKey.Environment;
	Root.SnapshotVersion = FMath::Max(Root.SnapshotVersion, 1);
	Root.LastSavedUnixTime = YcTimeUtils::GetUtcNowUnixTimestampSeconds();

	if (!SaveSubsystem->SaveProfileRootSync(ProfileIdentity, Root, Reason))
	{
		UE_LOG(LogYcInventory, Warning, TEXT("CommitInMatchPlayerLoadoutToProfile: save failed. profile='%s', reason='%s'"), *ProfileKey.ToDebugString(), *Reason);
		return false;
	}
	return true;
}

bool UYcMetaInventorySubsystem::BuildPlayerSnapshotFromRuntime(const FYcPlayerInventoryRuntime& Runtime, FYcMetaPlayerSnapshot& OutPlayerSnapshot) const
{
	return BuildPlayerSnapshot(Runtime, OutPlayerSnapshot);
}

bool UYcMetaInventorySubsystem::ApplyPlayerSnapshotToRuntime(const FYcPlayerInventoryRuntime& Runtime, const FYcMetaPlayerSnapshot& PlayerSnapshot)
{
	return ApplyPlayerSnapshot(Runtime, nullptr, PlayerSnapshot);
}

bool UYcMetaInventorySubsystem::IsProfileDirty(const FYcProfileIdentity& ProfileIdentity) const
{
	return ProfileIdentity.IsValid() && DirtyProfiles.Contains(FYcProfileSaveKey(ProfileIdentity));
}

void UYcMetaInventorySubsystem::MarkProfileDirty(const FYcProfileIdentity& ProfileIdentity)
{
	if (ProfileIdentity.IsValid())
	{
		const FYcProfileSaveKey ProfileKey(ProfileIdentity);
		DirtyProfiles.Add(ProfileKey);
		if (UYcProfileSaveSubsystem* SaveSubsystem = GetProfileSaveSubsystem())
		{
			SaveSubsystem->MarkProfileDirty(ProfileIdentity);
		}
	}
}

void UYcMetaInventorySubsystem::ClearProfileDirty(const FYcProfileIdentity& ProfileIdentity)
{
	if (ProfileIdentity.IsValid())
	{
		const FYcProfileSaveKey ProfileKey(ProfileIdentity);
		DirtyProfiles.Remove(ProfileKey);
		if (UYcProfileSaveSubsystem* SaveSubsystem = GetProfileSaveSubsystem())
		{
			SaveSubsystem->ClearProfileDirty(ProfileIdentity);
		}
	}
}

bool UYcMetaInventorySubsystem::BuildSnapshotFromContext(UYcInventorySceneContext* Context, FYcMetaInventoryRootSnapshot& OutSnapshot) const
{
	if (!ValidateOutOfMatchContext(Context, TEXT("BuildSnapshotFromContext")))
	{
		return false;
	}

	OutSnapshot = YcMetaInventoryVersion::MakeEmptySnapshot(Context->ProfileIdentity);

	if (!BuildPlayerSnapshot(Context->Runtime, OutSnapshot.Player))
	{
		return false;
	}
	if (!BuildInventoryRecords(Context->Runtime.StashInventory, OutSnapshot.Stash.InventoryItems, OutSnapshot.Stash.InventoryExtensions))
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
	if (!RestoreInventoryRecords(Context->Runtime.StashInventory, Snapshot.Stash.InventoryItems, Snapshot.Stash.InventoryExtensions, StashItemMap))
	{
		return false;
	}

	if (!ApplyPlayerSnapshot(Context->Runtime, &StashItemMap, Snapshot.Player))
	{
		return false;
	}

	return true;
}

bool UYcMetaInventorySubsystem::BuildPlayerSnapshot(const FYcPlayerInventoryRuntime& Runtime, FYcMetaPlayerSnapshot& OutPlayerSnapshot) const
{
	UYcInventoryManagerComponent* PlayerInventory = Runtime.PlayerInventory;
	UYcInventoryManagerComponent* StashInventory = Runtime.StashInventory;
	if (!IsValid(PlayerInventory))
	{
		return false;
	}

	OutPlayerSnapshot = FYcMetaPlayerSnapshot();
	if (!BuildInventoryRecords(PlayerInventory, OutPlayerSnapshot.InventoryItems, OutPlayerSnapshot.InventoryExtensions))
	{
		return false;
	}

	BuildEquipmentRecords(Runtime, OutPlayerSnapshot.EquipmentSlots);
	BuildQuickBarRecords(Runtime, OutPlayerSnapshot.QuickBarSlots);

	OutPlayerSnapshot.InventoryItems.Sort([](const FYcMetaInventoryItemRecord& A, const FYcMetaInventoryItemRecord& B)
	{
		return A.ItemInstId.ToString() < B.ItemInstId.ToString();
	});

	return true;
}

bool UYcMetaInventorySubsystem::ApplyPlayerSnapshot(const FYcPlayerInventoryRuntime& Runtime, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>* StashItemMap, const FYcMetaPlayerSnapshot& PlayerSnapshot)
{
	UYcInventoryManagerComponent* PlayerInventory = Runtime.PlayerInventory;
	if (!IsValid(PlayerInventory))
	{
		return false;
	}

	const bool bHasInventoryPayload = !PlayerSnapshot.InventoryItems.IsEmpty() || !PlayerSnapshot.InventoryExtensions.IsEmpty();
	const bool bHasEquipmentPayload = !PlayerSnapshot.EquipmentSlots.IsEmpty();
	const bool bHasQuickBarPayload = !PlayerSnapshot.QuickBarSlots.IsEmpty();

	TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>> PlayerItemMap;
	if (bHasEquipmentPayload)
	{
		ClearEquipment(Runtime);
	}
	if (bHasQuickBarPayload)
	{
		ClearQuickBar(Runtime);
	}

	// 仅在快照中存在库存载荷时才覆盖库存，避免“空快照”把运行时默认物品清空。
	if (bHasInventoryPayload && !RestoreInventoryRecords(PlayerInventory, PlayerSnapshot.InventoryItems, TArray<FYcMetaInventoryExtensionPayload>(), PlayerItemMap))
	{
		return false;
	}

	if (!bHasInventoryPayload)
	{
		const TArray<UYcInventoryItemInstance*> ExistingItems = PlayerInventory->GetAllItemInstance();
		for (UYcInventoryItemInstance* ExistingItem : ExistingItems)
		{
			if (IsValid(ExistingItem))
			{
				PlayerItemMap.Add(ExistingItem->GetItemInstId(), ExistingItem);
			}
		}
	}

	auto MaterializeDetachedPlayerItem = [&](const FYcMetaInventoryItemRecord& DetachedRecord) -> bool
	{
		if (!DetachedRecord.ItemInstId.IsValid())
		{
			UE_LOG(LogYcInventory, Warning, TEXT("ApplyPlayerSnapshot: detached item record missing item instance id."));
			return false;
		}
		if (PlayerItemMap.Contains(DetachedRecord.ItemInstId))
		{
			return true;
		}
		if (!DetachedRecord.ItemRegistryId.IsValid() || DetachedRecord.StackCount <= 0)
		{
			UE_LOG(LogYcInventory, Warning, TEXT("ApplyPlayerSnapshot: detached item record invalid. itemId=%s registry=%s"),
				*DetachedRecord.ItemInstId.ToString(),
				*DetachedRecord.ItemRegistryId.ToString());
			return false;
		}

		UYcInventoryItemInstance* CreatedItem = PlayerInventory->AddItemWithInstanceId(
			DetachedRecord.ItemRegistryId,
			DetachedRecord.ItemInstId,
			DetachedRecord.StackCount);
		if (!CreatedItem)
		{
			UE_LOG(LogYcInventory, Warning, TEXT("ApplyPlayerSnapshot: failed to materialize detached slot item. itemId=%s registry=%s"),
				*DetachedRecord.ItemInstId.ToString(),
				*DetachedRecord.ItemRegistryId.ToString());
			return false;
		}

		TArray<FYcMetaItemExtensionPayload> UnknownPayloads;
		YcMetaInventoryItemRecordCodec::ImportToItem(*CreatedItem, DetachedRecord, UnknownPayloads);
		if (UnknownPayloads.IsEmpty())
		{
			UnknownItemExtensionPayloads.Remove(DetachedRecord.ItemInstId);
		}
		else
		{
			UnknownItemExtensionPayloads.Add(DetachedRecord.ItemInstId, MoveTemp(UnknownPayloads));
		}

		PlayerItemMap.Add(DetachedRecord.ItemInstId, CreatedItem);
		return true;
	};

	for (const FYcMetaEquipmentSlotRecord& Slot : PlayerSnapshot.EquipmentSlots)
	{
		if (Slot.bOwnsDetachedItem && !MaterializeDetachedPlayerItem(Slot.DetachedItemRecord))
		{
			return false;
		}
	}

	for (const FYcMetaQuickBarSlotRecord& Slot : PlayerSnapshot.QuickBarSlots)
	{
		if (Slot.bOwnsDetachedItem && !MaterializeDetachedPlayerItem(Slot.DetachedItemRecord))
		{
			return false;
		}
	}

	const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>> EmptyStashMap;
	const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& EffectiveStashItemMap = StashItemMap ? *StashItemMap : EmptyStashMap;

	auto CanResolveItemId = [&](const FYcItemInstanceId& ItemId, const EYcMetaItemSourceScope SourceScope) -> bool
	{
		switch (SourceScope)
		{
		case EYcMetaItemSourceScope::PlayerInventory:
			return PlayerItemMap.Contains(ItemId);
		case EYcMetaItemSourceScope::StashInventory:
			return EffectiveStashItemMap.Contains(ItemId);
		case EYcMetaItemSourceScope::Unknown:
		default:
			return PlayerItemMap.Contains(ItemId) || EffectiveStashItemMap.Contains(ItemId);
		}
	};

	auto CanRestoreEquipmentPayload = [&]() -> bool
	{
		for (const FYcMetaEquipmentSlotRecord& Slot : PlayerSnapshot.EquipmentSlots)
		{
			if (!CanResolveItemId(Slot.ItemInstId, Slot.SourceScope))
			{
				return false;
			}
		}
		return true;
	};

	auto CanRestoreQuickBarPayload = [&]() -> bool
	{
		for (const FYcMetaQuickBarSlotRecord& Slot : PlayerSnapshot.QuickBarSlots)
		{
			if (!CanResolveItemId(Slot.ItemInstId, Slot.SourceScope))
			{
				return false;
			}
		}
		return true;
	};

	const bool bCanRestoreEquipmentPayload = bHasEquipmentPayload && CanRestoreEquipmentPayload();
	const bool bCanRestoreQuickBarPayload = bHasQuickBarPayload && CanRestoreQuickBarPayload();

	if (bHasEquipmentPayload && !bCanRestoreEquipmentPayload)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("ApplyPlayerSnapshot: skip equipment payload because some item ids are unresolved. keep runtime equipment."));
	}
	if (bHasQuickBarPayload && !bCanRestoreQuickBarPayload)
	{
		UE_LOG(LogYcInventory, Warning, TEXT("ApplyPlayerSnapshot: skip quickbar payload because some item ids are unresolved. keep runtime quickbar."));
	}
	if (bCanRestoreEquipmentPayload)
	{
		if (!RestoreEquipment(Runtime, PlayerSnapshot.EquipmentSlots, PlayerItemMap, EffectiveStashItemMap))
		{
			return false;
		}
	}

	if (bCanRestoreQuickBarPayload)
	{
		if (!RestoreQuickBar(Runtime, PlayerSnapshot.QuickBarSlots, PlayerItemMap, EffectiveStashItemMap))
		{
			return false;
		}
	}

	if (bHasInventoryPayload && !ApplyInventoryExtensions(PlayerInventory, PlayerItemMap, PlayerSnapshot.InventoryExtensions))
	{
		return false;
	}

	// 强制广播一次当前槽位状态，确保跨场景持久化UI能立即刷新图标。
	if (DoesComponentImplement(Runtime.EquipmentBridge, UYcMetaInventoryEquipmentBridge::StaticClass()))
	{
		UActorComponent* EquipmentComp = Runtime.EquipmentBridge;
		IYcMetaInventoryEquipmentBridge::Execute_MetaNotifySlotsUpdated(EquipmentComp);
	}
	if (DoesComponentImplement(Runtime.QuickBarBridge, UYcMetaInventoryQuickBarBridge::StaticClass()))
	{
		UActorComponent* QuickBarComp = Runtime.QuickBarBridge;
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

		const bool bTouchesPlayer = (Message.Operation.SourceInventory == Context->Runtime.PlayerInventory || Message.Operation.TargetInventory == Context->Runtime.PlayerInventory);
		const bool bTouchesContainer = (Message.Operation.SourceInventory == Context->Runtime.StashInventory || Message.Operation.TargetInventory == Context->Runtime.StashInventory);
		if (bTouchesPlayer || bTouchesContainer)
		{
			const FYcProfileSaveKey ProfileKey(Context->ProfileIdentity);
			DirtyProfiles.Add(ProfileKey);
			if (UYcProfileSaveSubsystem* SaveSubsystem = GetProfileSaveSubsystem())
			{
				SaveSubsystem->MarkProfileDirty(Context->ProfileIdentity);
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

void UYcMetaInventorySubsystem::BuildEquipmentRecords(const FYcPlayerInventoryRuntime& Runtime, TArray<FYcMetaEquipmentSlotRecord>& OutSlots) const
{
	OutSlots.Empty();
	if (!DoesComponentImplement(Runtime.EquipmentBridge, UYcMetaInventoryEquipmentBridge::StaticClass()))
	{
		return;
	}
	UActorComponent* EquipmentComp = Runtime.EquipmentBridge;
	const UYcInventoryManagerComponent* PlayerInventory = Runtime.PlayerInventory;
	const UYcInventoryManagerComponent* StashInventory = Runtime.StashInventory;

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
		const bool bInPlayerInventory = InventoryActuallyContainsItem(PlayerInventory, SlotItem);
		const bool bInStashInventory = InventoryActuallyContainsItem(StashInventory, SlotItem);
		if (bInStashInventory)
		{
			SlotRecord.SourceScope = EYcMetaItemSourceScope::StashInventory;
		}
		else if (bInPlayerInventory)
		{
			SlotRecord.SourceScope = EYcMetaItemSourceScope::PlayerInventory;
		}
		else
		{
			SlotRecord.SourceScope = EYcMetaItemSourceScope::PlayerInventory;
			SlotRecord.bOwnsDetachedItem = true;
		}

		if (SlotRecord.bOwnsDetachedItem)
		{
			SlotRecord.DetachedItemRecord.ItemInstId = SlotRecord.ItemInstId;
			SlotRecord.DetachedItemRecord.ItemRegistryId = SlotItem->GetItemRegistryId();
			SlotRecord.DetachedItemRecord.StackCount = 1;
			const TArray<FYcMetaItemExtensionPayload>* UnknownPayloads = UnknownItemExtensionPayloads.Find(SlotRecord.ItemInstId);
			YcMetaInventoryItemRecordCodec::ExportFromItem(*SlotItem, UnknownPayloads, SlotRecord.DetachedItemRecord);
		}

		OutSlots.Add(SlotRecord);
	}
}

void UYcMetaInventorySubsystem::BuildQuickBarRecords(const FYcPlayerInventoryRuntime& Runtime, TArray<FYcMetaQuickBarSlotRecord>& OutSlots) const
{
	OutSlots.Empty();
	if (!DoesComponentImplement(Runtime.QuickBarBridge, UYcMetaInventoryQuickBarBridge::StaticClass()))
	{
		return;
	}
	UActorComponent* QuickBarComp = Runtime.QuickBarBridge;
	const UYcInventoryManagerComponent* PlayerInventory = Runtime.PlayerInventory;
	const UYcInventoryManagerComponent* StashInventory = Runtime.StashInventory;

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
		const bool bInPlayerInventory = InventoryActuallyContainsItem(PlayerInventory, SlotItem);
		const bool bInStashInventory = InventoryActuallyContainsItem(StashInventory, SlotItem);
		if (bInStashInventory)
		{
			SlotRecord.SourceScope = EYcMetaItemSourceScope::StashInventory;
		}
		else if (bInPlayerInventory)
		{
			SlotRecord.SourceScope = EYcMetaItemSourceScope::PlayerInventory;
		}
		else
		{
			SlotRecord.SourceScope = EYcMetaItemSourceScope::PlayerInventory;
			SlotRecord.bOwnsDetachedItem = true;
		}

		if (SlotRecord.bOwnsDetachedItem)
		{
			SlotRecord.DetachedItemRecord.ItemInstId = SlotRecord.ItemInstId;
			SlotRecord.DetachedItemRecord.ItemRegistryId = SlotItem->GetItemRegistryId();
			SlotRecord.DetachedItemRecord.StackCount = 1;
			const TArray<FYcMetaItemExtensionPayload>* UnknownPayloads = UnknownItemExtensionPayloads.Find(SlotRecord.ItemInstId);
			YcMetaInventoryItemRecordCodec::ExportFromItem(*SlotItem, UnknownPayloads, SlotRecord.DetachedItemRecord);
		}

		OutSlots.Add(SlotRecord);
	}
}

void UYcMetaInventorySubsystem::ClearEquipment(const FYcPlayerInventoryRuntime& Runtime) const
{
	if (!DoesComponentImplement(Runtime.EquipmentBridge, UYcMetaInventoryEquipmentBridge::StaticClass()))
	{
		return;
	}
	UActorComponent* EquipmentComp = Runtime.EquipmentBridge;

	TArray<FGameplayTag> OccupiedSlots;
	IYcMetaInventoryEquipmentBridge::Execute_GetMetaOccupiedSlots(EquipmentComp, OccupiedSlots);
	for (const FGameplayTag& SlotTag : OccupiedSlots)
	{
		IYcMetaInventoryEquipmentBridge::Execute_MetaUnequipSlot(EquipmentComp, SlotTag);
	}
}

void UYcMetaInventorySubsystem::ClearQuickBar(const FYcPlayerInventoryRuntime& Runtime) const
{
	if (!DoesComponentImplement(Runtime.QuickBarBridge, UYcMetaInventoryQuickBarBridge::StaticClass()))
	{
		return;
	}
	UActorComponent* QuickBarComp = Runtime.QuickBarBridge;

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

bool UYcMetaInventorySubsystem::RestoreEquipment(const FYcPlayerInventoryRuntime& Runtime, const TArray<FYcMetaEquipmentSlotRecord>& InSlots, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& PlayerItemMap, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& StashItemMap) const
{
	if (!DoesComponentImplement(Runtime.EquipmentBridge, UYcMetaInventoryEquipmentBridge::StaticClass()))
	{
		return true;
	}
	UActorComponent* EquipmentComp = Runtime.EquipmentBridge;

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

bool UYcMetaInventorySubsystem::RestoreQuickBar(const FYcPlayerInventoryRuntime& Runtime, const TArray<FYcMetaQuickBarSlotRecord>& InSlots, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& PlayerItemMap, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& StashItemMap) const
{
	if (!DoesComponentImplement(Runtime.QuickBarBridge, UYcMetaInventoryQuickBarBridge::StaticClass()))
	{
		return true;
	}
	UActorComponent* QuickBarComp = Runtime.QuickBarBridge;

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
