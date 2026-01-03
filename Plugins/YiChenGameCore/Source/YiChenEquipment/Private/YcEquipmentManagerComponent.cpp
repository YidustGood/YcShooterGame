// Fill out your copyright notice in the Description page of Project Settings.


#include "YcEquipmentManagerComponent.h"

#include "AbilitySystemGlobals.h"
#include "YcAbilitySystemComponent.h"
#include "YcEquipmentInstance.h"
#include "YcInventoryItemInstance.h"
#include "YiChenEquipment.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcEquipmentManagerComponent)

//////////////////////////////////////////////////////////////////////
// FYcAppliedEquipmentEntry
FString FYcAppliedEquipmentEntry::GetDebugString() const
{
	return FString::Printf(TEXT("%s of %s"), *GetNameSafe(Instance), *GetNameSafe(OwnerItemInstance));
}

//////////////////////////////////////////////////////////////////////
// FYcEquipmentList
void FYcEquipmentList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	UYcEquipmentManagerComponent* EquipmentManager = Cast<UYcEquipmentManagerComponent>(OwnerComponent);
	
	// 获取并输出当前装备管理组件拥有者的本地角色(一般是拥有者是玩家控制的角色对象)
	ENetRole LocalRole = EquipmentManager->GetOwner()->GetLocalRole();
	
	for (const int32 Index : RemovedIndices)
	{
		if (const FYcAppliedEquipmentEntry& Entry = Equipments[Index]; Entry.Instance != nullptr)
		{
			/**
			 * 由于装备的附加Actor的显示和隐藏需要配合主控客户端做预测所以服务端也不通过SetActorHiddenInGame()了,
			 * 而是直接设置组件可视性这不会自动同步给模拟客户端, 所以我们要在移除装备实例同步到客户端时额外处理模拟客户端对于装备附加Actor的隐藏
			 */
			if (LocalRole == ROLE_SimulatedProxy)
			{
				Entry.Instance->OnUnequipped();
			}
		}
	}
}

void FYcEquipmentList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	UYcEquipmentManagerComponent* EquipmentManager = Cast<UYcEquipmentManagerComponent>(OwnerComponent);
	
	// 获取并输出当前装备管理组件拥有者的本地角色(一般是拥有者是玩家控制的角色对象)
	ENetRole LocalRole = EquipmentManager->GetOwner()->GetLocalRole();
	
	for (int32 Index : AddedIndices)
	{
		const FYcAppliedEquipmentEntry& Entry = Equipments[Index];
		if (Entry.Instance == nullptr) continue;
		
		Entry.Instance->SetInstigator(Entry.OwnerItemInstance);
		
		// 因为装备可能包含只需要在控制客户端生成的本地Actor, 所以需要在这里调用进行一次控制端的本地生成, 内部会处理判断生成逻辑
		Entry.Instance->SpawnEquipmentActors(Entry.Instance->EquipmentDef->ActorsToSpawn);
		
		/**
		 * 由于装备的附加Actor的显示和隐藏需要配合主控客户端做预测所以服务端也不通过SetActorHiddenInGame()了,
		 * 而是直接设置组件可视性这不会自动同步给模拟客户端, 所以我们要在添加的装备实例同步到客户端时额外处理模拟客户端对于装备附加Actor的显示
		 */
		if (LocalRole == ROLE_SimulatedProxy || !EquipmentManager->FindCachedEquipmentForItem(Entry.Instance->GetAssociatedItem()))
		{
			Entry.Instance->OnEquipped();
		}
		
		// 检查是否配置为不销毁（可复用）
		bool bShouldCache = false;
		if (const FYcEquipmentDefinition* EquipDef = Entry.Instance->GetEquipmentDef())
		{
			for (const FYcEquipmentActorToSpawn& SpawnInfo : EquipDef->ActorsToSpawn)
			{
				if (!SpawnInfo.bDestroyOnUnequip)
				{
					bShouldCache = true;
					break;
				}
			}
		}
		// 只需要主控端缓存, 模拟客户端用不上
		if (bShouldCache && EquipmentManager && Entry.OwnerItemInstance && LocalRole == ROLE_AutonomousProxy)
		{
			// 客户端缓存装备实例，用于主控端做预测显示
			EquipmentManager->CacheEquipmentInstance(Entry.OwnerItemInstance, Entry.Instance);
		}
	}
}

