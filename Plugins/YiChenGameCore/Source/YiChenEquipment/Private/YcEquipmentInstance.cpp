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

UYcEquipmentInstance::UYcEquipmentInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), EquipmentDef(nullptr), bActorsSpawned(false)
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
	UpdateEquipmentDef();
}

void UYcEquipmentInstance::UpdateEquipmentDef()
{
	if (!Instigator || !Instigator->GetItemDef()) return;
	
	const FInventoryFragment_Equippable* Equippable = Instigator->GetItemDef()->GetTypedFragment<FInventoryFragment_Equippable>();
	if (!Equippable) return;
	EquipmentDef = &Equippable->EquipmentDef;
}

void UYcEquipmentInstance::SpawnEquipmentActors(const TArray<FYcEquipmentActorToSpawn>& ActorsToSpawn)
{
	APawn* OwningPawn = GetPawn();
	if (!OwningPawn || OwningPawn->GetLocalRole() == ROLE_SimulatedProxy) return;
	
	const bool bIsServer = OwningPawn->HasAuthority();
	const bool bIsLocallyControlled = OwningPawn->IsLocallyControlled();
	
	// 如果已经生成过Actors，尝试复用（显示已隐藏的Actors）
	if (bActorsSpawned)
	{
		if (bIsServer)
		{
			// 服务器：显示服务器Actors并通知客户端
			ShowEquipmentActors();
		}
		else if (bIsLocallyControlled)
		{
			// 客户端：直接显示本地Actors（服务器Actors通过网络复制自动处理）
			ShowLocalEquipmentActors();
		}
		return;
	}
	
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
    
		if (SpawnInfo.bReplicateActor && bIsServer)
		{
			bShouldSpawn = true;
			ContextPrefix = TEXT("Server");
			TargetArray = &SpawnedActors;
		}
		else if (!SpawnInfo.bReplicateActor && bIsLocallyControlled)
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
	
	// 标记已生成过Actors
	bActorsSpawned = true;
}

void UYcEquipmentInstance::DestroyEquipmentActors()
{
	for (AActor* Actor : SpawnedActors)
	{
		if (Actor) Actor->Destroy();
	}
	SpawnedActors.Empty();
	
	DestroyEquipmentActorsOnOwnerClient();
	
	// 重置标记，允许下次重新生成
	bActorsSpawned = false;
}

void UYcEquipmentInstance::DestroyEquipmentActorsOnOwnerClient_Implementation()
{
	for (AActor* Actor : OwnerClientSpawnedActors)
	{
		if (Actor) Actor->Destroy();
	}
	OwnerClientSpawnedActors.Empty();
}

void UYcEquipmentInstance::HideEquipmentActors()
{
	if (!GetEquipmentDef()) return;
	
	const TArray<FYcEquipmentActorToSpawn>& ActorsToSpawn = EquipmentDef->ActorsToSpawn;
	
	// 处理服务器复制的Actors
	int32 ReplicatedActorIndex = 0;
	for (int32 i = 0; i < ActorsToSpawn.Num() && ReplicatedActorIndex < SpawnedActors.Num(); ++i)
	{
		const FYcEquipmentActorToSpawn& SpawnInfo = ActorsToSpawn[i];
		if (!SpawnInfo.bReplicateActor) continue; // 跳过非复制的Actor配置
		
		AActor* Actor = SpawnedActors[ReplicatedActorIndex];
		if (IsValid(Actor))
		{
			// 使用组件可见性而不是 Actor 隐藏，避免网络复制覆盖客户端预测状态
			SetActorVisualVisibility(Actor, false);
			
			// 根据配置决定是否进入休眠状态
			if (SpawnInfo.bDormantOnUnequip)
			{
				SetActorDormant(Actor, true);
			}
		}
		++ReplicatedActorIndex;
	}
	
	// 通知控制客户端隐藏非网络复制的本地Actors
	// HideEquipmentActorsOnOwnerClient(); // QuickBar中做了客户端预测了, 展示不走ClientRPC
}

void UYcEquipmentInstance::HideEquipmentActorsOnOwnerClient_Implementation()
{
	HideLocalEquipmentActors();
}

