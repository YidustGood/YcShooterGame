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
	for (const int32 Index : RemovedIndices)
	{
		if (const FYcAppliedEquipmentEntry& Entry = Equipments[Index]; Entry.Instance != nullptr)
		{
			Entry.Instance->OnUnequipped();
		}
	}
}

void FYcEquipmentList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (int32 Index : AddedIndices)
	{
		const FYcAppliedEquipmentEntry& Entry = Equipments[Index];
		if (Entry.Instance != nullptr)
		{
			Entry.Instance->SetInstigator(Entry.OwnerItemInstance);
			Entry.Instance->OnEquipped();
			// 因为装备可能包含只需要在控制客户端生成的Actor所以需要在这里调用进行一次控制端的本地生成, 内部会处理判断生成逻辑
			Entry.Instance->SpawnEquipmentActors(Entry.Instance->EquipmentDef->ActorsToSpawn);
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
	
	// 2. 尝试从缓存中获取已有的装备实例
	UYcEquipmentManagerComponent* EquipmentManager = Cast<UYcEquipmentManagerComponent>(OwnerComponent);
	UYcEquipmentInstance* ExistingInstance = EquipmentManager ? EquipmentManager->FindCachedEquipmentForItem(ItemInstance) : nullptr;
	
	if (ExistingInstance)
	{
		// 复用缓存的装备实例，从缓存中移除
		EquipmentManager->TakeCachedEquipmentForItem(ItemInstance);
		
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
		
		// 显示之前隐藏的Actors（内部会处理复用逻辑）
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

	// 6. 标记为脏，已触发FastArray的网络同步
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

	UYcEquipmentManagerComponent* EquipmentManager = Cast<UYcEquipmentManagerComponent>(OwnerComponent);

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
			else
			{
				// 隐藏Actors并缓存装备实例以便复用
				Instance->HideEquipmentActors();
				if (EquipmentManager && Entry.OwnerItemInstance)
				{
					EquipmentManager->CacheEquipmentInstance(Entry.OwnerItemInstance, Instance);
				}
			}
			
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
	CachedEquipmentInstances.Add(ItemInstance, EquipmentInstance);
}

UYcEquipmentInstance* UYcEquipmentManagerComponent::TakeCachedEquipmentForItem(UYcInventoryItemInstance* ItemInstance)
{
	if (!ItemInstance) return nullptr;
	
	TObjectPtr<UYcEquipmentInstance> Result;
	if (CachedEquipmentInstances.RemoveAndCopyValue(ItemInstance, Result))
	{
		return Result.Get();
	}
	return nullptr;
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