// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "System/YcQuestSubsystem.h"

#include "YcQuestAssetPolicy.h"
#include "YcQuestDefinition.h"
#include "YcQuestEffect.h"
#include "YcQuestObjective.h"
#include "YiChenQuest.h"
#include "Engine/AssetManager.h"
#include "System/YcQuestInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "System/YcPlayerIdentityProvider.h"
#include "Utils/CommonSimpleUtil.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcQuestSubsystem)

void UYcQuestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    WorldCleanupHandle = FWorldDelegates::OnWorldCleanup.AddUObject(this, &ThisClass::HandleWorldCleanup);
}

void UYcQuestSubsystem::Deinitialize()
{
    if (WorldCleanupHandle.IsValid())
    {
        FWorldDelegates::OnWorldCleanup.Remove(WorldCleanupHandle);
        WorldCleanupHandle.Reset();
    }

    RuntimeQuestInstances.Reset();
    BundleHandles.Reset();
    BundleRefStates.Reset();
    EventDedupTimestamps.Reset();
    SharedQuestMemberships.Reset();
    OwnerResolverObject = nullptr;
    ShareResolverObject = nullptr;
    AssetPolicy = nullptr;
    Super::Deinitialize();
}

UYcQuestSubsystem* UYcQuestSubsystem::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    if (const UGameInstance* GI = WorldContextObject->GetWorld() ? WorldContextObject->GetWorld()->GetGameInstance() : nullptr)
    {
        return GI->GetSubsystem<UYcQuestSubsystem>();
    }
    return nullptr;
}

bool UYcQuestSubsystem::ServerAcceptQuest(const FYcPlayerIdentitySnapshot& PlayerIdentity, const FName QuestId, FYcQuestInstanceKey& OutInstanceKey)
{
	if (!HasServerAuthority(TEXT("ServerAcceptQuest")))
    {
        OutInstanceKey = FYcQuestInstanceKey();
        return false;
    }

    const UYcQuestDefinition* QuestDef = ResolveQuestDefinition(QuestId);
	if (!QuestDef || !ResolveAcceptInstanceKey(QuestDef, PlayerIdentity, OutInstanceKey))
	{
		return false;
	}

    return ServerAcceptQuestForOwner(QuestId, OutInstanceKey.OwnerType, OutInstanceKey.OwnerId, OutInstanceKey);
}

bool UYcQuestSubsystem::ServerAcceptQuestForOwner(const FName QuestId, const EYcQuestOwnerType OwnerType, const FString& OwnerId, FYcQuestInstanceKey& OutInstanceKey)
{
    if (!HasServerAuthority(TEXT("ServerAcceptQuestForOwner")))
    {
        OutInstanceKey = FYcQuestInstanceKey();
        return false;
    }

    const UYcQuestDefinition* QuestDef = ResolveQuestDefinition(QuestId);
    if (!QuestDef || !QuestDef->RootObjective)
    {
        OutInstanceKey = FYcQuestInstanceKey();
        return false;
    }

    FYcQuestInstanceKey InstanceKey;
    InstanceKey.QuestId = QuestId;
    InstanceKey.OwnerType = OwnerType;
    InstanceKey.OwnerId = OwnerId;
    InstanceKey.Scope = QuestDef->QuestScope;
    if (!InstanceKey.IsValid())
    {
        OutInstanceKey = FYcQuestInstanceKey();
        return false;
    }

    if (UYcQuestInstance* ExistingInstance = RuntimeQuestInstances.FindRef(InstanceKey))
    {
        const EYcQuestState ExistingState = ExistingInstance->GetState();
        if (ExistingState == EYcQuestState::Accepted || ExistingState == EYcQuestState::InProgress)
        {
            OutInstanceKey = InstanceKey;
            return true;
        }

        if (IsTerminalState(ExistingState))
        {
            if (QuestDef->QuestScope == EYcQuestScope::MatchOnly)
            {
                DestroyQuestInstance(InstanceKey);
            }
            else
            {
                OutInstanceKey = FYcQuestInstanceKey();
                return false;
            }
        }
    }

    UYcQuestInstance* Instance = nullptr;
    if (!EnsureQuestInstance(InstanceKey, QuestDef, Instance) || !Instance)
    {
        OutInstanceKey = FYcQuestInstanceKey();
        return false;
    }

    Instance->SetState(EYcQuestState::Accepted, TEXT("Accepted"));
    if (QuestDef->ProgressOwnershipMode == EYcQuestProgressOwnershipMode::Shared)
    {
        RefreshSharedQuestMembersFromResolver(InstanceKey);
    }

    DispatchQuestEffects(Instance, EYcQuestEffectTrigger::QuestAccepted, NAME_None, FYcQuestPublicProgress());
    if (UYcQuestObjective* RootObjective = Instance->GetRuntimeRootObjective())
    {
        RootObjective->ActivateObjective(this, InstanceKey);
    }

    RequestQuestBundlesByInstance(InstanceKey, EYcQuestPhase::OnAccepted);
    RefreshInstanceStateFromObjective(Instance, InstanceKey);
    OutInstanceKey = InstanceKey;
    return true;
}