void FYcEquipmentList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (int32 Index : ChangedIndices)
	{
		const FYcAppliedEquipmentEntry& Entry = Equipments[Index];
		if (Entry.Instance != nullptr)
		{
			Entry.Instance->SetInstigator(Entry.OwnerItemInstance);
		}
	}
}

UYcAbilitySystemComponent* FYcEquipmentList::GetAbilitySystemComponent() const
{
	check(OwnerComponent);
	const AActor* OwningActor = OwnerComponent->GetOwner();
	return Cast<UYcAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor));
}

UYcEquipmentInstance* FYcEquipmentList::AddEntry(const FYcEquipmentDefinition& EquipmentDef, UYcInventoryItemInstance* ItemInstance)
{
	// 1. 有效性&网络权威检查
	check(OwnerComponent);
	check(OwnerComponent->GetOwner()->HasAuthority());
	
	// 2. 尝试从缓存中获取已有的装备实例（不移除，保持"一直缓存"策略）
	UYcEquipmentManagerComponent* EquipmentManager = Cast<UYcEquipmentManagerComponent>(OwnerComponent);
	UYcEquipmentInstance* ExistingInstance = EquipmentManager ? EquipmentManager->FindCachedEquipmentForItem(ItemInstance) : nullptr;
	
	if (ExistingInstance)
	{
		FYcAppliedEquipmentEntry& NewEntry = Equipments.AddDefaulted_GetRef();
		NewEntry.Instance = ExistingInstance;
		NewEntry.OwnerItemInstance = ItemInstance;
		
		// 重新授予技能
		if (UYcAbilitySystemComponent* ASC = GetAbilitySystemComponent())
		{
			for (const auto AbilitySet : EquipmentDef.AbilitySetsToGrant)
			{
				AbilitySet->GiveToAbilitySystem(ASC, /*inout*/ &NewEntry.GrantedHandles, NewEntry.Instance);
			}
		}
		
		// 处理装备附加Actors（内部会处理复用逻辑）
		ExistingInstance->SpawnEquipmentActors(EquipmentDef.ActorsToSpawn);
		
		MarkItemDirty(NewEntry);
		return ExistingInstance;
	}
	
	// 3. 没有缓存，创建新的装备实例
	TSubclassOf<UYcEquipmentInstance> InstanceType = EquipmentDef.InstanceType;
	if (InstanceType == nullptr)
	{
		InstanceType = UYcEquipmentInstance::StaticClass();
	}

	FYcAppliedEquipmentEntry& NewEntry = Equipments.AddDefaulted_GetRef();
	NewEntry.Instance = NewObject<UYcEquipmentInstance>(OwnerComponent->GetOwner(), InstanceType);
	NewEntry.Instance->SetInstigator(ItemInstance);
	NewEntry.OwnerItemInstance = ItemInstance;

	// 4. 把装备附带的AbilitySet应用到目标角色上
	if (UYcAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		for (const auto AbilitySet : EquipmentDef.AbilitySetsToGrant)
		{
			AbilitySet->GiveToAbilitySystem(ASC, /*inout*/ &NewEntry.GrantedHandles, NewEntry.Instance);
		}
	}
	else
	{
		UE_LOG(LogYcEquipment, Error, TEXT("装备尝试给玩家应用装备附带的AbilitySet失败，OwnerPawn的AbilitySystemComponent对象无效。"));
	}
	
	// 5. 生成装备附带的Actor并附加到玩家角色身
	NewEntry.Instance->SpawnEquipmentActors(EquipmentDef.ActorsToSpawn);
	
	// 6. 检查是否需要缓存（用于后续复用）
	bool bShouldCache = false;
	for (const FYcEquipmentActorToSpawn& SpawnInfo : EquipmentDef.ActorsToSpawn)
	{
		if (!SpawnInfo.bDestroyOnUnequip)
		{
			bShouldCache = true;
			break;
		}
	}
	if (bShouldCache)
	{
		EquipmentManager->CacheEquipmentInstance(ItemInstance, NewEntry.Instance);
	}

	// 7. 标记为脏，触发FastArray的网络同步
	MarkItemDirty(NewEntry);
	return NewEntry.Instance;
}

