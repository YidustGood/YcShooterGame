// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcInventoryOperationRouterComponent.h"

#include "NativeGameplayTags.h"
#include "YcInventoryItemInstance.h"
#include "YcInventoryManagerComponent.h"
#include "YiChenInventory.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcInventoryOperationRouterComponent)

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Yc_Inventory_Message_Operation_StateChanged, "Yc.Inventory.Message.Operation.StateChanged");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Yc_Inventory_Message_ProjectedState_Changed, "Yc.Inventory.Message.ProjectedStateChanged");

namespace
{
	bool MatchesPrefix(const FName OpType, const FName Prefix)
	{
		return OpType.ToString().StartsWith(Prefix.ToString(), ESearchCase::CaseSensitive);
	}

	AController* ResolveOwningController(const AActor* Actor)
	{
		if (!Actor)
		{
			return nullptr;
		}

		if (AController* Controller = const_cast<AController*>(Cast<AController>(Actor)))
		{
			return Controller;
		}

		if (const APawn* Pawn = Cast<APawn>(Actor))
		{
			return Pawn->GetController();
		}

		if (const APlayerState* PlayerState = Cast<APlayerState>(Actor))
		{
			if (AController* OwnerController = Cast<AController>(PlayerState->GetOwner()))
			{
				return OwnerController;
			}
			if (const APawn* Pawn = PlayerState->GetPawn())
			{
				return Pawn->GetController();
			}
		}

		return nullptr;
	}
}

UYcInventoryOperationRouterComponent::UYcInventoryOperationRouterComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	ProjectedOperationRevision = AuthoritativeOperationRevision;
	RegisterDefaultOperationHandlers();
}

UYcInventoryOperationRouterComponent* UYcInventoryOperationRouterComponent::FindRouter(const AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	if (AController* Controller = ResolveOwningController(Actor))
	{
		return Controller->FindComponentByClass<UYcInventoryOperationRouterComponent>();
	}
	return nullptr;
}

UYcInventoryOperationRouterComponent* UYcInventoryOperationRouterComponent::FindOrCreateRouter(AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	if (UYcInventoryOperationRouterComponent* Existing = FindRouter(Actor))
	{
		return Existing;
	}

	AActor* AttachActor = nullptr;
	if (AController* Controller = ResolveOwningController(Actor))
	{
		AttachActor = Controller;
	}
	else
	{
		// 统一规则：Router 只允许挂在 PlayerController 上。
		return nullptr;
	}

	UYcInventoryOperationRouterComponent* Router = NewObject<UYcInventoryOperationRouterComponent>(AttachActor, UYcInventoryOperationRouterComponent::StaticClass(), TEXT("YcInventoryOperationRouter"));
	if (!Router)
	{
		return nullptr;
	}

	Router->SetNetAddressable();
	Router->SetIsReplicated(true);
	AttachActor->AddInstanceComponent(Router);
	Router->RegisterComponent();
	return Router;
}

int64 UYcInventoryOperationRouterComponent::SubmitInventoryOperation(UYcInventoryManagerComponent* CallingInventory, FYcInventoryOperation InOperation, const bool bSendToServer)
{
	if (!IsValid(CallingInventory) || !IsValid(CallingInventory->GetOwner()))
	{
		return INDEX_NONE;
	}

	if (InOperation.OpId <= 0)
	{
		InOperation.OpId = NextLocalOperationId++;
	}

	InOperation.RequestActor = InOperation.RequestActor.Get() ? InOperation.RequestActor.Get() : CallingInventory->GetOwner();
	InOperation.RequestInventory = InOperation.RequestInventory.Get() ? InOperation.RequestInventory.Get() : CallingInventory;
	InOperation.SourceInventory = InOperation.SourceInventory.Get() ? InOperation.SourceInventory.Get() : CallingInventory;
	if (InOperation.BaseRevision < 0)
	{
		InOperation.BaseRevision = AuthoritativeOperationRevision;
	}
	if (InOperation.ClientTimestamp <= 0.0)
	{
		InOperation.ClientTimestamp = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	}

	PendingOperationOrder.Remove(InOperation.OpId);
	PendingOperations.Add(InOperation.OpId, InOperation);
	PendingOperationOrder.Add(InOperation.OpId);
	ProjectedOperationRevision = AuthoritativeOperationRevision + PendingOperations.Num();
	LogOperationTrace(TEXT("Submit"), InOperation, TEXT("Submitted"));
	RefreshProjectedStateFromAuthoritative(InOperation);
	BroadcastOperationStateMessage(EYcInventoryOperationEvent::Submitted, InOperation, TEXT("Submitted"));
	BroadcastProjectedStateChanged(EYcInventoryOperationEvent::Submitted, InOperation);

	if (CallingInventory->GetOwner()->HasAuthority())
	{
		int32 UnusedRevision = 0;
		FString UnusedDetail;
		HandleServerSubmit(InOperation, UnusedRevision, UnusedDetail);
	}
	else if (bSendToServer)
	{
		ServerSubmitInventoryOperation(InOperation);
	}

	return InOperation.OpId;
}