bool UYcQuestSubsystem::ServerAcceptQuestForPersistentOwner(const FName QuestId, const FYcPersistentOwnerKey& PersistentOwnerKey, FYcQuestInstanceKey& OutInstanceKey)
{
    if (!PersistentOwnerKey.IsValid())
    {
        OutInstanceKey = FYcQuestInstanceKey();
        return false;
    }

    EYcQuestOwnerType OwnerType = EYcQuestOwnerType::Player;
    switch (PersistentOwnerKey.OwnerType)
    {
    case EYcPersistentOwnerType::Account:
    case EYcPersistentOwnerType::Profile:
        OwnerType = EYcQuestOwnerType::Player;
        break;
    case EYcPersistentOwnerType::SharedGroup:
        OwnerType = EYcQuestOwnerType::SharedGroup;
        break;
    case EYcPersistentOwnerType::Match:
        OwnerType = EYcQuestOwnerType::Match;
        break;
    case EYcPersistentOwnerType::None:
    default:
        OutInstanceKey = FYcQuestInstanceKey();
        return false;
    }

    return ServerAcceptQuestForOwner(QuestId, OwnerType, PersistentOwnerKey.OwnerId, OutInstanceKey);
}

bool UYcQuestSubsystem::ServerCompleteQuestByInstance(const FYcQuestInstanceKey& InstanceKey, const FString& Detail)
{
    return TransitionInstanceState(InstanceKey, EYcQuestState::Completed, EYcQuestPhase::OnCompleted, Detail);
}

bool UYcQuestSubsystem::ServerFailQuestByInstance(const FYcQuestInstanceKey& InstanceKey, const FString& Detail)
{
    return TransitionInstanceState(InstanceKey, EYcQuestState::Failed, EYcQuestPhase::OnFailed, Detail);
}

bool UYcQuestSubsystem::ServerAbortQuestByInstance(const FYcQuestInstanceKey& InstanceKey, const FString& Detail)
{
    return TransitionInstanceState(InstanceKey, EYcQuestState::Aborted, EYcQuestPhase::OnAborted, Detail);
}

void UYcQuestSubsystem::ServerSubmitQuestEventRouted(const FYcQuestEvent& Event, const EYcQuestEventRouteScope RouteScope)
{
    if (!HasServerAuthority(TEXT("ServerSubmitQuestEventRouted")) || IsDuplicateEvent(Event))
    {
        return;
    }

    for (const TPair<FYcQuestInstanceKey, TObjectPtr<UYcQuestInstance>>& Pair : RuntimeQuestInstances)
    {
        const FYcQuestInstanceKey& InstanceKey = Pair.Key;
        const UYcQuestInstance* Instance = Pair.Value;
        if (!Instance || !Instance->IsActiveState())
        {
            continue;
        }

        const bool bIsMatchScope = InstanceKey.Scope == EYcQuestScope::MatchOnly;
        if ((RouteScope == EYcQuestEventRouteScope::Global && bIsMatchScope)
            || (RouteScope == EYcQuestEventRouteScope::Match && !bIsMatchScope))
        {
            continue;
        }

        bool bShouldProcess = true;
        if (const UYcQuestDefinition* QuestDef = ResolveQuestDefinition(InstanceKey.QuestId))
        {
            FYcQuestInstanceKey RoutedKey;
            if (ResolveEventInstanceKey(QuestDef, Event, RoutedKey))
            {
                bShouldProcess = RoutedKey == InstanceKey;
            }
        }

        if (bShouldProcess && IsSharedEventInstigatorAllowed(InstanceKey, Event))
        {
            ProcessQuestEventInternal(InstanceKey, Event);
        }
    }
}

void UYcQuestSubsystem::ServerSubmitQuestEventToInstance(const FYcQuestInstanceKey& InstanceKey, const FYcQuestEvent& Event)
{
    if (!HasServerAuthority(TEXT("ServerSubmitQuestEventToInstance")) || IsDuplicateEvent(Event) || !IsSharedEventInstigatorAllowed(InstanceKey, Event))
    {
        return;
    }

    ProcessQuestEventInternal(InstanceKey, Event);
}

bool UYcQuestSubsystem::RequestQuestBundlesByInstance(const FYcQuestInstanceKey& InstanceKey, const EYcQuestPhase Phase)
{
    const FPrimaryAssetId AssetId = ResolveQuestAssetId(InstanceKey.QuestId);
    if (!AssetId.IsValid())
    {
        return false;
    }

    const TArray<FName> BundleNames = ResolveBundlesForPhase(InstanceKey, Phase);
    for (const FName BundleName : BundleNames)
    {
        FYcQuestBundleRefState& RefState = BundleRefStates.FindOrAdd(MakeBundleRefKey(AssetId, BundleName));
        ++RefState.RefCount;
        RefState.bReady = true;
    }

    return true;
}

void UYcQuestSubsystem::ReleaseQuestBundlesByInstance(const FYcQuestInstanceKey& InstanceKey, const EYcQuestPhase Phase)
{
    const FPrimaryAssetId AssetId = ResolveQuestAssetId(InstanceKey.QuestId);
    if (!AssetId.IsValid())
    {
        return;
    }

    for (const FName BundleName : ResolveBundlesForPhase(InstanceKey, Phase))
    {
        const FString RefKey = MakeBundleRefKey(AssetId, BundleName);
        if (FYcQuestBundleRefState* RefState = BundleRefStates.Find(RefKey))
        {
            RefState->RefCount = FMath::Max(0, RefState->RefCount - 1);
            if (RefState->RefCount == 0)
            {
                BundleRefStates.Remove(RefKey);
            }
        }
    }
}

