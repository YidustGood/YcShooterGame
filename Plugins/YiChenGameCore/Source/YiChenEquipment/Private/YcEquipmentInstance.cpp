// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcEquipmentInstance.h"

#include "YcEquipmentActorComponent.h"
#include "YcInventoryItemInstance.h"
#include "YiChenEquipment.h"
#include "Fragments/InventoryFragment_Equippable.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcEquipmentInstance)

// ============================================================================
// 构造函数和生命周期
// ============================================================================

UYcEquipmentInstance::UYcEquipmentInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, EquipmentState(EYcEquipmentState::Unequipped)
	, EquipmentDef(nullptr)
	, bActorsSpawned(false)
	, bAutoShowEquipmentActors(true)
	, bAutoHideEquipmentActors(false)
{
}

void UYcEquipmentInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION(UYcEquipmentInstance, EquipmentState, COND_SkipOwner);
	DOREPLIFETIME(UYcEquipmentInstance, SpawnedActors);
}

// ============================================================================
// 基础查询接口
// ============================================================================

UYcInventoryItemInstance* UYcEquipmentInstance::GetAssociatedItem() const
{
	return Instigator;
}

APawn* UYcEquipmentInstance::GetPawn() const
{
	// EquipmentManagerComponent可能挂载到Pawn、PlayerState或PlayerController上
	if (APawn* Pawn = Cast<APawn>(GetOuter()))
	{
		return Pawn;
	}
	
	if (const APlayerState* PlayerState = Cast<APlayerState>(GetOuter()))
	{
		return PlayerState->GetPawn();
	}
	
	if (const APlayerController* PlayerController = Cast<APlayerController>(GetOuter()))
	{
		return PlayerController->GetPawn();
	}
	
	return nullptr;
}

APawn* UYcEquipmentInstance::GetTypedPawn(TSubclassOf<APawn> PawnType) const
{
	APawn* Result = nullptr;
	if (UClass* ActualPawnType = PawnType)
	{
		if (GetOuter()->IsA(ActualPawnType))
		{
			Result = Cast<APawn>(GetOuter());
		}
	}
	return Result;
}

const FYcEquipmentDefinition* UYcEquipmentInstance::GetEquipmentDef()
{
	if (EquipmentDef) return EquipmentDef;
	UpdateEquipmentDef();
	return EquipmentDef;
}

bool UYcEquipmentInstance::K2_GetEquipmentDef(FYcEquipmentDefinition& OutEquipmentDef)
{
	if (!GetEquipmentDef()) return false;
	OutEquipmentDef = *EquipmentDef;
	return true;
}

TInstancedStruct<FYcEquipmentFragment> UYcEquipmentInstance::FindEquipmentFragment(const UScriptStruct* FragmentStructType)
{
	if (!GetEquipmentDef() || !FragmentStructType)
	{
		return TInstancedStruct<FYcEquipmentFragment>();
	}
	
	for (const auto& Fragment : EquipmentDef->Fragments)
	{
		if (Fragment.GetScriptStruct() == FragmentStructType)
		{
			return Fragment;
		}
	}
	
	return TInstancedStruct<FYcEquipmentFragment>();
}

void UYcEquipmentInstance::OnEquipmentInstanceCreated(const FYcEquipmentDefinition& Definition)
{
	// 遍历所有 Fragment 并调用 OnEquipmentInstanceCreated
	for (const TInstancedStruct<FYcEquipmentFragment>& Fragment : Definition.Fragments)
	{
		if (const FYcEquipmentFragment* FragmentPtr = Fragment.GetPtr<FYcEquipmentFragment>())
		{
			FragmentPtr->OnEquipmentInstanceCreated(this);
		}
	}
	
	// 调用蓝图版本
	K2_OnEquipmentInstanceCreated();
}

// ============================================================================
// 状态管理
// ============================================================================