void UYcInventoryOperationRouterComponent::ServerSubmitInventoryOperation_Implementation(const FYcInventoryOperation& InOperation)
{
	int32 NewRevision = 0;
	FString Detail;
	switch (HandleServerSubmit(InOperation, NewRevision, Detail))
	{
	case EYcInventoryOperationProcessResult::Succeeded:
		ClientReceiveInventoryOperationAck(InOperation.OpId, NewRevision, Detail);
		break;
	case EYcInventoryOperationProcessResult::Failed:
		ClientReceiveInventoryOperationNack(InOperation.OpId, NewRevision, Detail);
		break;
	case EYcInventoryOperationProcessResult::Deferred:
		break;
	default:
		ClientReceiveInventoryOperationNack(InOperation.OpId, NewRevision, TEXT("Unknown operation process result."));
		break;
	}
}

EYcInventoryOperationProcessResult UYcInventoryOperationRouterComponent::HandleServerSubmit(const FYcInventoryOperation& InOperation, int32& OutRevision, FString& OutDetail)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		OutRevision = AuthoritativeOperationRevision;
		OutDetail = TEXT("ServerSubmit must run on authority.");
		return EYcInventoryOperationProcessResult::Failed;
	}

	FString Reason;
	FString DeltaSummary;
	switch (ExecuteOperationOnServer(InOperation, Reason, DeltaSummary))
	{
	case EYcInventoryOperationProcessResult::Succeeded:
		++AuthoritativeOperationRevision;
		NotifyInventoryOperationAck(InOperation.OpId, AuthoritativeOperationRevision, DeltaSummary.IsEmpty() ? TEXT("Ack") : DeltaSummary);
		OutRevision = AuthoritativeOperationRevision;
		OutDetail = DeltaSummary.IsEmpty() ? TEXT("Ack") : DeltaSummary;
		return EYcInventoryOperationProcessResult::Succeeded;
	case EYcInventoryOperationProcessResult::Failed:
		NotifyInventoryOperationNack(InOperation.OpId, AuthoritativeOperationRevision, Reason);
		OutRevision = AuthoritativeOperationRevision;
		OutDetail = Reason;
		return EYcInventoryOperationProcessResult::Failed;
	case EYcInventoryOperationProcessResult::Deferred:
		OutRevision = AuthoritativeOperationRevision;
		OutDetail = TEXT("");
		return EYcInventoryOperationProcessResult::Deferred;
	default:
		NotifyInventoryOperationNack(InOperation.OpId, AuthoritativeOperationRevision, TEXT("Unknown operation process result."));
		OutRevision = AuthoritativeOperationRevision;
		OutDetail = TEXT("Unknown operation process result.");
		return EYcInventoryOperationProcessResult::Failed;
	}
	return EYcInventoryOperationProcessResult::Failed;
}

void UYcInventoryOperationRouterComponent::ClientReceiveInventoryOperationAck_Implementation(const int64 OpId, const int32 NewRevision, const FString& DeltaSummary)
{
	HandleClientReceiveAck(OpId, NewRevision, DeltaSummary);
}

void UYcInventoryOperationRouterComponent::ClientReceiveInventoryOperationNack_Implementation(const int64 OpId, const int32 NewRevision, const FString& Reason)
{
	HandleClientReceiveNack(OpId, NewRevision, Reason);
}

