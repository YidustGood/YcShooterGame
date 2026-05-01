// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcQuestObjective.h"

#include "System/YcQuestSubsystem.h"
#include "YcQuestEffect.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcQuestObjective)

void UYcQuestObjective::ActivateObjective(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey)
{
    if (RuntimeState == EYcQuestObjectiveState::Completed || RuntimeState == EYcQuestObjectiveState::Failed)
    {
        return;
    }

    RuntimeState = EYcQuestObjectiveState::Active;
    RuntimeActiveSeconds = 0.0f;
    if (RuntimePublicProgress.DisplayText.IsEmpty())
    {
        RuntimePublicProgress.DisplayText = DisplayText.ToString();
        RuntimePublicProgress.ObjectiveId = ObjectiveId;
        RuntimePublicProgress.bIsVisible = bVisibleInPublicProgress;
    }

    OnActivated(QuestSubsystem, InstanceKey);
    DispatchEffects(QuestSubsystem, InstanceKey, EYcQuestEffectTrigger::ObjectiveActivated);
    ActivateInitialChildren(QuestSubsystem, InstanceKey);
}

void UYcQuestObjective::DeactivateObjective(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey)
{
    if (RuntimeState == EYcQuestObjectiveState::Inactive)
    {
        return;
    }

    OnDeactivated(QuestSubsystem, InstanceKey);
    RuntimeState = EYcQuestObjectiveState::Inactive;
}

void UYcQuestObjective::HandleQuestEvent(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const FYcQuestEvent& Event)
{
    if (RuntimeState != EYcQuestObjectiveState::Active)
    {
        return;
    }

    OnQuestEvent(QuestSubsystem, InstanceKey, Event);
    HandleChildrenQuestEvent(QuestSubsystem, InstanceKey, Event);
    RefreshCompletionFromChildren(QuestSubsystem, InstanceKey);
}

void UYcQuestObjective::TickObjective(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const float DeltaSeconds)
{
    if (RuntimeState != EYcQuestObjectiveState::Active)
    {
        return;
    }

    RuntimeActiveSeconds += FMath::Max(0.0f, DeltaSeconds);
    OnObjectiveTick(QuestSubsystem, InstanceKey, DeltaSeconds);
    TickChildren(QuestSubsystem, InstanceKey, DeltaSeconds);
    RefreshCompletionFromChildren(QuestSubsystem, InstanceKey);
}

void UYcQuestObjective::RestoreRuntimeSnapshot(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const TArray<FYcQuestObjectiveRuntimeSnapshot>& Snapshots)
{
    const FYcQuestObjectiveRuntimeSnapshot* Snapshot = Snapshots.FindByPredicate([this](const FYcQuestObjectiveRuntimeSnapshot& Entry)
    {
        return Entry.ObjectiveId == ObjectiveId;
    });
    if (!Snapshot)
    {
        return;
    }

    RuntimeState = Snapshot->State;
    RuntimeActiveSeconds = Snapshot->ActiveSeconds;
    RuntimePublicProgress = Snapshot->PublicProgress;
    ImportCustomSnapshot(QuestSubsystem, InstanceKey, Snapshot->CustomSnapshot);

    for (UYcQuestObjective* Child : ChildObjectives)
    {
        if (Child)
        {
            Child->RestoreRuntimeSnapshot(QuestSubsystem, InstanceKey, Snapshots);
        }
    }
}

void UYcQuestObjective::AppendRuntimeSnapshots(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const FName ParentObjectiveId, TArray<FYcQuestObjectiveRuntimeSnapshot>& OutSnapshots) const
{
    FYcQuestObjectiveRuntimeSnapshot Snapshot;
    Snapshot.ParentObjectiveId = ParentObjectiveId;
    Snapshot.ObjectiveId = ObjectiveId;
    Snapshot.ObjectiveClassName = GetClass()->GetName();
    Snapshot.State = RuntimeState;
    Snapshot.ActiveSeconds = RuntimeActiveSeconds;
    Snapshot.PublicProgress = RuntimePublicProgress;
    Snapshot.CustomSnapshot = ExportCustomSnapshot(QuestSubsystem, InstanceKey);
    OutSnapshots.Add(Snapshot);

    for (const UYcQuestObjective* Child : ChildObjectives)
    {
        if (Child)
        {
            Child->AppendRuntimeSnapshots(QuestSubsystem, InstanceKey, ObjectiveId, OutSnapshots);
        }
    }
}

void UYcQuestObjective::GatherVisibleProgress(TArray<FYcQuestPublicProgress>& OutProgress) const
{
    if (RuntimePublicProgress.bIsVisible && RuntimeState == EYcQuestObjectiveState::Active)
    {
        OutProgress.Add(RuntimePublicProgress);
    }

    for (const UYcQuestObjective* Child : ChildObjectives)
    {
        if (Child)
        {
            Child->GatherVisibleProgress(OutProgress);
        }
    }
}