void UYcEquipmentInstance::SetEquipmentState(EYcEquipmentState NewState)
{
	if (EquipmentState == NewState) return;
	
	const EYcEquipmentState OldState = EquipmentState;
	EquipmentState = NewState;
	
	// 服务器端/主控客户端直接处理状态变化
	if (GetPawn() && (GetPawn()->HasAuthority() || GetPawn()->GetLocalRole() == ROLE_AutonomousProxy))
	{
		OnRep_EquipmentState(OldState);
	}
}

void UYcEquipmentInstance::OnRep_EquipmentState(EYcEquipmentState OldState)
{
	if (OldState == EquipmentState) return;
	
	UE_LOG(LogYcEquipment, Verbose, TEXT("OnRep_EquipmentState: %s -> %s"), 
		*UEnum::GetValueAsString(OldState), *UEnum::GetValueAsString(EquipmentState));
	
	switch (EquipmentState)
	{
	case EYcEquipmentState::Equipped:
		OnEquipped();
		break;
		
	case EYcEquipmentState::Unequipped:
		OnUnequipped();
		break;
	}
}

void UYcEquipmentInstance::OnEquipped()
{
	UE_LOG(LogYcEquipment, Verbose, TEXT("OnEquipped: %s (AutoShow=%d)"), *GetNameSafe(this), bAutoShowEquipmentActors);
	
	// 遍历所有 Fragment 并调用 OnEquipped
	if (const FYcEquipmentDefinition* EquipDef = GetEquipmentDef())
	{
		for (const TInstancedStruct<FYcEquipmentFragment>& Fragment : EquipDef->Fragments)
		{
			if (const FYcEquipmentFragment* FragmentPtr = Fragment.GetPtr<FYcEquipmentFragment>())
			{
				FragmentPtr->OnEquipped(this);
			}
		}
	}
	
	// 根据配置决定是否自动显示Actors
	// 如果设为false，需要在 K2_OnEquipped 蓝图中播放装备动画后手动调用 ShowEquipmentActors()
	if (bAutoShowEquipmentActors)
	{
		ShowEquipmentActors();
	}
	
	// 调用蓝图事件
	K2_OnEquipped();
}

void UYcEquipmentInstance::OnUnequipped()
{
	UE_LOG(LogYcEquipment, Verbose, TEXT("OnUnequipped: %s (AutoHide=%d)"), *GetNameSafe(this), bAutoHideEquipmentActors);
	
	// 遍历所有 Fragment 并调用 OnUnequipped
	if (const FYcEquipmentDefinition* EquipDef = GetEquipmentDef())
	{
		for (const TInstancedStruct<FYcEquipmentFragment>& Fragment : EquipDef->Fragments)
		{
			if (const FYcEquipmentFragment* FragmentPtr = Fragment.GetPtr<FYcEquipmentFragment>())
			{
				FragmentPtr->OnUnequipped(this);
			}
		}
	}
	
	// 根据配置决定是否自动隐藏Actors
	// 如果设为false，需要在 K2_OnUnequipped 蓝图中播放卸下动画后手动调用 HideEquipmentActors()
	if (bAutoHideEquipmentActors)
	{
		HideEquipmentActors();
	}
	
	// 调用蓝图事件
	K2_OnUnequipped();
}

void UYcEquipmentInstance::BroadcastEquippedEvent()
{
	OnEquippedEvent.Broadcast(this);
}

void UYcEquipmentInstance::BroadcastUnequippedEvent()
{
	OnUnequippedEvent.Broadcast(this);
}

// ============================================================================
// Actor生命周期管理
// ============================================================================