void UYcInventoryOperationRouterComponent::NotifyInventoryOperationAck(const int64 OpId, const int32 NewRevision, const FString& DeltaSummary)
{
	FYcInventoryOperation OpCopy;
	const bool bHadPending = PendingOperations.RemoveAndCopyValue(OpId, OpCopy);
	PendingOperationOrder.Remove(OpId);
	AuthoritativeOperationRevision = FMath::Max(AuthoritativeOperationRevision, NewRevision);
	ProjectedOperationRevision = AuthoritativeOperationRevision + PendingOperations.Num();

	if (bHadPending)
	{
		FYcInventoryOperationDelta Delta;
		Delta.Summary = DeltaSummary;
		BroadcastOperationStateMessage(EYcInventoryOperationEvent::Acked, OpCopy, DeltaSummary, &Delta);
		BroadcastProjectedStateChanged(EYcInventoryOperationEvent::Acked, OpCopy);
		LogOperationTrace(TEXT("Ack"), OpCopy, DeltaSummary);
	}
}

void UYcInventoryOperationRouterComponent::NotifyInventoryOperationNack(const int64 OpId, const int32 NewRevision, const FString& Reason)
{
	FYcInventoryOperation OpCopy;
	const bool bHadPending = PendingOperations.RemoveAndCopyValue(OpId, OpCopy);
	PendingOperationOrder.Remove(OpId);
	AuthoritativeOperationRevision = FMath::Max(AuthoritativeOperationRevision, NewRevision);
	ProjectedOperationRevision = AuthoritativeOperationRevision + PendingOperations.Num();

	if (bHadPending)
	{
		FYcInventoryOperationDelta Delta;
		Delta.Summary = Reason;
		BroadcastOperationStateMessage(EYcInventoryOperationEvent::Nacked, OpCopy, Reason, &Delta);
		LogOperationTrace(TEXT("Nack"), OpCopy, Reason);
		RebuildProjectedStateFromPending(OpId);
		BroadcastProjectedStateChanged(EYcInventoryOperationEvent::Nacked, OpCopy);
	}
}

void UYcInventoryOperationRouterComponent::HandleClientReceiveAck(const int64 OpId, const int32 NewRevision, const FString& DeltaSummary)
{
	if (FYcInventoryOperation* PendingOp = PendingOperations.Find(OpId))
	{
		AuthoritativeOperationRevision = FMath::Max(AuthoritativeOperationRevision, NewRevision);
		ProjectedOperationRevision = AuthoritativeOperationRevision;
		for (const int64 PendingId : PendingOperationOrder)
		{
			if (PendingId != OpId && PendingOperations.Contains(PendingId))
			{
				++ProjectedOperationRevision;
			}
		}

		const FYcInventoryOperation OpCopy = *PendingOp;
		PendingOperations.Remove(OpId);
		PendingOperationOrder.Remove(OpId);

		FYcInventoryOperationDelta Delta;
		Delta.Summary = DeltaSummary;
		RefreshProjectedStateFromAuthoritative(OpCopy);
		BroadcastOperationStateMessage(EYcInventoryOperationEvent::Acked, OpCopy, DeltaSummary, &Delta);
		BroadcastProjectedStateChanged(EYcInventoryOperationEvent::Acked, OpCopy);
		LogOperationTrace(TEXT("Ack"), OpCopy, DeltaSummary);
	}
}

void UYcInventoryOperationRouterComponent::HandleClientReceiveNack(const int64 OpId, const int32 NewRevision, const FString& Reason)
{
	if (FYcInventoryOperation* PendingOp = PendingOperations.Find(OpId))
	{
		AuthoritativeOperationRevision = FMath::Max(AuthoritativeOperationRevision, NewRevision);

		const FYcInventoryOperation OpCopy = *PendingOp;
		PendingOperations.Remove(OpId);
		PendingOperationOrder.Remove(OpId);

		FYcInventoryOperationDelta Delta;
		Delta.Summary = Reason;
		BroadcastOperationStateMessage(EYcInventoryOperationEvent::Nacked, OpCopy, Reason, &Delta);
		LogOperationTrace(TEXT("Nack"), OpCopy, Reason);
		RebuildProjectedStateFromPending(OpId);
		BroadcastProjectedStateChanged(EYcInventoryOperationEvent::Nacked, OpCopy);
	}
}