void FYcEquipmentList::RemoveEntry(UYcEquipmentInstance* Instance)
{
	// 1. 有效性&网络权威检查
	if(!OwnerComponent || !OwnerComponent->GetOwner())
	{
		UE_LOG(LogYcEquipment, Error, TEXT(" FYcEquipmentList::RemoveEntry->OwnerComponent->GetOwner() invalid."));
		return;
	}

	if(!OwnerComponent->GetOwner()->HasAuthority())
	{
		UE_LOG(LogYcEquipment, Error, TEXT(" FYcEquipmentList::RemoveEntry->Not Has Authority."));
		return;
	}

	// 2. 遍历当前的装备实例列表，并将其移除
	for (auto EntryIt = Equipments.CreateIterator(); EntryIt; ++EntryIt)
	{
		FYcAppliedEquipmentEntry& Entry = *EntryIt;
		if (Entry.Instance == Instance)
		{
			if (UYcAbilitySystemComponent* ASC = GetAbilitySystemComponent())
			{
				Entry.GrantedHandles.RemoveFromAbilitySystem(ASC); // 移除装备所赋予的能力
			}
			
			// 根据配置决定是销毁还是隐藏装备Actors
			// 注意：缓存已在 AddEntry 时完成，这里只需要判断是否销毁 Actors
			bool bShouldDestroy = true;
			if (const FYcEquipmentDefinition* EquipDef = Instance->GetEquipmentDef())
			{
				// 检查是否有任何Actor配置为不销毁
				for (const FYcEquipmentActorToSpawn& SpawnInfo : EquipDef->ActorsToSpawn)
				{
					if (!SpawnInfo.bDestroyOnUnequip)
					{
						bShouldDestroy = false;
						break;
					}
				}
			}
			
			if (bShouldDestroy)
			{
				Instance->DestroyEquipmentActors();
			}
			// 不销毁时，Actors 保持隐藏状态，等待下次装备时复用
			
			// 移除当前装备实例并进行标记网络同步
			EntryIt.RemoveCurrent();
			MarkArrayDirty();
		}
	}
}

//////////////////////////////////////////////////////////////////////
// UYcEquipmentManagerComponent

UYcEquipmentManagerComponent::UYcEquipmentManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	  , EquipmentList(this)
{
	SetIsReplicatedByDefault(true);
	bWantsInitializeComponent = true;
	bReplicateUsingRegisteredSubObjectList = true;
}

void UYcEquipmentManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UYcEquipmentManagerComponent, EquipmentList);
}

void UYcEquipmentManagerComponent::InitializeComponent()
{
	Super::InitializeComponent();
}

void UYcEquipmentManagerComponent::UninitializeComponent()
{
	// 在移除前收集所有实例，避免装备列表迭代器失效问题
	if(GetNetMode() < NM_Client)
	{
		TArray<UYcEquipmentInstance*> AllEquipmentInstances;
		for (const FYcAppliedEquipmentEntry& Entry : EquipmentList.Equipments)
		{
			AllEquipmentInstances.Add(Entry.Instance);
		}

		for (UYcEquipmentInstance* EquipInstance : AllEquipmentInstances)
		{
			UnequipItem(EquipInstance);
		}
		
		// 清理所有缓存的装备实例
		ClearAllCachedEquipment();
	}
	
	Super::UninitializeComponent();
}

void UYcEquipmentManagerComponent::ReadyForReplication()
{
	// 注册已存在的装备实例
	if (IsUsingRegisteredSubObjectList())
	{
		for (const FYcAppliedEquipmentEntry& Entry : EquipmentList.Equipments)
		{
			if (UYcEquipmentInstance* Instance = Entry.Instance)
			{
				AddReplicatedSubObject(Instance);
			}
		}
	}
	Super::ReadyForReplication();
}