bool UYcQuestSubsystem::IsQuestBundleReadyByInstance(const FYcQuestInstanceKey& InstanceKey, const FName BundleName) const
{
    const FPrimaryAssetId AssetId = ResolveQuestAssetId(InstanceKey.QuestId);
    if (const FYcQuestBundleRefState* RefState = BundleRefStates.Find(MakeBundleRefKey(AssetId, BundleName)))
    {
        return RefState->bReady;
    }
    return false;
}

bool UYcQuestSubsystem::GetQuestRuntimeSnapshotByInstanceKey(const FYcQuestInstanceKey& InstanceKey, FYcQuestRuntimeSnapshot& OutSnapshot) const
{
    UYcQuestInstance* Instance = nullptr;
    if (!GetQuestInstanceByKey(InstanceKey, Instance) || !Instance)
    {
        OutSnapshot = FYcQuestRuntimeSnapshot();
        return false;
    }

    OutSnapshot.InstanceKey = Instance->GetInstanceKey();
    OutSnapshot.QuestId = Instance->GetQuestId();
    OutSnapshot.State = Instance->GetState();
    OutSnapshot.Version = Instance->GetVersion();
    OutSnapshot.LastUpdatedUnixTime = Instance->GetLastUpdatedUnixTime();
    OutSnapshot.ReplicatedPayload = Instance->GetReplicatedPayload();
    if (UYcQuestObjective* RootObjective = Instance->GetRuntimeRootObjective())
    {
        RootObjective->AppendRuntimeSnapshots(const_cast<UYcQuestSubsystem*>(this), InstanceKey, NAME_None, OutSnapshot.ObjectiveSnapshots);
        RootObjective->GatherVisibleProgress(OutSnapshot.VisibleObjectives);
    }
    return true;
}

void UYcQuestSubsystem::GetRuntimeQuestSnapshots(TArray<FYcQuestRuntimeSnapshot>& OutSnapshots) const
{
    OutSnapshots.Reset(RuntimeQuestInstances.Num());
    for (const TPair<FYcQuestInstanceKey, TObjectPtr<UYcQuestInstance>>& Pair : RuntimeQuestInstances)
    {
        FYcQuestRuntimeSnapshot Snapshot;
        if (GetQuestRuntimeSnapshotByInstanceKey(Pair.Key, Snapshot))
        {
            OutSnapshots.Add(Snapshot);
        }
    }
}

bool UYcQuestSubsystem::GetQuestInstanceByKey(const FYcQuestInstanceKey& InstanceKey, UYcQuestInstance*& OutInstance) const
{
    if (const TObjectPtr<UYcQuestInstance>* Found = RuntimeQuestInstances.Find(InstanceKey))
    {
        OutInstance = Found->Get();
        return OutInstance != nullptr;
    }
    OutInstance = nullptr;
    return false;
}

bool UYcQuestSubsystem::GetQuestPublicProgressByInstance(const FYcQuestInstanceKey& InstanceKey, TArray<FYcQuestPublicProgress>& OutProgress) const
{
    OutProgress.Reset();
    UYcQuestInstance* Instance = nullptr;
    if (!GetQuestInstanceByKey(InstanceKey, Instance) || !Instance || !Instance->GetRuntimeRootObjective())
    {
        return false;
    }

    Instance->GetRuntimeRootObjective()->GatherVisibleProgress(OutProgress);
    return true;
}

bool UYcQuestSubsystem::ServerSetCounterObjectiveTargetValueByInstance(const FYcQuestInstanceKey& InstanceKey, const FName ObjectiveId, const float NewTargetValue)
{
    if (!HasServerAuthority(TEXT("ServerSetCounterObjectiveTargetValueByInstance")) || ObjectiveId.IsNone())
    {
        return false;
    }

    UYcQuestInstance* Instance = nullptr;
    if (!GetQuestInstanceByKey(InstanceKey, Instance) || !Instance)
    {
        return false;
    }

    UYcQuestObjective* RootObjective = Instance->GetRuntimeRootObjective();
    if (!RootObjective)
    {
        return false;
    }

    UYcQuestObjective* FoundObjective = RootObjective->FindObjectiveById(ObjectiveId);
    UYcQuestCounterObjective* CounterObjective = Cast<UYcQuestCounterObjective>(FoundObjective);
    if (!CounterObjective)
    {
        return false;
    }

    CounterObjective->SetTargetValueRuntime(this, InstanceKey, NewTargetValue);
    RefreshInstanceStateFromObjective(Instance, InstanceKey);
    return true;
}

bool UYcQuestSubsystem::ServerSetQuestReplicatedPayloadByInstance(const FYcQuestInstanceKey& InstanceKey, const FInstancedStruct& Payload)
{
    UYcQuestInstance* Instance = nullptr;
    if (!GetQuestInstanceByKey(InstanceKey, Instance) || !Instance)
    {
        return false;
    }
    Instance->SetReplicatedPayload(Payload);
    return true;
}