void UYcInventoryOperationRouterComponent::RegisterDefaultOperationHandlers()
{
	RegisteredOperationHandlers.Reset();
	RegisteredHandlerOwners.Reset();
}

bool UYcInventoryOperationRouterComponent::RegisterOperationHandler(const FName& OpTypeOrPrefix, const FYcInventoryOperationHandler& Handler)
{
	if (OpTypeOrPrefix.IsNone())
	{
		return false;
	}

	RegisteredOperationHandlers.Add(OpTypeOrPrefix, Handler);
	RegisteredHandlerOwners.Remove(OpTypeOrPrefix);
	return true;
}

bool UYcInventoryOperationRouterComponent::UnregisterOperationHandler(const FName& OpTypeOrPrefix)
{
	const bool bRemoved = RegisteredOperationHandlers.Remove(OpTypeOrPrefix) > 0;
	RegisteredHandlerOwners.Remove(OpTypeOrPrefix);
	return bRemoved;
}

bool UYcInventoryOperationRouterComponent::UnregisterOperationHandlersByOwner(const UObject* HandlerOwner)
{
	if (!HandlerOwner)
	{
		return false;
	}

	TArray<FName> KeysToRemove;
	for (const TPair<FName, TWeakObjectPtr<const UObject>>& Pair : RegisteredHandlerOwners)
	{
		if (Pair.Value.Get() == HandlerOwner)
		{
			KeysToRemove.Add(Pair.Key);
		}
	}

	for (const FName& Key : KeysToRemove)
	{
		RegisteredOperationHandlers.Remove(Key);
		RegisteredHandlerOwners.Remove(Key);
	}
	return KeysToRemove.Num() > 0;
}