EYcQuestObjectiveState UYcQuestObjective::GetObjectiveState() const
{
    return RuntimeState;
}

FYcQuestPublicProgress UYcQuestObjective::GetPublicProgress() const
{
    return RuntimePublicProgress;
}

void UYcQuestObjective::CompleteObjective(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey)
{
    if (RuntimeState == EYcQuestObjectiveState::Completed)
    {
        return;
    }

    RuntimeState = EYcQuestObjectiveState::Completed;
    OnCompleted(QuestSubsystem, InstanceKey);
    DispatchEffects(QuestSubsystem, InstanceKey, EYcQuestEffectTrigger::ObjectiveCompleted);
}

void UYcQuestObjective::FailObjective(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey)
{
    if (RuntimeState == EYcQuestObjectiveState::Failed)
    {
        return;
    }

    RuntimeState = EYcQuestObjectiveState::Failed;
    OnFailed(QuestSubsystem, InstanceKey);
    DispatchEffects(QuestSubsystem, InstanceKey, EYcQuestEffectTrigger::ObjectiveFailed);
}

void UYcQuestObjective::BlockObjective()
{
    RuntimeState = EYcQuestObjectiveState::Blocked;
}

void UYcQuestObjective::SetPublicProgress(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const FString& InDisplayText, const float InCurrentValue, const float InTargetValue, const float InRemainingSeconds, const bool bInVisible)
{
    RuntimePublicProgress.ObjectiveId = ObjectiveId;
    RuntimePublicProgress.DisplayText = InDisplayText;
    RuntimePublicProgress.CurrentValue = InCurrentValue;
    RuntimePublicProgress.TargetValue = InTargetValue;
    RuntimePublicProgress.RemainingSeconds = InRemainingSeconds;
    RuntimePublicProgress.bIsVisible = bInVisible;
    DispatchEffects(QuestSubsystem, InstanceKey, EYcQuestEffectTrigger::ObjectiveProgressChanged);
}

void UYcQuestObjective::ActivateInitialChildren(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey)
{
    for (UYcQuestObjective* Child : ChildObjectives)
    {
        if (Child && Child->GetObjectiveState() == EYcQuestObjectiveState::Inactive)
        {
            Child->ActivateObjective(QuestSubsystem, InstanceKey);
        }
    }
}

void UYcQuestObjective::HandleChildrenQuestEvent(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const FYcQuestEvent& Event)
{
    for (UYcQuestObjective* Child : ChildObjectives)
    {
        if (Child)
        {
            Child->HandleQuestEvent(QuestSubsystem, InstanceKey, Event);
        }
    }
}

void UYcQuestObjective::TickChildren(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const float DeltaSeconds)
{
    for (UYcQuestObjective* Child : ChildObjectives)
    {
        if (Child)
        {
            Child->TickObjective(QuestSubsystem, InstanceKey, DeltaSeconds);
        }
    }
}

void UYcQuestObjective::RefreshCompletionFromChildren(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey)
{
    bool bHasRequiredChildren = false;
    bool bAllRequiredCompleted = true;

    for (UYcQuestObjective* Child : ChildObjectives)
    {
        if (!Child || Child->bOptional)
        {
            continue;
        }

        bHasRequiredChildren = true;
        if (Child->GetObjectiveState() == EYcQuestObjectiveState::Failed)
        {
            FailObjective(QuestSubsystem, InstanceKey);
            return;
        }

        if (Child->GetObjectiveState() != EYcQuestObjectiveState::Completed)
        {
            bAllRequiredCompleted = false;
        }
    }

    if (bHasRequiredChildren && bAllRequiredCompleted)
    {
        CompleteObjective(QuestSubsystem, InstanceKey);
    }
}

void UYcQuestObjective::DispatchEffects(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const EYcQuestEffectTrigger Trigger) const
{
    for (UYcQuestEffect* Effect : Effects)
    {
        if (Effect)
        {
            Effect->ExecuteEffect(QuestSubsystem, InstanceKey, Trigger, ObjectiveId, RuntimePublicProgress);
        }
    }
}

void UYcQuestObjective::OnActivated_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey)
{
    (void)QuestSubsystem;
    (void)InstanceKey;
}

void UYcQuestObjective::OnDeactivated_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey)
{
    (void)QuestSubsystem;
    (void)InstanceKey;
}

void UYcQuestObjective::OnQuestEvent_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const FYcQuestEvent& Event)
{
    (void)QuestSubsystem;
    (void)InstanceKey;
    (void)Event;
}

void UYcQuestObjective::OnObjectiveTick_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const float DeltaSeconds)
{
    (void)QuestSubsystem;
    (void)InstanceKey;
    (void)DeltaSeconds;
}

