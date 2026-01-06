// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcEquipmentManagerComponent.h"

#include "AbilitySystemGlobals.h"
#include "YcAbilitySystemComponent.h"
#include "YcEquipmentInstance.h"
#include "YcInventoryItemInstance.h"
#include "YiChenEquipment.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcEquipmentManagerComponent)

// ============================================================================
// FYcEquipmentEntry
// ============================================================================

FString FYcEquipmentEntry::GetDebugString() const
{
	return FString::Printf(TEXT("%s of %s [%s]"), 
		*GetNameSafe(Instance), 
		*GetNameSafe(OwnerItemInstance),
		Instance ? *UEnum::GetValueAsString(Instance->GetEquipmentState()) : TEXT("null"));
}

// ============================================================================
// FYcEquipmentList - FastArray 回调
// ============================================================================

void FYcEquipmentList::PreReplicatedRemove(const TArrayView<int32> RemovedIndices, int32 FinalSize)
{
	for (const int32 Index : RemovedIndices)
	{
		const FYcEquipmentEntry& Entry = Entries[Index];
		if (Entry.Instance)
		{
			// 如果装备处于已装备状态，先卸下
			if (Entry.Instance->IsEquipped())
			{
				Entry.Instance->OnUnequipped();
			}
			
			UE_LOG(LogYcEquipment, Log, TEXT("PreReplicatedRemove: %s"), *Entry.GetDebugString());
		}
	}
}

void FYcEquipmentList::PostReplicatedAdd(const TArrayView<int32> AddedIndices, int32 FinalSize)
{
	for (const int32 Index : AddedIndices)
	{
		FYcEquipmentEntry& Entry = Entries[Index];
		if (!Entry.Instance) continue;
		
		// 设置 Instigator（客户端需要通过这个获取 EquipmentDef）
		Entry.Instance->SetInstigator(Entry.OwnerItemInstance);
		
		// 客户端生成本地Actors（如果需要）
		if (const FYcEquipmentDefinition* EquipDef = Entry.Instance->GetEquipmentDef())
		{
			Entry.Instance->SpawnEquipmentActors(EquipDef->ActorsToSpawn);
			// 如果添加后并未装备, 代表是初始化创建那么我们就隐藏附加Actors
			if (!Entry.Instance->IsEquipped())
			{
				Entry.Instance->HideEquipmentActors();
			}
		}
		
		UE_LOG(LogYcEquipment, Log, TEXT("PostReplicatedAdd: %s"), *Entry.GetDebugString());
	}
}

void FYcEquipmentList::PostReplicatedChange(const TArrayView<int32> ChangedIndices, int32 FinalSize)
{
	for (const int32 Index : ChangedIndices)
	{
		FYcEquipmentEntry& Entry = Entries[Index];
		if (Entry.Instance)
		{
			Entry.Instance->SetInstigator(Entry.OwnerItemInstance);
			UE_LOG(LogYcEquipment, Log, TEXT("PostReplicatedChange: %s"), *Entry.GetDebugString());
		}
	}
}

// ============================================================================
// FYcEquipmentList - 装备管理
// ============================================================================

UYcAbilitySystemComponent* FYcEquipmentList::GetAbilitySystemComponent() const
{
	check(OwnerComponent);
	const AActor* OwningActor = OwnerComponent->GetOwner();
	return Cast<UYcAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwningActor));
}

FYcEquipmentEntry* FYcEquipmentList::FindEntry(UYcEquipmentInstance* Instance)
{
	for (FYcEquipmentEntry& Entry : Entries)
	{
		if (Entry.Instance == Instance)
		{
			return &Entry;
		}
	}
	return nullptr;
}

UYcEquipmentInstance* FYcEquipmentList::CreateEntry(const FYcEquipmentDefinition& EquipmentDef, UYcInventoryItemInstance* ItemInstance)
{
	check(OwnerComponent);
	check(OwnerComponent->GetOwner()->HasAuthority());
	
	if (!ItemInstance)
	{
		UE_LOG(LogYcEquipment, Error, TEXT("CreateEntry: ItemInstance is null"));
		return nullptr;
	}
	
	// 检查是否已存在该物品的装备实例
	for (const FYcEquipmentEntry& Entry : Entries)
	{
		if (Entry.OwnerItemInstance == ItemInstance)
		{
			UE_LOG(LogYcEquipment, Warning, TEXT("CreateEntry: Equipment already exists for item %s"), 
				*GetNameSafe(ItemInstance));
			return Entry.Instance;
		}
	}
	
	// 确定实例类型
	TSubclassOf<UYcEquipmentInstance> InstanceType = EquipmentDef.InstanceType;
	if (!InstanceType)
	{
		InstanceType = UYcEquipmentInstance::StaticClass();
	}

	// 创建装备实例
	FYcEquipmentEntry& NewEntry = Entries.AddDefaulted_GetRef();
	NewEntry.Instance = NewObject<UYcEquipmentInstance>(OwnerComponent->GetOwner(), InstanceType);
	NewEntry.Instance->SetInstigator(ItemInstance);
	NewEntry.OwnerItemInstance = ItemInstance;
	
	// 生成Actors（隐藏状态）
	NewEntry.Instance->SpawnEquipmentActors(EquipmentDef.ActorsToSpawn);
	
	// 标记网络同步
	MarkItemDirty(NewEntry);
	
	UE_LOG(LogYcEquipment, Log, TEXT("CreateEntry: Created equipment %s for item %s"), 
		*GetNameSafe(NewEntry.Instance), *GetNameSafe(ItemInstance));
	
	return NewEntry.Instance;
}

