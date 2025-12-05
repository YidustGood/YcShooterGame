// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcEquipmentInstance.h"

#include "YcInventoryItemInstance.h"
#include "YiChenEquipment.h"
#include "Fragments/InventoryFragment_Equippable.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcEquipmentInstance)

UYcEquipmentInstance::UYcEquipmentInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), EquipmentDef()
{
}

void UYcEquipmentInstance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UYcEquipmentInstance, SpawnedActors);
}

UYcInventoryItemInstance* UYcEquipmentInstance::GetAssociatedItem() const
{
	return Instigator;
}

APawn* UYcEquipmentInstance::GetPawn() const
{
	// EquipmentManagerComponent可能挂载到Pawn或者PlayerState和PlayerController这三者上，所以需要三种方案获取实际的Pawn
	if(APawn* Pawn = Cast<APawn>(GetOuter()))
	{
		return Pawn;
	}
	
	if(const APlayerState* PlayerState = Cast<APlayerState>(GetOuter()))
	{
		return PlayerState->GetPawn();
	}
	
	if(const APlayerController* PlayerController = Cast<APlayerController>(GetOuter()))
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

void UYcEquipmentInstance::SetInstigator(UYcInventoryItemInstance* InInstigator)
{
	Instigator = InInstigator;
}

void UYcEquipmentInstance::SpawnEquipmentActors(const TArray<FYcEquipmentActorToSpawn>& ActorsToSpawn)
{
	APawn* OwningPawn = GetPawn();
	if (!OwningPawn || OwningPawn->GetLocalRole() == ROLE_SimulatedProxy) return;
	
	// 先获取默认的附加组件
	USceneComponent* AttachTarget = OwningPawn->GetRootComponent();	
	if (const ACharacter* Char = Cast<ACharacter>(OwningPawn))	
	{
		AttachTarget = Char->GetMesh();
	}

	for (const FYcEquipmentActorToSpawn& SpawnInfo : ActorsToSpawn)
	{
		// 1. 计算附加目标
		USceneComponent* SpecifiedAttachTarget = OwningPawn->FindComponentByTag<USceneComponent>(SpawnInfo.ParentComponentTag);
		AttachTarget = SpecifiedAttachTarget ? SpecifiedAttachTarget : AttachTarget;
    
		if (SpawnInfo.ActorToSpawn.IsNull()) continue;
    
		// 2. 确定生成条件和上下文, 以区分网络复制和非网络复制的Actor
		bool bShouldSpawn = false;
		FString ContextPrefix;
		TArray<TObjectPtr<AActor>>* TargetArray = nullptr;
    
		if (SpawnInfo.bReplicateActor && OwningPawn->HasAuthority())
		{
			bShouldSpawn = true;
			ContextPrefix = TEXT("Server");
			TargetArray = &SpawnedActors;
		}
		else if (!SpawnInfo.bReplicateActor && OwningPawn->IsLocallyControlled())
		{
			bShouldSpawn = true;
			ContextPrefix = TEXT("Locally");
			TargetArray = &OwnerClientSpawnedActors;
		}
    
		if (!bShouldSpawn) continue;
    
		// 3. 统一生成逻辑
		FString ScopeActivityText = FString::Printf(TEXT("%s Spawning actor to component %s (%s)"), 
			*ContextPrefix, *AttachTarget->GetName(), *SpawnInfo.ActorToSpawn.ToString());
    
		UE_SCOPED_ENGINE_ACTIVITY(*ScopeActivityText);
    
		const TSubclassOf<AActor> ActorToSpawnClass = SpawnInfo.ActorToSpawn.LoadSynchronous();
    
		if (!ActorToSpawnClass)
		{
			UE_LOG(LogYcEquipment, Error, TEXT("[YcEquipmentInstance %s]: %s - Failed to load actor to spawn class %s. Not spawn actor."),
				*GetPathNameSafe(this), *ContextPrefix, *SpawnInfo.ActorToSpawn.ToString());
			continue;
		}
    
		AActor* NewActor = SpawnEquipActorInternal(ActorToSpawnClass, SpawnInfo, AttachTarget);
		if (NewActor == nullptr) continue;
    
		TargetArray->Add(NewActor);
	}
}

void UYcEquipmentInstance::DestroyEquipmentActors()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor) Actor->Destroy();
	}
	//@喜剧BUG效果-注释掉下面一行代码触发武器重叠BUG
	DestroyEquipmentActorsOnOwnerClient();
}

void UYcEquipmentInstance::DestroyEquipmentActorsOnOwnerClient_Implementation()
{
	for (AActor* Actor : OwnerClientSpawnedActors)
	{
		if (Actor) Actor->Destroy();
	}
}

AActor* UYcEquipmentInstance::FindSpawnedActorByTag(const FName Tag, const bool bReplicateActor)
{
	if(bReplicateActor)
	{
		for (const auto& SpawnedActor : SpawnedActors)
		{
			if(IsValid(SpawnedActor) && SpawnedActor->ActorHasTag(Tag)) return SpawnedActor;
		}
	}
	else
	{
		for (const auto& OwnerClientSpawnedActor : OwnerClientSpawnedActors)
		{
			if(IsValid(OwnerClientSpawnedActor) && OwnerClientSpawnedActor->ActorHasTag(Tag)) return OwnerClientSpawnedActor;
		}
	}
	return nullptr;
}

const FYcEquipmentDefinition* UYcEquipmentInstance::GetEquipmentDef()
{
	if (!Instigator || !Instigator->GetItemDef()) return nullptr;
	
	if (!EquipmentDef)
	{
		const FInventoryFragment_Equippable* Equippable = Instigator->GetItemDef()->GetTypedFragment<FInventoryFragment_Equippable>();
		if (!Equippable) return nullptr;
		EquipmentDef = &Equippable->EquipmentDef;
	}
	return EquipmentDef;
}

bool UYcEquipmentInstance::K2_GetEquipmentDef(FYcEquipmentDefinition& OutEquipmentDef)
{
	EquipmentDef = EquipmentDef ?  EquipmentDef : GetEquipmentDef();
	if (!EquipmentDef) return false;
	
	OutEquipmentDef = *EquipmentDef;
	return true;
}

void UYcEquipmentInstance::OnEquipped()
{
	K2_OnEquipped();
}

void UYcEquipmentInstance::OnUnequipped()
{
	K2_OnUnequipped();
}

AActor* UYcEquipmentInstance::SpawnEquipActorInternal(const TSubclassOf<AActor>& ActorToSpawnClass,const FYcEquipmentActorToSpawn& SpawnInfo, USceneComponent* AttachTarget) const
{
	AActor* NewActor = GetWorld()->SpawnActorDeferred<AActor>(ActorToSpawnClass, FTransform::Identity, GetPawn());

	if(NewActor == nullptr)
	{
		UE_LOG(LogYcEquipment, Error, TEXT("UYcEquipmentInstance::SpawnEquipActorInternal: Spawn equip actor failed."));
		return nullptr;
	}
	NewActor->FinishSpawning(FTransform::Identity, /*bIsDefaultTransform=*/ true);
	NewActor->SetActorRelativeTransform(SpawnInfo.AttachTransform);
	NewActor->AttachToComponent(AttachTarget, FAttachmentTransformRules::KeepRelativeTransform, SpawnInfo.AttachSocket);
	NewActor->Tags.Append(SpawnInfo.ActorTags);
	
	return NewActor;
}