void UYcQuestObjective::OnCompleted_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey)
{
    (void)QuestSubsystem;
    (void)InstanceKey;
}

void UYcQuestObjective::OnFailed_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey)
{
    (void)QuestSubsystem;
    (void)InstanceKey;
}

FInstancedStruct UYcQuestObjective::ExportCustomSnapshot_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey) const
{
    (void)QuestSubsystem;
    (void)InstanceKey;
    return FInstancedStruct();
}

void UYcQuestObjective::ImportCustomSnapshot_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const FInstancedStruct& Snapshot)
{
    (void)QuestSubsystem;
    (void)InstanceKey;
    (void)Snapshot;
}

void UYcQuestSequenceObjective::ActivateInitialChildren(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey)
{
    for (UYcQuestObjective* Child : ChildObjectives)
    {
        if (Child && Child->GetObjectiveState() == EYcQuestObjectiveState::Inactive)
        {
            Child->ActivateObjective(QuestSubsystem, InstanceKey);
            return;
        }
        if (Child && Child->GetObjectiveState() != EYcQuestObjectiveState::Completed && !Child->bOptional)
        {
            return;
        }
    }
}

void UYcQuestSequenceObjective::RefreshCompletionFromChildren(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey)
{
    for (UYcQuestObjective* Child : ChildObjectives)
    {
        if (!Child)
        {
            continue;
        }

        const EYcQuestObjectiveState ChildState = Child->GetObjectiveState();
        if (ChildState == EYcQuestObjectiveState::Failed && !Child->bOptional)
        {
            FailObjective(QuestSubsystem, InstanceKey);
            return;
        }

        if (ChildState == EYcQuestObjectiveState::Completed || (Child->bOptional && ChildState == EYcQuestObjectiveState::Failed))
        {
            continue;
        }

        if (ChildState == EYcQuestObjectiveState::Inactive)
        {
            Child->ActivateObjective(QuestSubsystem, InstanceKey);
        }
        return;
    }

    CompleteObjective(QuestSubsystem, InstanceKey);
}

void UYcQuestParallelObjective::ActivateInitialChildren(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey)
{
    for (UYcQuestObjective* Child : ChildObjectives)
    {
        if (Child && Child->GetObjectiveState() == EYcQuestObjectiveState::Inactive)
        {
            Child->ActivateObjective(QuestSubsystem, InstanceKey);
        }
    }
}

void UYcQuestParallelObjective::RefreshCompletionFromChildren(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey)
{
    bool bAnyRequired = false;
    bool bAllRequiredCompleted = true;
    for (UYcQuestObjective* Child : ChildObjectives)
    {
        if (!Child || Child->bOptional)
        {
            continue;
        }

        bAnyRequired = true;
        const EYcQuestObjectiveState ChildState = Child->GetObjectiveState();
        if (ChildState == EYcQuestObjectiveState::Failed)
        {
            FailObjective(QuestSubsystem, InstanceKey);
            return;
        }
        if (ChildState != EYcQuestObjectiveState::Completed)
        {
            bAllRequiredCompleted = false;
        }
    }

    if (bAnyRequired && bAllRequiredCompleted)
    {
        CompleteObjective(QuestSubsystem, InstanceKey);
    }
}

void UYcQuestCounterObjective::OnActivated_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey)
{
    Super::OnActivated_Implementation(QuestSubsystem, InstanceKey);
    CurrentValue = 0.0f;
    SetPublicProgress(QuestSubsystem, InstanceKey, DisplayText.ToString(), CurrentValue, TargetValue, 0.0f, bVisibleInPublicProgress);
}

void UYcQuestCounterObjective::OnQuestEvent_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const FYcQuestEvent& Event)
{
    Super::OnQuestEvent_Implementation(QuestSubsystem, InstanceKey, Event);
    if (EventTag.IsValid() && Event.EventTag != EventTag)
    {
        return;
    }

    CurrentValue += bUseEventMagnitude ? static_cast<float>(Event.Magnitude) : 1.0f;
    SetPublicProgress(QuestSubsystem, InstanceKey, DisplayText.ToString(), CurrentValue, TargetValue, 0.0f, bVisibleInPublicProgress);
    if (CurrentValue >= FMath::Max(1.0f, TargetValue))
    {
        CompleteObjective(QuestSubsystem, InstanceKey);
    }
}

FInstancedStruct UYcQuestCounterObjective::ExportCustomSnapshot_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey) const
{
    FYcQuestCounterObjectiveSnapshot Snapshot;
    Snapshot.CurrentValue = CurrentValue;

    FInstancedStruct Result;
    Result.InitializeAs<FYcQuestCounterObjectiveSnapshot>(Snapshot);
    return Result;
}