bool UYcInventoryOperationRouterComponent::RegisterScriptOperationHandler(const FName& OpTypeOrPrefix, UObject* HandlerObject, FName ValidateFunctionName, FName ExecuteFunctionName, FName BuildDeltaFunctionName, const bool bPrefixMatch, const int32 Priority)
{
	if (OpTypeOrPrefix.IsNone() || !IsValid(HandlerObject) || ExecuteFunctionName.IsNone())
	{
		return false;
	}

	const TWeakObjectPtr<UObject> WeakHandlerObject(HandlerObject);
	FYcInventoryOperationHandler Handler;
	Handler.bPrefixMatch = bPrefixMatch;
	Handler.Priority = Priority;
	Handler.Validate.BindLambda([WeakHandlerObject, ValidateFunctionName](const FYcInventoryOperation& Op, FString& OutReason)
	{
		UObject* TargetObject = WeakHandlerObject.Get();
		if (!IsValid(TargetObject))
		{
			OutReason = TEXT("Script handler object missing.");
			return false;
		}

		if (ValidateFunctionName.IsNone())
		{
			return true;
		}

		UFunction* ValidateFn = TargetObject->FindFunction(ValidateFunctionName);
		if (!ValidateFn)
		{
			OutReason = FString::Printf(TEXT("Validate function not found: %s"), *ValidateFunctionName.ToString());
			return false;
		}

		struct FValidateParams
		{
			FYcInventoryOperation Operation;
			FString OutReason;
			bool ReturnValue;
		};

		FValidateParams Params;
		Params.Operation = Op;
		Params.OutReason = TEXT("");
		Params.ReturnValue = false;
		TargetObject->ProcessEvent(ValidateFn, &Params);
		if (!Params.OutReason.IsEmpty())
		{
			OutReason = Params.OutReason;
		}
		return Params.ReturnValue;
	});
	Handler.Execute.BindLambda([WeakHandlerObject, ExecuteFunctionName](const FYcInventoryOperation& Op, FString& OutReason)
	{
		UObject* TargetObject = WeakHandlerObject.Get();
		if (!IsValid(TargetObject))
		{
			OutReason = TEXT("Script handler object missing.");
			return false;
		}

		UFunction* ExecuteFn = TargetObject->FindFunction(ExecuteFunctionName);
		if (!ExecuteFn)
		{
			OutReason = FString::Printf(TEXT("Execute function not found: %s"), *ExecuteFunctionName.ToString());
			return false;
		}

		struct FExecuteParams
		{
			FYcInventoryOperation Operation;
			FString OutReason;
			bool ReturnValue;
		};

		FExecuteParams Params;
		Params.Operation = Op;
		Params.OutReason = TEXT("");
		Params.ReturnValue = false;
		TargetObject->ProcessEvent(ExecuteFn, &Params);
		if (!Params.OutReason.IsEmpty())
		{
			OutReason = Params.OutReason;
		}
		return Params.ReturnValue;
	});
	Handler.BuildDelta.BindLambda([WeakHandlerObject, BuildDeltaFunctionName](const FYcInventoryOperation& Op, bool bSuccess, FYcInventoryOperationDelta& OutDelta)
	{
		if (BuildDeltaFunctionName.IsNone())
		{
			return;
		}

		UObject* TargetObject = WeakHandlerObject.Get();
		if (!IsValid(TargetObject))
		{
			return;
		}

		UFunction* BuildDeltaFn = TargetObject->FindFunction(BuildDeltaFunctionName);
		if (!BuildDeltaFn)
		{
			return;
		}

		struct FBuildDeltaParams
		{
			FYcInventoryOperation Operation;
			bool bSuccess;
			FYcInventoryOperationDelta OutDelta;
		};

		FBuildDeltaParams Params;
		Params.Operation = Op;
		Params.bSuccess = bSuccess;
		Params.OutDelta = OutDelta;
		TargetObject->ProcessEvent(BuildDeltaFn, &Params);
		OutDelta = Params.OutDelta;
	});

	// 预校验：防止注册成功但运行时才发现函数名错误。
	{
		UObject* TargetObject = WeakHandlerObject.Get();
		if (!TargetObject)
		{
			return false;
		}
		if (!ValidateFunctionName.IsNone() && !TargetObject->FindFunction(ValidateFunctionName))
		{
			return false;
		}
		if (!TargetObject->FindFunction(ExecuteFunctionName))
		{
			return false;
		}
		if (!BuildDeltaFunctionName.IsNone() && !TargetObject->FindFunction(BuildDeltaFunctionName))
		{
			return false;
		}
	}

	RegisteredOperationHandlers.Add(OpTypeOrPrefix, Handler);
	RegisteredHandlerOwners.Add(OpTypeOrPrefix, HandlerObject);
	return true;
}

bool UYcInventoryOperationRouterComponent::UnregisterScriptOperationHandler(const FName& OpTypeOrPrefix, UObject* HandlerObject)
{
	if (OpTypeOrPrefix.IsNone())
	{
		return false;
	}

	if (!HandlerObject)
	{
		return UnregisterOperationHandler(OpTypeOrPrefix);
	}

	if (const TWeakObjectPtr<const UObject>* OwnerPtr = RegisteredHandlerOwners.Find(OpTypeOrPrefix))
	{
		if (OwnerPtr->Get() != HandlerObject)
		{
			return false;
		}
	}

	return UnregisterOperationHandler(OpTypeOrPrefix);
}

const FYcInventoryOperationHandler* UYcInventoryOperationRouterComponent::ResolveOperationHandler(const FName& OpType) const
{
	if (const FYcInventoryOperationHandler* Exact = RegisteredOperationHandlers.Find(OpType))
	{
		if (Exact->bEnabled)
		{
			return Exact;
		}
	}

	const FYcInventoryOperationHandler* BestPrefix = nullptr;
	for (const TPair<FName, FYcInventoryOperationHandler>& Pair : RegisteredOperationHandlers)
	{
		const FYcInventoryOperationHandler& Candidate = Pair.Value;
		if (!Candidate.bEnabled || !Candidate.bPrefixMatch)
		{
			continue;
		}

		if (!MatchesPrefix(OpType, Pair.Key))
		{
			continue;
		}

		if (BestPrefix == nullptr || Candidate.Priority > BestPrefix->Priority)
		{
			BestPrefix = &Candidate;
		}
	}

	return BestPrefix;
}