bool UYcQuestSubsystem::GetQuestReplicatedPayloadByInstance(const FYcQuestInstanceKey& InstanceKey, FInstancedStruct& OutPayload) const
{
    UYcQuestInstance* Instance = nullptr;
    if (GetQuestInstanceByKey(InstanceKey, Instance) && Instance)
    {
        OutPayload = Instance->GetReplicatedPayload();
        return true;
    }
    OutPayload.Reset();
    return false;
}

bool UYcQuestSubsystem::ServerUpdateSharedQuestMembers(const FYcQuestInstanceKey& InstanceKey, const TArray<FString>& MemberIds)
{
    if (!InstanceKey.IsValid() || InstanceKey.OwnerType != EYcQuestOwnerType::SharedGroup)
    {
        return false;
    }

    FYcQuestSharedMembership& Membership = SharedQuestMemberships.FindOrAdd(InstanceKey);
    Membership.InstanceKey = InstanceKey;
    Membership.TeamId = InstanceKey.OwnerId;
    Membership.ActiveMemberPlayerIds = MemberIds;
    Membership.Version += 1;
    Membership.LastUpdatedTime = YcTimeUtils::GetUtcNowUnixTimestampSeconds();
    return true;
}

bool UYcQuestSubsystem::IsPlayerInSharedQuest(const FYcQuestInstanceKey& InstanceKey, const FString& PlayerId) const
{
    if (const FYcQuestSharedMembership* Membership = SharedQuestMemberships.Find(InstanceKey))
    {
        return Membership->ActiveMemberPlayerIds.ContainsByPredicate([&PlayerId](const FString& Existing) { return Existing.Equals(PlayerId, ESearchCase::IgnoreCase); });
    }
    return false;
}

bool UYcQuestSubsystem::GetSharedQuestMembers(const FYcQuestInstanceKey& InstanceKey, TArray<FString>& OutMemberIds) const
{
    OutMemberIds.Reset();
    if (const FYcQuestSharedMembership* Membership = SharedQuestMemberships.Find(InstanceKey))
    {
        OutMemberIds = Membership->ActiveMemberPlayerIds;
        return true;
    }
    return false;
}

bool UYcQuestSubsystem::IsQuestActive(const FYcQuestInstanceKey& InstanceKey) const
{
    UYcQuestInstance* Instance = nullptr;
    return GetQuestInstanceByKey(InstanceKey, Instance) && Instance && Instance->IsActiveState();
}

bool UYcQuestSubsystem::BuildSaveSnapshot(FYcQuestSaveSnapshot& OutSnapshot) const
{
    OutSnapshot.SnapshotVersion = 2;
    OutSnapshot.RuntimeEntries.Reset();
    GetRuntimeQuestSnapshots(OutSnapshot.RuntimeEntries);
    return true;
}

bool UYcQuestSubsystem::ApplySaveSnapshot(const FYcQuestSaveSnapshot& InSnapshot)
{
    TArray<FYcQuestInstanceKey> ExistingKeys;
    RuntimeQuestInstances.GetKeys(ExistingKeys);
    for (const FYcQuestInstanceKey& Key : ExistingKeys)
    {
        DestroyQuestInstance(Key);
    }

    for (const FYcQuestRuntimeSnapshot& Entry : InSnapshot.RuntimeEntries)
    {
        const UYcQuestDefinition* QuestDef = ResolveQuestDefinition(Entry.QuestId);
        if (!QuestDef)
        {
            continue;
        }

        UYcQuestInstance* Instance = nullptr;
        if (!EnsureQuestInstance(Entry.InstanceKey, QuestDef, Instance) || !Instance)
        {
            continue;
        }

        Instance->RestoreFromSnapshot(Entry.InstanceKey, Entry.QuestId, Entry.State, Entry.Version, Entry.LastUpdatedUnixTime, Entry.ReplicatedPayload);
        if (UYcQuestObjective* RootObjective = Instance->GetRuntimeRootObjective())
        {
            RootObjective->RestoreRuntimeSnapshot(this, Entry.InstanceKey, Entry.ObjectiveSnapshots);
        }
    }

    return true;
}

void UYcQuestSubsystem::DispatchQuestEffects(const UYcQuestInstance* Instance, const EYcQuestEffectTrigger Trigger, const FName ObjectiveId, const FYcQuestPublicProgress& Progress) const
{
    if (!Instance)
    {
        return;
    }

    for (UYcQuestEffect* Effect : Instance->GetRuntimeQuestEffects())
    {
        if (Effect)
        {
            Effect->ExecuteEffect(const_cast<UYcQuestSubsystem*>(this), Instance->GetInstanceKey(), Trigger, ObjectiveId, Progress);
        }
    }
}

void UYcQuestSubsystem::Tick(const float DeltaTime)
{
    TArray<FYcQuestInstanceKey> Keys;
    RuntimeQuestInstances.GetKeys(Keys);
    for (const FYcQuestInstanceKey& Key : Keys)
    {
        UYcQuestInstance* Instance = nullptr;
        if (!GetQuestInstanceByKey(Key, Instance) || !Instance || !Instance->IsActiveState())
        {
            continue;
        }

        if (UYcQuestObjective* RootObjective = Instance->GetRuntimeRootObjective())
        {
            RootObjective->TickObjective(this, Key, DeltaTime);
            RefreshInstanceStateFromObjective(Instance, Key);
        }
    }
}