void FYcEquipmentList::EquipEntry(UYcEquipmentInstance* Instance)
{
	check(OwnerComponent);
	check(OwnerComponent->GetOwner()->HasAuthority());
	
	FYcEquipmentEntry* Entry = FindEntry(Instance);
	if (!Entry)
	{
		UE_LOG(LogYcEquipment, Error, TEXT("EquipEntry: Equipment instance not found"));
		return;
	}
	
	if (Instance->IsEquipped())
	{
		UE_LOG(LogYcEquipment, Warning, TEXT("EquipEntry: Equipment already equipped"));
		return;
	}
	
	// 授予技能
	if (UYcAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		if (const FYcEquipmentDefinition* EquipDef = Instance->GetEquipmentDef())
		{
			for (const UYcAbilitySet* AbilitySet : EquipDef->AbilitySetsToGrant)
			{
				if (AbilitySet)
				{
					AbilitySet->GiveToAbilitySystem(ASC, &Entry->GrantedHandles, Instance);
				}
			}
		}
	}
	
	// 设置状态为已装备（会触发 OnRep 和 OnEquipped）
	// 注：EquipmentState 通过 SubObjectList 复制，不需要 MarkItemDirty
	Instance->SetEquipmentState(EYcEquipmentState::Equipped);
	
	UE_LOG(LogYcEquipment, Log, TEXT("EquipEntry: Equipped %s"), *GetNameSafe(Instance));
}

void FYcEquipmentList::UnequipEntry(UYcEquipmentInstance* Instance)
{
	check(OwnerComponent);
	check(OwnerComponent->GetOwner()->HasAuthority());
	
	FYcEquipmentEntry* Entry = FindEntry(Instance);
	if (!Entry)
	{
		UE_LOG(LogYcEquipment, Error, TEXT("UnequipEntry: Equipment instance not found"));
		return;
	}
	
	if (!Instance->IsEquipped())
	{
		UE_LOG(LogYcEquipment, Warning, TEXT("UnequipEntry: Equipment already unequipped"));
		return;
	}
	
	// 移除技能
	if (UYcAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		Entry->GrantedHandles.RemoveFromAbilitySystem(ASC);
	}
	
	// 设置状态为未装备（会触发 OnRep 和 OnUnequipped）
	// 注：EquipmentState 通过 SubObjectList 复制，不需要 MarkItemDirty
	Instance->SetEquipmentState(EYcEquipmentState::Unequipped);
	
	UE_LOG(LogYcEquipment, Log, TEXT("UnequipEntry: Unequipped %s"), *GetNameSafe(Instance));
}

void FYcEquipmentList::DestroyEntry(UYcEquipmentInstance* Instance)
{
	check(OwnerComponent);
	check(OwnerComponent->GetOwner()->HasAuthority());
	
	for (auto It = Entries.CreateIterator(); It; ++It)
	{
		FYcEquipmentEntry& Entry = *It;
		if (Entry.Instance == Instance)
		{
			// 如果处于已装备状态，先卸下
			if (Instance->IsEquipped())
			{
				if (UYcAbilitySystemComponent* ASC = GetAbilitySystemComponent())
				{
					Entry.GrantedHandles.RemoveFromAbilitySystem(ASC);
				}
				Instance->SetEquipmentState(EYcEquipmentState::Unequipped);
			}
			
			// 销毁Actors
			Instance->DestroyEquipmentActors();
			
			// 从列表移除
			It.RemoveCurrent();
			MarkArrayDirty();
			
			UE_LOG(LogYcEquipment, Log, TEXT("DestroyEntry: Destroyed equipment %s"), *GetNameSafe(Instance));
			return;
		}
	}
	
	UE_LOG(LogYcEquipment, Warning, TEXT("DestroyEntry: Equipment instance not found"));
}