bool UYcInventoryOperationRouterComponent::ValidateOperationWithHandler(const FYcInventoryOperation& InOperation, const FYcInventoryOperationHandler& Handler, FString& OutReason) const
{
	if (Handler.Validate.IsBound())
	{
		return Handler.Validate.Execute(InOperation, OutReason);
	}
	return true;
}

bool UYcInventoryOperationRouterComponent::ExecuteOperationWithHandler(const FYcInventoryOperation& InOperation, const FYcInventoryOperationHandler& Handler, FString& OutReason, FString& OutDeltaSummary)
{
	const bool bSuccess = Handler.Execute.IsBound() ? Handler.Execute.Execute(InOperation, OutReason) : false;
	FYcInventoryOperationDelta Delta;
	if (Handler.BuildDelta.IsBound())
	{
		Handler.BuildDelta.Execute(InOperation, bSuccess, Delta);
	}
	if (!Delta.Summary.IsEmpty())
	{
		OutDeltaSummary = Delta.Summary;
	}
	else if (bSuccess && !OutReason.IsEmpty())
	{
		// 允许执行器把成功摘要写到 OutReason，Router 会透传给 DeltaSummary。
		OutDeltaSummary = OutReason;
	}
	return bSuccess;
}

EYcInventoryOperationProcessResult UYcInventoryOperationRouterComponent::ExecuteOperationOnServer(const FYcInventoryOperation& InOperation, FString& OutReason, FString& OutDeltaSummary)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		OutReason = TEXT("Must execute on authority.");
		return EYcInventoryOperationProcessResult::Failed;
	}

	if (InOperation.OpType.IsNone())
	{
		OutReason = TEXT("OpType is none.");
		return EYcInventoryOperationProcessResult::Failed;
	}

	const FYcInventoryOperationHandler* Handler = ResolveOperationHandler(InOperation.OpType);
	if (!Handler)
	{
		OutReason = FString::Printf(TEXT("Unsupported OpType: %s"), *InOperation.OpType.ToString());
		return EYcInventoryOperationProcessResult::Failed;
	}

	if (!ValidateOperationWithHandler(InOperation, *Handler, OutReason))
	{
		return EYcInventoryOperationProcessResult::Failed;
	}

	if (!ExecuteOperationWithHandler(InOperation, *Handler, OutReason, OutDeltaSummary))
	{
		return EYcInventoryOperationProcessResult::Failed;
	}

	return EYcInventoryOperationProcessResult::Succeeded;
}

void UYcInventoryOperationRouterComponent::RebuildProjectedStateFromPending(int64 NackedOpId)
{
	ProjectedState = FYcInventoryProjectedState();
	ProjectedOperationRevision = AuthoritativeOperationRevision;
	for (const int64 PendingId : PendingOperationOrder)
	{
		if (FYcInventoryOperation* PendingOp = PendingOperations.Find(PendingId))
		{
			++ProjectedOperationRevision;
			RefreshProjectedStateFromAuthoritative(*PendingOp);
			const FString Detail = FString::Printf(TEXT("ReprojectedAfterNack(%lld)"), NackedOpId);
			BroadcastOperationStateMessage(EYcInventoryOperationEvent::Submitted, *PendingOp, Detail);
			BroadcastProjectedStateChanged(EYcInventoryOperationEvent::Submitted, *PendingOp);
		}
	}
}

void UYcInventoryOperationRouterComponent::RefreshProjectedStateFromAuthoritative(const FYcInventoryOperation& Operation)
{
	if (Operation.TargetInventory)
	{
		int32& Revision = ProjectedState.InventoryGridRevision.FindOrAdd(Operation.TargetInventory->GetFName());
		++Revision;
	}
	if (const FYcInventoryOperationHandler* Handler = ResolveOperationHandler(Operation.OpType))
	{
		if (Handler->ProjectState.IsBound())
		{
			Handler->ProjectState.Execute(Operation, ProjectedState);
		}
	}
}

