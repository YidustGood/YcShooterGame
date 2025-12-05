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
	
	// 2. 根据EquipmentDef创建对应的EquipmentInst实例对象,并添加到复制的列表中
	TSubclassOf<UYcEquipmentInstance> InstanceType = EquipmentDef.InstanceType;
	if (InstanceType == nullptr) // 如果装备定义中没有指定装备实例对象，那么就使用默认实例对象进行创建添加
	{
		InstanceType = UYcEquipmentInstance::StaticClass();
	}

	FYcAppliedEquipmentEntry& NewEntry = Equipments.AddDefaulted_GetRef();
	NewEntry.Instance = NewObject<UYcEquipmentInstance>(OwnerComponent->GetOwner(), InstanceType); //@TODO: Using the actor instead of component as the outer due to UE-127172
	NewEntry.Instance->SetInstigator(ItemInstance);
	NewEntry.OwnerItemInstance = ItemInstance;

	// 3. 把装备附带的AbilitySet应用到目标角色上
	if (UYcAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		for (const auto AbilitySet : EquipmentDef.AbilitySetsToGrant)
		{
			AbilitySet->GiveToAbilitySystem(ASC, /*inout*/ &NewEntry.GrantedHandles, NewEntry.Instance);
		}
	}
	else
	{
		UE_LOG(LogYcEquipment, Warning, TEXT("装备尝试给玩家应用装备附带的AbilitySet失败，OwnerPawn的AbilitySystemComponent对象无效。"));
	}
	
	// 4. 生成装备附带的Actor并附加到玩家角色身
	NewEntry.Instance->SpawnEquipmentActors(EquipmentDef.ActorsToSpawn);

	// 5. 标记为脏，已触发FastArray的网络同步
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
			
			Instance->DestroyEquipmentActors();
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