TStatId UYcQuestSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UYcQuestSubsystem, STATGROUP_Tickables);
}

bool UYcQuestSubsystem::IsTickable() const
{
    return RuntimeQuestInstances.Num() > 0;
}

void UYcQuestSubsystem::HandleWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
    (void)bSessionEnded;
    (void)bCleanupResources;

    if (!World || World->GetGameInstance() != GetGameInstance())
    {
        return;
    }

    ResetMatchScopeQuestInstances();
}

void UYcQuestSubsystem::ResetMatchScopeQuestInstances()
{
    TArray<FYcQuestInstanceKey> KeysToRemove;
    for (const TPair<FYcQuestInstanceKey, TObjectPtr<UYcQuestInstance>>& Pair : RuntimeQuestInstances)
    {
        if (Pair.Key.Scope == EYcQuestScope::MatchOnly)
        {
            KeysToRemove.Add(Pair.Key);
        }
    }

    for (const FYcQuestInstanceKey& Key : KeysToRemove)
    {
        DestroyQuestInstance(Key);
    }
}

bool UYcQuestSubsystem::IsTerminalState(const EYcQuestState State) const
{
    return State == EYcQuestState::Completed
        || State == EYcQuestState::Failed
        || State == EYcQuestState::Aborted;
}

bool UYcQuestSubsystem::HasServerAuthority(const TCHAR* ActionName) const
{
    const UWorld* World = GetWorld();
    const bool bHasAuthority = World && World->GetNetMode() != NM_Client;
    if (!bHasAuthority)
    {
        UE_LOG(LogYcQuest, Warning, TEXT("%s rejected: server authority required."), ActionName);
    }
    return bHasAuthority;
}

bool UYcQuestSubsystem::IsValidStateTransition(const EYcQuestState CurrentState, const EYcQuestState NewState) const
{
    if (CurrentState == NewState)
    {
        return false;
    }

    switch (CurrentState)
    {
    case EYcQuestState::Available:
        return NewState == EYcQuestState::Accepted;
    case EYcQuestState::Accepted:
    case EYcQuestState::InProgress:
        return NewState == EYcQuestState::Completed || NewState == EYcQuestState::Failed || NewState == EYcQuestState::Aborted || NewState == EYcQuestState::InProgress;
    default:
        return false;
    }
}

UYcQuestAssetPolicy* UYcQuestSubsystem::GetOrCreateAssetPolicy()
{
    if (AssetPolicy)
    {
        return AssetPolicy;
    }

    UClass* PolicyClass = AssetPolicyClass.LoadSynchronous();
    if (!PolicyClass)
    {
        PolicyClass = UYcQuestAssetPolicy::StaticClass();
    }

    AssetPolicy = NewObject<UYcQuestAssetPolicy>(this, PolicyClass);
    return AssetPolicy;
}

UObject* UYcQuestSubsystem::GetOrCreateOwnerResolver()
{
    if (!OwnerResolverObject && !OwnerResolverClass.IsNull())
    {
        if (UClass* ResolverClass = OwnerResolverClass.LoadSynchronous())
        {
            OwnerResolverObject = NewObject<UObject>(this, ResolverClass);
        }
    }
    return OwnerResolverObject;
}

UObject* UYcQuestSubsystem::GetOrCreateShareResolver()
{
    if (!ShareResolverObject && !ShareResolverClass.IsNull())
    {
        if (UClass* ResolverClass = ShareResolverClass.LoadSynchronous())
        {
            ShareResolverObject = NewObject<UObject>(this, ResolverClass);
        }
    }
    return ShareResolverObject;
}

const UYcQuestDefinition* UYcQuestSubsystem::ResolveQuestDefinition(const FName QuestId) const
{
    if (QuestId.IsNone())
    {
        return nullptr;
    }

    if (UAssetManager::IsInitialized())
    {
        const FPrimaryAssetId AssetId(UYcQuestDefinition::QuestDefinitionType, QuestId);
        if (UObject* LoadedObject = UAssetManager::Get().GetPrimaryAssetObject(AssetId))
        {
            return Cast<UYcQuestDefinition>(LoadedObject);
        }

        if (const FSoftObjectPath AssetPath = UAssetManager::Get().GetPrimaryAssetPath(AssetId); AssetPath.IsValid())
        {
            return Cast<UYcQuestDefinition>(AssetPath.TryLoad());
        }
    }

    return nullptr;
}