void UYcInventoryOperationRouterComponent::BroadcastOperationStateMessage(const EYcInventoryOperationEvent Event, const FYcInventoryOperation& Operation, const FString& Detail, const FYcInventoryOperationDelta* Delta)
{
	if (!GetWorld())
	{
		return;
	}

	FYcInventoryOperationStateMessage Message;
	Message.Owner = GetOwner();
	Message.Event = Event;
	Message.Operation = Operation;
	Message.AuthoritativeRevision = AuthoritativeOperationRevision;
	Message.ProjectedRevision = ProjectedOperationRevision;
	Message.PendingCount = PendingOperations.Num();
	Message.Detail = Detail;
	if (Delta)
	{
		Message.Delta = *Delta;
	}

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
	MessageSystem.BroadcastMessage(TAG_Yc_Inventory_Message_Operation_StateChanged, Message);
}

void UYcInventoryOperationRouterComponent::BroadcastProjectedStateChanged(const EYcInventoryOperationEvent Event, const FYcInventoryOperation& Operation)
{
	if (!GetWorld())
	{
		return;
	}

	FYcInventoryProjectedStateChangedMessage Message;
	Message.Owner = GetOwner();
	Message.AuthoritativeRevision = AuthoritativeOperationRevision;
	Message.ProjectedRevision = ProjectedOperationRevision;
	Message.PendingCount = PendingOperations.Num();
	Message.ProjectedState = ProjectedState;
	Message.Operation = Operation;
	Message.Event = Event;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(GetWorld());
	MessageSystem.BroadcastMessage(TAG_Yc_Inventory_Message_ProjectedState_Changed, Message);
}

void UYcInventoryOperationRouterComponent::LogOperationTrace(const TCHAR* Stage, const FYcInventoryOperation& Operation, const FString& Detail) const
{
	const FString ItemInstId = Operation.ItemInstance ? Operation.ItemInstance->GetItemInstId().ToString() : TEXT("None");
	const FString SourceInventoryName = GetNameSafe(Operation.SourceInventory.Get());
	const FString TargetInventoryName = GetNameSafe(Operation.TargetInventory.Get());
	const FString RequestActorName = GetNameSafe(Operation.RequestActor.Get());

	UE_LOG(LogYcInventory, Log,
		TEXT("[OpTrace] stage=%s opId=%lld opType=%s item=%s source=%s target=%s requester=%s baseRev=%d authRev=%d projectedRev=%d pending=%d detail=%s"),
		Stage,
		Operation.OpId,
		*Operation.OpType.ToString(),
		*ItemInstId,
		*SourceInventoryName,
		*TargetInventoryName,
		*RequestActorName,
		Operation.BaseRevision,
		AuthoritativeOperationRevision,
		ProjectedOperationRevision,
		PendingOperations.Num(),
		*Detail);
}

FString UYcInventoryOperationRouterComponent::GetOperationStateSnapshot() const
{
	TArray<int64> PendingIds = PendingOperationOrder;
	PendingIds.RemoveAll([this](const int64 OpId)
	{
		return !PendingOperations.Contains(OpId);
	});
	PendingIds.Sort();

	FString PendingText;
	for (int32 i = 0; i < PendingIds.Num(); ++i)
	{
		const int64 OpId = PendingIds[i];
		const FYcInventoryOperation* Op = PendingOperations.Find(OpId);
		const FString OpTypeText = Op ? Op->OpType.ToString() : TEXT("Missing");
		PendingText += FString::Printf(TEXT("%lld:%s"), OpId, *OpTypeText);
		if (i < PendingIds.Num() - 1)
		{
			PendingText += TEXT(", ");
		}
	}
	if (PendingText.IsEmpty())
	{
		PendingText = TEXT("None");
	}

	return FString::Printf(
		TEXT("RouterOwner=%s | AuthoritativeRevision=%d | ProjectedRevision=%d | PendingCount=%d | Pending=[%s]"),
		*GetNameSafe(GetOwner()),
		AuthoritativeOperationRevision,
		ProjectedOperationRevision,
		PendingOperations.Num(),
		*PendingText);
}

void UYcInventoryOperationRouterComponent::LogOperationStateSnapshot() const
{
	UE_LOG(LogYcInventory, Log, TEXT("[OpSnapshot] %s"), *GetOperationStateSnapshot());
}