void UYcEquipmentInstance::SpawnEquipmentActors(const TArray<FYcEquipmentActorToSpawn>& ActorsToSpawn)
{
	APawn* OwningPawn = GetPawn();
	if (!OwningPawn || OwningPawn->GetLocalRole() == ROLE_SimulatedProxy) return;
	
	const bool bIsServer = OwningPawn->HasAuthority();
	const bool bIsLocallyControlled = OwningPawn->IsLocallyControlled();
	
	// 如果已经生成过Actors，不重复生成
	if (bActorsSpawned)
	{
		UE_LOG(LogYcEquipment, Verbose, TEXT("SpawnEquipmentActors: Actors already spawned, skipping"));
		return;
	}
	
	// 获取默认的附加组件
	USceneComponent* AttachTarget = OwningPawn->GetRootComponent();	
	if (const ACharacter* Char = Cast<ACharacter>(OwningPawn))	
	{
		AttachTarget = Char->GetMesh();
	}

	for (const FYcEquipmentActorToSpawn& SpawnInfo : ActorsToSpawn)
	{
		// 计算附加目标
		USceneComponent* SpecifiedAttachTarget = OwningPawn->FindComponentByTag<USceneComponent>(SpawnInfo.ParentComponentTag);
		USceneComponent* FinalAttachTarget = SpecifiedAttachTarget ? SpecifiedAttachTarget : AttachTarget;
    
		if (SpawnInfo.ActorToSpawn.IsNull()) continue;
    
		// 确定生成条件
		bool bShouldSpawn = false;
		TArray<TObjectPtr<AActor>>* TargetArray = nullptr;
    
		if (SpawnInfo.bReplicateActor && bIsServer)
		{
			bShouldSpawn = true;
			TargetArray = &SpawnedActors;
		}
		else if (!SpawnInfo.bReplicateActor && bIsLocallyControlled)
		{
			bShouldSpawn = true;
			TargetArray = &OwnerClientSpawnedActors;
		}
    
		if (!bShouldSpawn) continue;
    
		// 加载Actor类
		const TSubclassOf<AActor> ActorToSpawnClass = SpawnInfo.ActorToSpawn.LoadSynchronous();
		if (!ActorToSpawnClass)
		{
			UE_LOG(LogYcEquipment, Error, TEXT("SpawnEquipmentActors: Failed to load actor class %s"), 
				*SpawnInfo.ActorToSpawn.ToString());
			continue;
		}
    
		// 生成Actor
		AActor* NewActor = SpawnEquipActorInternal(ActorToSpawnClass, SpawnInfo, FinalAttachTarget);
		if (NewActor)
		{
			TargetArray->Add(NewActor);
			
			// 新生成的Actor默认隐藏（Unequipped状态）
			SetActorVisualVisibility(NewActor, false);
			
			// 如果配置了休眠，进入休眠状态
			if (SpawnInfo.bDormantOnUnequip)
			{
				SetActorDormant(NewActor, SpawnInfo.bReplicateActor);
			}
		}
	}
	
	bActorsSpawned = true;
	UE_LOG(LogYcEquipment, Verbose, TEXT("SpawnEquipmentActors: Spawned %d replicated actors, %d local actors"), 
		SpawnedActors.Num(), OwnerClientSpawnedActors.Num());
}

void UYcEquipmentInstance::DestroyEquipmentActors()
{
	// 销毁服务器复制的Actors
	for (AActor* Actor : SpawnedActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	SpawnedActors.Empty();
	
	// 通知客户端销毁本地Actors
	ClientDestroyLocalActors();
	
	bActorsSpawned = false;
}

void UYcEquipmentInstance::ClientDestroyLocalActors_Implementation()
{
	for (AActor* Actor : OwnerClientSpawnedActors)
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	}
	OwnerClientSpawnedActors.Empty();
}

AActor* UYcEquipmentInstance::FindSpawnedActorByTag(const FName Tag, const bool bReplicateActor)
{
	const TArray<TObjectPtr<AActor>>& TargetArray = bReplicateActor ? SpawnedActors : OwnerClientSpawnedActors;
	
	for (AActor* Actor : TargetArray)
	{
		if (!IsValid(Actor)) continue;
		
		// 优先检查 EquipmentActorComponent 中的 Tags
		if (UYcEquipmentActorComponent* EquipComp = Actor->FindComponentByClass<UYcEquipmentActorComponent>())
		{
			if (EquipComp->HasEquipmentTag(Tag)) return Actor;
		}
		
		// 备选：检查 Actor 原生 Tags
		if (Actor->ActorHasTag(Tag)) return Actor;
	}
	return nullptr;
}

// ============================================================================
// Actor显示/隐藏（公开接口）
// ============================================================================