bool UYcQuestSubsystem::ResolveAcceptInstanceKey(const UYcQuestDefinition* QuestDef, const FYcPlayerIdentitySnapshot& PlayerIdentity, FYcQuestInstanceKey& OutInstanceKey) const
{
	if (!QuestDef || !PlayerIdentity.IsReady())
	{
		return false;
	}

	OutInstanceKey.QuestId = QuestDef->QuestId.IsNone() ? QuestDef->GetFName() : QuestDef->QuestId;
	OutInstanceKey.Scope = QuestDef->QuestScope;
	OutInstanceKey.OwnerType = QuestDef->ProgressOwnershipMode == EYcQuestProgressOwnershipMode::Shared ? EYcQuestOwnerType::SharedGroup : EYcQuestOwnerType::Player;
	OutInstanceKey.OwnerId = QuestDef->ProgressOwnershipMode == EYcQuestProgressOwnershipMode::Shared
		? PlayerIdentity.AccountIdentity.ToOwnerId()
		: PlayerIdentity.GetPersistentOwnerId();

	if (QuestDef->ProgressOwnershipMode == EYcQuestProgressOwnershipMode::Shared)
	{
        if (UObject* ShareResolver = const_cast<UYcQuestSubsystem*>(this)->GetOrCreateShareResolver())
        {
            EYcQuestOwnerType ResolvedType = OutInstanceKey.OwnerType;
            FString ResolvedOwnerId = OutInstanceKey.OwnerId;
			if (IYcQuestShareResolver::Execute_ResolveSharedOwnerForAccept(ShareResolver, PlayerIdentity, QuestDef, ResolvedType, ResolvedOwnerId))
			{
                OutInstanceKey.OwnerType = ResolvedType;
                OutInstanceKey.OwnerId = ResolvedOwnerId;
            }
        }
        return OutInstanceKey.IsValid();
    }

    if (UObject* OwnerResolver = const_cast<UYcQuestSubsystem*>(this)->GetOrCreateOwnerResolver())
    {
        EYcQuestOwnerType ResolvedType = OutInstanceKey.OwnerType;
        FString ResolvedOwnerId = OutInstanceKey.OwnerId;
		if (IYcQuestOwnerResolver::Execute_ResolveOwnerForAccept(OwnerResolver, PlayerIdentity, QuestDef, ResolvedType, ResolvedOwnerId))
		{
            OutInstanceKey.OwnerType = ResolvedType;
            OutInstanceKey.OwnerId = ResolvedOwnerId;
        }
    }

    return OutInstanceKey.IsValid();
}

bool UYcQuestSubsystem::ResolveEventInstanceKey(const UYcQuestDefinition* QuestDef, const FYcQuestEvent& Event, FYcQuestInstanceKey& OutInstanceKey) const
{
    if (!QuestDef)
    {
        return false;
    }

    OutInstanceKey.QuestId = QuestDef->QuestId.IsNone() ? QuestDef->GetFName() : QuestDef->QuestId;
    OutInstanceKey.Scope = QuestDef->QuestScope;

    if (QuestDef->ProgressOwnershipMode == EYcQuestProgressOwnershipMode::Shared)
    {
        OutInstanceKey.OwnerType = EYcQuestOwnerType::SharedGroup;
        if (UObject* ShareResolver = const_cast<UYcQuestSubsystem*>(this)->GetOrCreateShareResolver())
        {
            EYcQuestOwnerType ResolvedType = OutInstanceKey.OwnerType;
            FString ResolvedOwnerId;
            if (IYcQuestShareResolver::Execute_ResolveSharedOwnerForEvent(ShareResolver, Event, QuestDef, ResolvedType, ResolvedOwnerId))
            {
                OutInstanceKey.OwnerType = ResolvedType;
                OutInstanceKey.OwnerId = ResolvedOwnerId;
                return OutInstanceKey.IsValid();
            }
        }
        return false;
    }

    OutInstanceKey.OwnerType = EYcQuestOwnerType::Player;
    if (UObject* OwnerResolver = const_cast<UYcQuestSubsystem*>(this)->GetOrCreateOwnerResolver())
    {
        EYcQuestOwnerType ResolvedType = OutInstanceKey.OwnerType;
        FString ResolvedOwnerId;
        if (IYcQuestOwnerResolver::Execute_ResolveOwnerForEvent(OwnerResolver, Event, QuestDef, ResolvedType, ResolvedOwnerId))
        {
            OutInstanceKey.OwnerType = ResolvedType;
            OutInstanceKey.OwnerId = ResolvedOwnerId;
            return OutInstanceKey.IsValid();
        }
    }

	FYcPlayerIdentitySnapshot InstigatorIdentity;
	if (TryResolvePlayerIdentityFromObject(Event.Instigator.Get(), InstigatorIdentity))
	{
		OutInstanceKey.OwnerId = InstigatorIdentity.GetPersistentOwnerId();
		return !OutInstanceKey.OwnerId.IsEmpty();
	}
	return false;
}

bool UYcQuestSubsystem::IsDuplicateEvent(const FYcQuestEvent& Event)
{
    if (!Event.EventId.IsValid())
    {
        return false;
    }

    PruneEventDedupCache();
    if (EventDedupTimestamps.Contains(Event.EventId))
    {
        return true;
    }

    EventDedupTimestamps.Add(Event.EventId, YcTimeUtils::GetUtcNowUnixTimestampSeconds());
    return false;
}

void UYcQuestSubsystem::PruneEventDedupCache()
{
    const int64 Now = YcTimeUtils::GetUtcNowUnixTimestampSeconds();
    for (auto It = EventDedupTimestamps.CreateIterator(); It; ++It)
    {
        if (Now - It.Value() > 60)
        {
            It.RemoveCurrent();
        }
    }
}