void UYcEquipmentInstance::HideLocalEquipmentActors()
{
	if (!GetEquipmentDef()) return;
	
	const TArray<FYcEquipmentActorToSpawn>& ActorsToSpawn = EquipmentDef->ActorsToSpawn;
	
	// 找到本地Actor对应的SpawnInfo（非复制的）
	int32 LocalActorIndex = 0;
	for (int32 i = 0; i < ActorsToSpawn.Num() && LocalActorIndex < OwnerClientSpawnedActors.Num(); ++i)
	{
		const FYcEquipmentActorToSpawn& SpawnInfo = ActorsToSpawn[i];
		if (SpawnInfo.bReplicateActor) continue;
		
		AActor* Actor = OwnerClientSpawnedActors[LocalActorIndex];
		if (IsValid(Actor))
		{
			// 使用组件可见性而不是 Actor 隐藏，避免网络复制问题
			SetActorVisualVisibility(Actor, false);
			
			if (SpawnInfo.bDormantOnUnequip)
			{
				SetActorDormant(Actor, false);
			}
		}
		++LocalActorIndex;
	}
}

void UYcEquipmentInstance::ShowEquipmentActors()
{
	if (!GetEquipmentDef()) return;
	
	const TArray<FYcEquipmentActorToSpawn>& ActorsToSpawn = EquipmentDef->ActorsToSpawn;
	
	// 处理服务器复制的Actors
	int32 ReplicatedActorIndex = 0;
	for (int32 i = 0; i < ActorsToSpawn.Num() && ReplicatedActorIndex < SpawnedActors.Num(); ++i)
	{
		const FYcEquipmentActorToSpawn& SpawnInfo = ActorsToSpawn[i];
		if (!SpawnInfo.bReplicateActor) continue; // 跳过非复制的Actor配置
		
		AActor* Actor = SpawnedActors[ReplicatedActorIndex];
		if (IsValid(Actor))
		{
			// 从休眠状态唤醒
			if (SpawnInfo.bDormantOnUnequip)
			{
				WakeActorFromDormant(Actor, true);
			}
			
			// 使用组件可见性显示 Actor
			SetActorVisualVisibility(Actor, true);
		}
		++ReplicatedActorIndex;
	}
	
	// 通知客户端处理本地Actors
	// ShowEquipmentActorsOnOwnerClient(); // QuickBar中做了客户端预测了, 展示不走ClientRPC
}

void UYcEquipmentInstance::ShowEquipmentActorsOnOwnerClient_Implementation()
{
	ShowLocalEquipmentActors();
}

void UYcEquipmentInstance::ShowLocalEquipmentActors()
{
	if (!GetEquipmentDef()) return;
	
	const TArray<FYcEquipmentActorToSpawn>& ActorsToSpawn = EquipmentDef->ActorsToSpawn;
	
	// 找到本地Actor对应的SpawnInfo（非复制的）
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
			
			// 使用组件可见性显示 Actor
			SetActorVisualVisibility(Actor, true);
		}
		++LocalActorIndex;
	}
}

void UYcEquipmentInstance::SetActorDormant(AActor* Actor, bool bReplicated)
{
	if (!IsValid(Actor)) return;
	
	// 禁用Tick
	Actor->SetActorTickEnabled(false);
	
	// 禁用碰撞
	Actor->SetActorEnableCollision(false);
	
	// 对于网络复制的Actor，设置网络休眠
	if (bReplicated && Actor->GetIsReplicated())
	{
		Actor->SetNetDormancy(DORM_DormantAll);
	}
}

void UYcEquipmentInstance::WakeActorFromDormant(AActor* Actor, bool bReplicated)
{
	if (!IsValid(Actor)) return;
	
	// 对于网络复制的Actor，先唤醒网络
	if (bReplicated && Actor->GetIsReplicated())
	{
		Actor->SetNetDormancy(DORM_Awake);
		Actor->FlushNetDormancy();
	}
	
	// 恢复Tick
	Actor->SetActorTickEnabled(true);
	
	// 恢复碰撞
	Actor->SetActorEnableCollision(true);
}