// ============================================================================
// UYcEquipmentManagerComponent
// ============================================================================

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
	// 服务器端清理所有装备
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		// 收集所有实例避免迭代器失效
		TArray<UYcEquipmentInstance*> AllInstances;
		for (const FYcEquipmentEntry& Entry : EquipmentList.Entries)
		{
			if (Entry.Instance)
			{
				AllInstances.Add(Entry.Instance);
			}
		}
		
		// 销毁所有装备
		for (UYcEquipmentInstance* Instance : AllInstances)
		{
			DestroyEquipment(Instance);
		}
	}
	
	Super::UninitializeComponent();
}

void UYcEquipmentManagerComponent::ReadyForReplication()
{
	Super::ReadyForReplication();
	
	// 注册已存在的装备实例用于网络复制
	if (IsUsingRegisteredSubObjectList())
	{
		for (const FYcEquipmentEntry& Entry : EquipmentList.Entries)
		{
			if (Entry.Instance)
			{
				AddReplicatedSubObject(Entry.Instance);
			}
		}
	}
}

// ============================================================================
// 主要接口
// ============================================================================

UYcEquipmentInstance* UYcEquipmentManagerComponent::CreateEquipment(
	const FYcEquipmentDefinition& EquipmentDef, 
	UYcInventoryItemInstance* ItemInstance)
{
	if (!ItemInstance)
	{
		UE_LOG(LogYcEquipment, Error, TEXT("CreateEquipment: ItemInstance is null"));
		return nullptr;
	}
	
	UYcEquipmentInstance* Instance = EquipmentList.CreateEntry(EquipmentDef, ItemInstance);
	if (!Instance)
	{
		return nullptr;
	}
	
	// 注册用于网络复制
	if (IsUsingRegisteredSubObjectList() && IsReadyForReplication())
	{
		AddReplicatedSubObject(Instance);
		AddReplicatedSubObject(ItemInstance);
	}
	
	return Instance;
}

void UYcEquipmentManagerComponent::EquipItem(UYcEquipmentInstance* EquipmentInstance)
{
	if (!EquipmentInstance)
	{
		UE_LOG(LogYcEquipment, Error, TEXT("EquipItem: EquipmentInstance is null"));
		return;
	}
	
	EquipmentList.EquipEntry(EquipmentInstance);
}

void UYcEquipmentManagerComponent::UnequipItem(UYcEquipmentInstance* EquipmentInstance)
{
	if (!EquipmentInstance)
	{
		UE_LOG(LogYcEquipment, Error, TEXT("UnequipItem: EquipmentInstance is null"));
		return;
	}
	
	EquipmentList.UnequipEntry(EquipmentInstance);
}

void UYcEquipmentManagerComponent::DestroyEquipment(UYcEquipmentInstance* EquipmentInstance)
{
	if (!EquipmentInstance)
	{
		UE_LOG(LogYcEquipment, Error, TEXT("DestroyEquipment: EquipmentInstance is null"));
		return;
	}
	
	// 取消注册网络复制
	if (IsUsingRegisteredSubObjectList())
	{
		RemoveReplicatedSubObject(EquipmentInstance);
	}
	
	EquipmentList.DestroyEntry(EquipmentInstance);
}

// ============================================================================
// 查询接口
// ============================================================================

UYcEquipmentInstance* UYcEquipmentManagerComponent::FindEquipmentByItem(UYcInventoryItemInstance* ItemInstance) const
{
	if (!ItemInstance) return nullptr;
	
	for (const FYcEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (Entry.OwnerItemInstance == ItemInstance)
		{
			return Entry.Instance;
		}
	}
	return nullptr;
}

UYcEquipmentInstance* UYcEquipmentManagerComponent::GetEquippedItem() const
{
	for (const FYcEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (Entry.Instance && Entry.Instance->IsEquipped())
		{
			return Entry.Instance;
		}
	}
	return nullptr;
}

UYcEquipmentInstance* UYcEquipmentManagerComponent::GetFirstInstanceOfType(const TSubclassOf<UYcEquipmentInstance> InstanceType) const
{
	for (const FYcEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (Entry.Instance && Entry.Instance->IsA(InstanceType))
		{
			return Entry.Instance;
		}
	}
	return nullptr;
}

TArray<UYcEquipmentInstance*> UYcEquipmentManagerComponent::GetEquipmentInstancesOfType(const TSubclassOf<UYcEquipmentInstance> InstanceType) const
{
	TArray<UYcEquipmentInstance*> Results;
	for (const FYcEquipmentEntry& Entry : EquipmentList.Entries)
	{
		if (Entry.Instance && Entry.Instance->IsA(InstanceType))
		{
			Results.Add(Entry.Instance);
		}
	}
	return Results;
}