void UYcEquipmentInstance::ShowEquipmentActors()
{
	// 显示服务器复制的Actors
	ShowReplicatedActors();
	
	// 本地控制的Pawn还需要显示本地Actors
	if (APawn* Pawn = GetPawn())
	{
		if (Pawn->IsLocallyControlled())
		{
			ShowLocalActors();
		}
	}
}

void UYcEquipmentInstance::HideEquipmentActors()
{
	// 隐藏服务器复制的Actors
	HideReplicatedActors();
	
	// 本地控制的Pawn还需要隐藏本地Actors
	if (APawn* Pawn = GetPawn())
	{
		if (Pawn->IsLocallyControlled())
		{
			HideLocalActors();
		}
	}
}

// ============================================================================
// Actor显示/隐藏（内部实现）
// ============================================================================

void UYcEquipmentInstance::ShowReplicatedActors()
{
	if (!GetEquipmentDef()) return;
	
	const TArray<FYcEquipmentActorToSpawn>& ActorsToSpawn = EquipmentDef->ActorsToSpawn;
	
	int32 ReplicatedActorIndex = 0;
	for (int32 i = 0; i < ActorsToSpawn.Num() && ReplicatedActorIndex < SpawnedActors.Num(); ++i)
	{
		const FYcEquipmentActorToSpawn& SpawnInfo = ActorsToSpawn[i];
		if (!SpawnInfo.bReplicateActor) continue;
		
		AActor* Actor = SpawnedActors[ReplicatedActorIndex];
		if (IsValid(Actor))
		{
			// 从休眠状态唤醒
			if (SpawnInfo.bDormantOnUnequip)
			{
				WakeActorFromDormant(Actor, true);
			}
			
			SetActorVisualVisibility(Actor, true);
		}
		++ReplicatedActorIndex;
	}
}

void UYcEquipmentInstance::HideReplicatedActors()
{
	if (!GetEquipmentDef()) return;
	
	const TArray<FYcEquipmentActorToSpawn>& ActorsToSpawn = EquipmentDef->ActorsToSpawn;
	
	int32 ReplicatedActorIndex = 0;
	for (int32 i = 0; i < ActorsToSpawn.Num() && ReplicatedActorIndex < SpawnedActors.Num(); ++i)
	{
		const FYcEquipmentActorToSpawn& SpawnInfo = ActorsToSpawn[i];
		if (!SpawnInfo.bReplicateActor) continue;
		
		AActor* Actor = SpawnedActors[ReplicatedActorIndex];
		if (IsValid(Actor))
		{
			SetActorVisualVisibility(Actor, false);
			
			if (SpawnInfo.bDormantOnUnequip)
			{
				SetActorDormant(Actor, true);
			}
		}
		++ReplicatedActorIndex;
	}
}

void UYcEquipmentInstance::ShowLocalActors()
{
	if (!GetEquipmentDef()) return;
	
	const TArray<FYcEquipmentActorToSpawn>& ActorsToSpawn = EquipmentDef->ActorsToSpawn;
	
	int32 LocalActorIndex = 0;
	for (int32 i = 0; i < ActorsToSpawn.Num() && LocalActorIndex < OwnerClientSpawnedActors.Num(); ++i)
	{
		const FYcEquipmentActorToSpawn& SpawnInfo = ActorsToSpawn[i];
		if (SpawnInfo.bReplicateActor) continue;
		
		AActor* Actor = OwnerClientSpawnedActors[LocalActorIndex];
		if (IsValid(Actor))
		{
			if (SpawnInfo.bDormantOnUnequip)
			{
				WakeActorFromDormant(Actor, false);
			}
			
			SetActorVisualVisibility(Actor, true);
		}
		++LocalActorIndex;
	}
}