UYcEquipmentInstance* UYcEquipmentManagerComponent::EquipItemByEquipmentDef(const FYcEquipmentDefinition& EquipmentDef,
                                                                             UYcInventoryItemInstance* ItemInstance)
{
	if (ItemInstance == nullptr)
	{
		UE_LOG(LogYcEquipment, Error, TEXT("Create equipment instance failed! The ItemInst is invalid."));
		return nullptr;
	}
	
	UYcEquipmentInstance* Result = EquipmentList.AddEntry(EquipmentDef, ItemInstance);
	if (Result == nullptr)
	{
		UE_LOG(LogYcEquipment, Error, TEXT("Create equipment instance failed!"));
		return nullptr;
	}
	
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
	{
		AddReplicatedSubObject(Result);
		AddReplicatedSubObject(ItemInstance);
	}
	
	Result->OnEquipped();
	return Result;
}

void UYcEquipmentManagerComponent::UnequipItem(UYcEquipmentInstance* EquipmentInst)
{
	if (EquipmentInst == nullptr) return;
	
	if (IsUsingRegisteredSubObjectList())
	{
		RemoveReplicatedSubObject(EquipmentInst);
	}
	EquipmentInst->OnUnequipped();
	EquipmentList.RemoveEntry(EquipmentInst);
}

UYcEquipmentInstance* UYcEquipmentManagerComponent::GetFirstInstanceOfType(const TSubclassOf<UYcEquipmentInstance> InstanceType)
{
	for (FYcAppliedEquipmentEntry& Entry : EquipmentList.Equipments)
	{
		if (UYcEquipmentInstance* Instance = Entry.Instance; Instance->IsA(InstanceType))
		{
			return Instance;
		}
	}

	return nullptr;
}

TArray<UYcEquipmentInstance*> UYcEquipmentManagerComponent::GetEquipmentInstancesOfType(const TSubclassOf<UYcEquipmentInstance> InstanceType) const
{
	TArray<UYcEquipmentInstance*> Results;
	for (const FYcAppliedEquipmentEntry& Entry : EquipmentList.Equipments)
	{
		if (UYcEquipmentInstance* Instance = Entry.Instance; Instance->IsA(InstanceType))
		{
			Results.Add(Instance);
		}
	}
	return Results;
}

UYcEquipmentInstance* UYcEquipmentManagerComponent::FindCachedEquipmentForItem(UYcInventoryItemInstance* ItemInstance) const
{
	if (!ItemInstance) return nullptr;
	
	const TObjectPtr<UYcEquipmentInstance>* Found = CachedEquipmentInstances.Find(ItemInstance);
	return Found ? Found->Get() : nullptr;
}

void UYcEquipmentManagerComponent::CacheEquipmentInstance(UYcInventoryItemInstance* ItemInstance, UYcEquipmentInstance* EquipmentInstance)
{
	if (!ItemInstance || !EquipmentInstance) return;
 	
	// 避免重复缓存
	if (CachedEquipmentInstances.Contains(ItemInstance))
	{
		return;
	}
	
	CachedEquipmentInstances.Add(ItemInstance, EquipmentInstance);
	UE_LOG(LogYcEquipment, Verbose, TEXT("CacheEquipmentInstance: 缓存装备实例 %s 用于物品 %s"), 
		*GetNameSafe(EquipmentInstance), *GetNameSafe(ItemInstance));
}

void UYcEquipmentManagerComponent::ClearCachedEquipmentForItem(UYcInventoryItemInstance* ItemInstance)
{
	if (!ItemInstance) return;
	
	if (TObjectPtr<UYcEquipmentInstance>* Found = CachedEquipmentInstances.Find(ItemInstance))
	{
		if (UYcEquipmentInstance* CachedInstance = Found->Get())
		{
			// 真正销毁缓存的装备实例的Actors
			CachedInstance->DestroyEquipmentActors();
		}
		CachedEquipmentInstances.Remove(ItemInstance);
	}
}

void UYcEquipmentManagerComponent::ClearAllCachedEquipment()
{
	for (auto& Pair : CachedEquipmentInstances)
	{
		if (UYcEquipmentInstance* CachedInstance = Pair.Value.Get())
		{
			CachedInstance->DestroyEquipmentActors();
		}
	}
	CachedEquipmentInstances.Empty();
}