bool UYcQuestSubsystem::EnsureQuestInstance(const FYcQuestInstanceKey& InstanceKey, const UYcQuestDefinition* QuestDef, UYcQuestInstance*& OutInstance)
{
    if (const TObjectPtr<UYcQuestInstance>* Found = RuntimeQuestInstances.Find(InstanceKey))
    {
        OutInstance = Found->Get();
        return OutInstance != nullptr;
    }

    if (!QuestDef || !QuestDef->RootObjective)
    {
        OutInstance = nullptr;
        return false;
    }

    UYcQuestInstance* Instance = NewObject<UYcQuestInstance>(this);
    Instance->InitializeInstance(InstanceKey, InstanceKey.QuestId, EYcQuestState::Available);
    Instance->SetRuntimeRootObjective(DuplicateObject<UYcQuestObjective>(QuestDef->RootObjective, this));

    TArray<TObjectPtr<UYcQuestEffect>> RuntimeEffects;
    RuntimeEffects.Reserve(QuestDef->QuestEffects.Num());
    for (UYcQuestEffect* Effect : QuestDef->QuestEffects)
    {
        if (Effect)
        {
            RuntimeEffects.Add(DuplicateObject<UYcQuestEffect>(Effect, this));
        }
    }
    Instance->SetRuntimeQuestEffects(RuntimeEffects);

    RuntimeQuestInstances.Add(InstanceKey, Instance);
    OutInstance = Instance;
    return true;
}

void UYcQuestSubsystem::DestroyQuestInstance(const FYcQuestInstanceKey& InstanceKey)
{
    ReleaseQuestBundlesByInstance(InstanceKey, EYcQuestPhase::OnAccepted);
    ReleaseQuestBundlesByInstance(InstanceKey, EYcQuestPhase::OnStartedInMatch);
    ReleaseQuestBundlesByInstance(InstanceKey, EYcQuestPhase::OnReturnOutOfMatch);
    ReleaseQuestBundlesByInstance(InstanceKey, EYcQuestPhase::OnCompleted);
    ReleaseQuestBundlesByInstance(InstanceKey, EYcQuestPhase::OnFailed);
    ReleaseQuestBundlesByInstance(InstanceKey, EYcQuestPhase::OnAborted);
    RuntimeQuestInstances.Remove(InstanceKey);
    SharedQuestMemberships.Remove(InstanceKey);
}

bool UYcQuestSubsystem::TransitionInstanceState(const FYcQuestInstanceKey& InstanceKey, const EYcQuestState NewState, const EYcQuestPhase Phase, const FString& Detail)
{
    UYcQuestInstance* Instance = nullptr;
    if (!GetQuestInstanceByKey(InstanceKey, Instance) || !Instance)
    {
        UE_LOG(LogYcQuest, Warning, TEXT("[QuestSubsystem] TransitionInstanceState failed: instance not found. QuestId=%s NewState=%d Detail=%s"),
            *InstanceKey.QuestId.ToString(), static_cast<int32>(NewState), *Detail);
        return false;
    }

    const EYcQuestState CurrentState = Instance->GetState();
    if (!IsValidStateTransition(CurrentState, NewState))
    {
        UE_LOG(LogYcQuest, Warning, TEXT("[QuestSubsystem] TransitionInstanceState rejected. QuestId=%s CurrentState=%d NewState=%d Detail=%s"),
            *InstanceKey.QuestId.ToString(), static_cast<int32>(CurrentState), static_cast<int32>(NewState), *Detail);
        return false;
    }

    Instance->SetState(NewState, Detail);
    ReleaseQuestBundlesByInstance(InstanceKey, Phase);

    EYcQuestEffectTrigger Trigger = EYcQuestEffectTrigger::QuestCompleted;
    switch (NewState)
    {
    case EYcQuestState::Completed: Trigger = EYcQuestEffectTrigger::QuestCompleted; break;
    case EYcQuestState::Failed: Trigger = EYcQuestEffectTrigger::QuestFailed; break;
    case EYcQuestState::Aborted: Trigger = EYcQuestEffectTrigger::QuestAborted; break;
    default: break;
    }

    UE_LOG(LogYcQuest, Log, TEXT("[QuestSubsystem] TransitionInstanceState success. QuestId=%s CurrentState=%d NewState=%d Trigger=%d Detail=%s"),
        *InstanceKey.QuestId.ToString(), static_cast<int32>(CurrentState), static_cast<int32>(NewState), static_cast<int32>(Trigger), *Detail);

    DispatchQuestEffects(Instance, Trigger, NAME_None, FYcQuestPublicProgress());
    return true;
}

void UYcQuestSubsystem::ProcessQuestEventInternal(const FYcQuestInstanceKey& InstanceKey, const FYcQuestEvent& Event)
{
    UYcQuestInstance* Instance = nullptr;
    if (!GetQuestInstanceByKey(InstanceKey, Instance) || !Instance || !Instance->GetRuntimeRootObjective())
    {
        return;
    }

    if (Instance->GetState() == EYcQuestState::Accepted)
    {
        Instance->SetState(EYcQuestState::InProgress, TEXT("InProgress"));
        RequestQuestBundlesByInstance(InstanceKey, EYcQuestPhase::OnStartedInMatch);
    }

    Instance->GetRuntimeRootObjective()->HandleQuestEvent(this, InstanceKey, Event);
    RefreshInstanceStateFromObjective(Instance, InstanceKey);
}