void UYcEquipmentInstance::HideLocalActors()
{
	if (!GetEquipmentDef()) return;
	
	const TArray<FYcEquipmentActorToSpawn>& ActorsToSpawn = EquipmentDef->ActorsToSpawn;
	
	int32 LocalActorIndex = 0;
	for (int32 i = 0; i < ActorsToSpawn.Num() && LocalActorIndex < OwnerClientSpawnedActors.Num(); ++i)
	{
		const FYcEquipmentActorToSpawn& SpawnInfo = ActorsToSpawn[i];
		if (SpawnInfo.bReplicateActor) continue;
		
		AActor* Actor = OwnerClientSpawnedActors[LocalActorIndex];
		if (IsValid(Actor))
		{
			SetActorVisualVisibility(Actor, false);
			
			if (SpawnInfo.bDormantOnUnequip)
			{
				SetActorDormant(Actor, false);
			}
		}
		++LocalActorIndex;
	}
}

// ============================================================================
// 休眠管理
// ============================================================================

void UYcEquipmentInstance::SetActorDormant(AActor* Actor, bool bReplicated)
{
	if (!IsValid(Actor)) return;
	
	Actor->SetActorTickEnabled(false);
	Actor->SetActorEnableCollision(false);
	
	if (bReplicated && Actor->GetIsReplicated())
	{
		Actor->SetNetDormancy(DORM_DormantAll);
	}
}

void UYcEquipmentInstance::WakeActorFromDormant(AActor* Actor, bool bReplicated)
{
	if (!IsValid(Actor)) return;
	
	if (bReplicated && Actor->GetIsReplicated())
	{
		Actor->SetNetDormancy(DORM_Awake);
		Actor->FlushNetDormancy();
	}
	
	Actor->SetActorTickEnabled(true);
	Actor->SetActorEnableCollision(true);
}

void UYcEquipmentInstance::SetActorVisualVisibility(const AActor* Actor, const bool bVisible)
{
	if (!IsValid(Actor)) return;
	
	if (USceneComponent* RootComp = Actor->GetRootComponent())
	{
		RootComp->SetVisibility(bVisible, true);
	}
}

// ============================================================================
// 内部实现
// ============================================================================

AActor* UYcEquipmentInstance::SpawnEquipActorInternal(
	const TSubclassOf<AActor>& ActorToSpawnClass,
	const FYcEquipmentActorToSpawn& SpawnInfo, 
	USceneComponent* AttachTarget)
{
	AActor* NewActor = GetWorld()->SpawnActorDeferred<AActor>(ActorToSpawnClass, FTransform::Identity, GetPawn());
	if (!NewActor)
	{
		UE_LOG(LogYcEquipment, Error, TEXT("SpawnEquipActorInternal: Failed to spawn actor"));
		return nullptr;
	}
	
	NewActor->FinishSpawning(FTransform::Identity, true);
	NewActor->SetActorRelativeTransform(SpawnInfo.AttachTransform);
	NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, SpawnInfo.AttachSocket);
	NewActor->Tags.Append(SpawnInfo.ActorTags);
	NewActor->SetInstigator(GetPawn());
	
	// 设置装备Actor组件数据
	if (UYcEquipmentActorComponent* EquipComp = NewActor->FindComponentByClass<UYcEquipmentActorComponent>())
	{
		EquipComp->SetIsReplicated(NewActor->GetIsReplicated());
		EquipComp->EquipmentInst = this;
		EquipComp->EquipmentTags = SpawnInfo.ActorTags;
		EquipComp->OnRep_EquipmentInst();
	}
	else
	{
		UE_LOG(LogYcEquipment, Warning, TEXT("SpawnEquipActorInternal: %s missing UYcEquipmentActorComponent"), 
			*GetNameSafe(ActorToSpawnClass));
	}
	
	return NewActor;
}

void UYcEquipmentInstance::SetInstigator(UYcInventoryItemInstance* InInstigator)
{
	Instigator = InInstigator;
	UpdateEquipmentDef();
}

void UYcEquipmentInstance::UpdateEquipmentDef()
{
	if (!Instigator || !Instigator->GetItemDef()) return;
	
	const FInventoryFragment_Equippable* Equippable = Instigator->GetItemDef()->GetTypedFragment<FInventoryFragment_Equippable>();
	if (!Equippable) return;
	
	EquipmentDef = &Equippable->EquipmentDef;
}