AActor* UYcEquipmentInstance::FindSpawnedActorByTag(const FName Tag, const bool bReplicateActor)
{
	const TArray<TObjectPtr<AActor>>& TargetArray = bReplicateActor ? SpawnedActors : OwnerClientSpawnedActors;
	
	for (AActor* Actor : TargetArray)
	{
		if (!IsValid(Actor)) continue;
		
		// 优先检查 EquipmentActorComponent 中的 Tags（服务器和客户端都有效）
		if (UYcEquipmentActorComponent* EquipComp = Actor->FindComponentByClass<UYcEquipmentActorComponent>())
		{
			if (EquipComp->HasEquipmentTag(Tag)) return Actor;
		}
		
		// 备选：检查 Actor 原生 Tags（仅服务器端有效）
		if (Actor->ActorHasTag(Tag)) return Actor;
	}
	return nullptr;
}

bool UYcEquipmentInstance::IsEquipped() const
{
	return bEquipped;
}

const FYcEquipmentDefinition* UYcEquipmentInstance::GetEquipmentDef()
{
	if (EquipmentDef) return EquipmentDef;
	// 如果无效尝试更新获取更新一下
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
		UE_LOG(LogYcEquipment, Warning, TEXT("UYcEquipmentInstance::FindEquipmentFragment - EquipmentDef or FragmentStructType is null"));
		return TInstancedStruct<FYcEquipmentFragment>();
	}
	
	// 在装备定义的 Fragments 数组中查找指定类型的 Fragment
	for (const auto& Fragment : EquipmentDef->Fragments)
	{
		if (Fragment.GetScriptStruct() == FragmentStructType)
		{
			return Fragment;
		}
	}
	
	// 如果没有找到，返回空结构体
	return TInstancedStruct<FYcEquipmentFragment>();
}

void UYcEquipmentInstance::OnEquipped()
{
	if (bEquipped) return; // 避免重复调用
	bEquipped = true;
	// 显示装备附加的Actor
	ShowEquipmentActors();
	if (GetPawn() && GetPawn()->IsLocallyControlled())
	{
		ShowLocalEquipmentActors();
	}
	// 调用蓝图实现的装备函数, 例如在蓝图中扩展播放装备动画等
	K2_OnEquipped();
}

void UYcEquipmentInstance::OnUnequipped()
{
	if (bEquipped == false) return; // 避免重复调用
	bEquipped = false;
	K2_OnUnequipped();
	// 这里注意要在蓝图中调用隐藏装备附加Actor, 因为需要播放卸下装备的动画所以不能先隐藏
}

void UYcEquipmentInstance::BroadcastEquippedEvent()
{
	OnEquippedEvent.Broadcast(this);
}

void UYcEquipmentInstance::BroadcastUnequippedEvent()
{
	OnUnequippedEvent.Broadcast(this);
}

AActor* UYcEquipmentInstance::SpawnEquipActorInternal(const TSubclassOf<AActor>& ActorToSpawnClass,const FYcEquipmentActorToSpawn& SpawnInfo, USceneComponent* AttachTarget)
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
	NewActor->Tags.Append(SpawnInfo.ActorTags);  // 服务器端仍然设置 Actor Tags（兼容性）
	
	// 设置装备Actor组件数据，用于反向查询所属装备实例和存储 Tags, 如果没有则打印错误日志提示开发者添加
	if (UYcEquipmentActorComponent* EquipComp = NewActor->FindComponentByClass<UYcEquipmentActorComponent>())
	{
		EquipComp->SetIsReplicated(NewActor->GetIsReplicated());
		EquipComp->EquipmentInst = this;
		EquipComp->EquipmentTags = SpawnInfo.ActorTags;  // 设置 Tags，会通过组件复制到客户端
		EquipComp->OnRep_EquipmentInst(); // 服务端手动触发委托, 客户端网络同步到达自动触发
	}else
	{
		UE_LOG(LogYcEquipment, Error, TEXT("UYcEquipmentInstance::SpawnEquipActorInternal: %s 这个装备生成的Actor没有添加UYcEquipmentActorComponent组件, 请添加!"), *GetNameSafe(NewActor->GetClass()));
	}
	
	return NewActor;
}

void UYcEquipmentInstance::SetActorVisualVisibility(const AActor* Actor, const bool bVisible)
{
	if (!IsValid(Actor)) return;
	
	if (USceneComponent* RootComp = Actor->GetRootComponent())
	{
		// 设置根组件及所有子组件的可见性
		RootComp->SetVisibility(bVisible, true);
	}
}