void UYcQuestCounterObjective::ImportCustomSnapshot_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const FInstancedStruct& Snapshot)
{
    Super::ImportCustomSnapshot_Implementation(QuestSubsystem, InstanceKey, Snapshot);

    if (const FYcQuestCounterObjectiveSnapshot* Value = Snapshot.GetPtr<FYcQuestCounterObjectiveSnapshot>())
    {
        CurrentValue = Value->CurrentValue;
        SetPublicProgress(QuestSubsystem, InstanceKey, DisplayText.ToString(), CurrentValue, TargetValue, 0.0f, bVisibleInPublicProgress);
    }
}

void UYcQuestTimedObjective::OnActivated_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey)
{
    Super::OnActivated_Implementation(QuestSubsystem, InstanceKey);
    SetPublicProgress(QuestSubsystem, InstanceKey, DisplayText.ToString(), 0.0f, RequiredSeconds, RequiredSeconds, bVisibleInPublicProgress);
}

void UYcQuestTimedObjective::OnObjectiveTick_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const float DeltaSeconds)
{
    Super::OnObjectiveTick_Implementation(QuestSubsystem, InstanceKey, DeltaSeconds);

    const float CurrentSeconds = FMath::Min(RequiredSeconds, RuntimeActiveSeconds);
    SetPublicProgress(QuestSubsystem, InstanceKey, DisplayText.ToString(), CurrentSeconds, RequiredSeconds, FMath::Max(0.0f, RequiredSeconds - CurrentSeconds), bVisibleInPublicProgress);
    if (CurrentSeconds >= RequiredSeconds)
    {
        CompleteObjective(QuestSubsystem, InstanceKey);
    }
}

void UYcQuestHoldAreaObjective::OnActivated_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey)
{
    Super::OnActivated_Implementation(QuestSubsystem, InstanceKey);
    bHolding = false;
    HeldSeconds = 0.0f;
    SetPublicProgress(QuestSubsystem, InstanceKey, DisplayText.ToString(), HeldSeconds, RequiredSeconds, RequiredSeconds, bVisibleInPublicProgress);
}

void UYcQuestHoldAreaObjective::OnQuestEvent_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const FYcQuestEvent& Event)
{
    Super::OnQuestEvent_Implementation(QuestSubsystem, InstanceKey, Event);

    if (HoldStartedTag.IsValid() && Event.EventTag == HoldStartedTag)
    {
        bHolding = true;
        return;
    }

    if (HoldEndedTag.IsValid() && Event.EventTag == HoldEndedTag)
    {
        bHolding = false;
        if (bResetProgressOnExit)
        {
            HeldSeconds = 0.0f;
            SetPublicProgress(QuestSubsystem, InstanceKey, DisplayText.ToString(), HeldSeconds, RequiredSeconds, RequiredSeconds, bVisibleInPublicProgress);
        }
    }
}

void UYcQuestHoldAreaObjective::OnObjectiveTick_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const float DeltaSeconds)
{
    Super::OnObjectiveTick_Implementation(QuestSubsystem, InstanceKey, DeltaSeconds);
    if (!bHolding)
    {
        return;
    }

    HeldSeconds = FMath::Min(RequiredSeconds, HeldSeconds + FMath::Max(0.0f, DeltaSeconds));
    SetPublicProgress(QuestSubsystem, InstanceKey, DisplayText.ToString(), HeldSeconds, RequiredSeconds, FMath::Max(0.0f, RequiredSeconds - HeldSeconds), bVisibleInPublicProgress);
    if (HeldSeconds >= RequiredSeconds)
    {
        CompleteObjective(QuestSubsystem, InstanceKey);
    }
}

FInstancedStruct UYcQuestHoldAreaObjective::ExportCustomSnapshot_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey) const
{
    FYcQuestHoldAreaObjectiveSnapshot Snapshot;
    Snapshot.bHolding = bHolding;
    Snapshot.HeldSeconds = HeldSeconds;

    FInstancedStruct Result;
    Result.InitializeAs<FYcQuestHoldAreaObjectiveSnapshot>(Snapshot);
    return Result;
}

void UYcQuestHoldAreaObjective::ImportCustomSnapshot_Implementation(UYcQuestSubsystem* QuestSubsystem, const FYcQuestInstanceKey& InstanceKey, const FInstancedStruct& Snapshot)
{
    Super::ImportCustomSnapshot_Implementation(QuestSubsystem, InstanceKey, Snapshot);

    if (const FYcQuestHoldAreaObjectiveSnapshot* Value = Snapshot.GetPtr<FYcQuestHoldAreaObjectiveSnapshot>())
    {
        bHolding = Value->bHolding;
        HeldSeconds = Value->HeldSeconds;
        SetPublicProgress(QuestSubsystem, InstanceKey, DisplayText.ToString(), HeldSeconds, RequiredSeconds, FMath::Max(0.0f, RequiredSeconds - HeldSeconds), bVisibleInPublicProgress);
    }
}