bool UYcQuestSubsystem::RefreshSharedQuestMembersFromResolver(const FYcQuestInstanceKey& InstanceKey)
{
    TArray<FString> MemberIds;
    if (!ResolveSharedMembersForInstance(InstanceKey, MemberIds))
    {
        return false;
    }
    return ServerUpdateSharedQuestMembers(InstanceKey, MemberIds);
}

bool UYcQuestSubsystem::ResolveSharedMembersForInstance(const FYcQuestInstanceKey& InstanceKey, TArray<FString>& OutMemberIds) const
{
    OutMemberIds.Reset();
    if (InstanceKey.OwnerType != EYcQuestOwnerType::SharedGroup || InstanceKey.OwnerId.IsEmpty())
    {
        return false;
    }

    if (UObject* ShareResolver = const_cast<UYcQuestSubsystem*>(this)->GetOrCreateShareResolver())
    {
        return IYcQuestShareResolver::Execute_ResolveSharedMembers(ShareResolver, InstanceKey.OwnerType, InstanceKey.OwnerId, OutMemberIds);
    }
    return false;
}

bool UYcQuestSubsystem::TryResolvePlayerIdentityFromObject(const UObject* SourceObject, FYcPlayerIdentitySnapshot& OutPlayerIdentity) const
{
	OutPlayerIdentity = FYcPlayerIdentitySnapshot();
	if (!IsValid(SourceObject))
	{
		return false;
	}

	auto ResolveFromObject = [&OutPlayerIdentity](const UObject* Object) -> bool
	{
		if (!IsValid(Object) || !Object->GetClass()->ImplementsInterface(UYcPlayerIdentityProvider::StaticClass()))
		{
			return false;
		}

		return IYcPlayerIdentityProvider::Execute_GetPlayerIdentity(Object, OutPlayerIdentity) && OutPlayerIdentity.IsReady();
	};

	if (ResolveFromObject(SourceObject))
	{
		return true;
	}
	if (const APlayerState* PlayerState = Cast<APlayerState>(SourceObject))
	{
		return ResolveFromObject(PlayerState);
	}
	if (const APlayerController* PlayerController = Cast<APlayerController>(SourceObject))
	{
		if (ResolveFromObject(PlayerController))
		{
			return true;
		}
		return ResolveFromObject(PlayerController->PlayerState);
	}
	if (const APawn* Pawn = Cast<APawn>(SourceObject))
	{
		if (ResolveFromObject(Pawn))
		{
			return true;
		}
		if (ResolveFromObject(Pawn->GetPlayerState()))
		{
			return true;
		}
		return ResolveFromObject(Pawn->GetController());
	}
	return false;
}

bool UYcQuestSubsystem::IsSharedEventInstigatorAllowed(const FYcQuestInstanceKey& InstanceKey, const FYcQuestEvent& Event)
{
	if (InstanceKey.OwnerType != EYcQuestOwnerType::SharedGroup)
	{
		return true;
	}

	RefreshSharedQuestMembersFromResolver(InstanceKey);
	FYcPlayerIdentitySnapshot PlayerIdentity;
	return TryResolvePlayerIdentityFromObject(Event.Instigator.Get(), PlayerIdentity)
		&& IsPlayerInSharedQuest(InstanceKey, PlayerIdentity.GetPersistentOwnerId());
}

void UYcQuestSubsystem::RefreshInstanceStateFromObjective(UYcQuestInstance* Instance, const FYcQuestInstanceKey& InstanceKey)
{
    if (!Instance)
    {
        return;
    }

    UYcQuestObjective* RootObjective = Instance->GetRuntimeRootObjective();
    if (!RootObjective)
    {
        return;
    }

    switch (RootObjective->GetObjectiveState())
    {
    case EYcQuestObjectiveState::Completed:
        ServerCompleteQuestByInstance(InstanceKey, TEXT("RootObjectiveCompleted"));
        break;
    case EYcQuestObjectiveState::Failed:
        ServerFailQuestByInstance(InstanceKey, TEXT("RootObjectiveFailed"));
        break;
    default:
        break;
    }
}

TArray<FName> UYcQuestSubsystem::ResolveBundlesForPhase(const FYcQuestInstanceKey& InstanceKey, const EYcQuestPhase Phase) const
{
    TArray<FName> Bundles;
    const UYcQuestDefinition* QuestDef = ResolveQuestDefinition(InstanceKey.QuestId);
    if (UYcQuestAssetPolicy* Policy = const_cast<UYcQuestSubsystem*>(this)->GetOrCreateAssetPolicy())
    {
        Policy->GetBundlesForPhase(InstanceKey.QuestId, QuestDef, Phase, Bundles);
    }
    return Bundles;
}

FPrimaryAssetId UYcQuestSubsystem::ResolveQuestAssetId(const FName QuestId) const
{
    return QuestId.IsNone() ? FPrimaryAssetId() : FPrimaryAssetId(UYcQuestDefinition::QuestDefinitionType, QuestId);
}

FString UYcQuestSubsystem::MakeBundleRefKey(const FPrimaryAssetId& AssetId, const FName BundleName) const
{
    return AssetId.ToString() + TEXT("|") + BundleName.ToString();
}
