// Copyright (c) 2025 YiChen. All Rights Reserved.

/**
 * ============================================================================
 * UYcEquipmentSlotComponent 实现
 * ============================================================================
 * 
 * 本文件实现了装备栏槽位管理，与 InventoryManager 和 EquipmentManager 协作：
 * - 装备时：从 Inventory 移出 → 放入 Slot → 通知 EquipmentManager 装备
 * - 卸下时：通知 EquipmentManager 卸下 → 从 Slot 移出 → 回归 Inventory
 * 
 * ============================================================================
 */

#include "YcEquipmentSlotComponent.h"

#include "YcEquipmentInstance.h"
#include "YcEquipmentManagerComponent.h"
#include "YcInventoryItemInstance.h"
#include "YcInventoryManagerComponent.h"
#include "Fragments/InventoryFragment_Equippable.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "NativeGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcEquipmentSlotComponent)

// ============================================================================
// Gameplay Tags
// ============================================================================

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Yc_EquipmentSlot_Message_SlotChanged, "Yc.EquipmentSlot.Message.SlotChanged");

// ============================================================================
// 构造函数和生命周期
// ============================================================================

UYcEquipmentSlotComponent::UYcEquipmentSlotComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UYcEquipmentSlotComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(UYcEquipmentSlotComponent, Slots, SharedParams);
}

void UYcEquipmentSlotComponent::BeginPlay()
{
	Super::BeginPlay();
}

// ============================================================================
// 装备接口
// ============================================================================

bool UYcEquipmentSlotComponent::EquipItem(UYcInventoryItemInstance* ItemInstance)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipItem: Must be called on server"));
		return false;
	}

	if (!ItemInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipItem: ItemInstance is null"));
		return false;
	}

	// 获取物品应装备到的槽位
	FGameplayTag TargetSlotTag = GetEquipmentSlotTag(ItemInstance);
	if (!TargetSlotTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipItem: Item '%s' has no equipment slot defined. Check EquipmentDefinition.EquipmentSlot in DataTable."), 
			*ItemInstance->GetItemRegistryId().ToString());
		return false;
	}

	// 查找槽位索引
	int32 SlotIndex = FindSlotIndex(TargetSlotTag);
	if (SlotIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipItem: Slot '%s' not found in configuration for item '%s'"), 
			*TargetSlotTag.ToString(), *ItemInstance->GetItemRegistryId().ToString());
		return false;
	}

	// 获取 Inventory 和 EquipmentManager
	UYcInventoryManagerComponent* InventoryManager = GetInventoryManager();
	UYcEquipmentManagerComponent* EquipmentManager = GetEquipmentManager();
	
	if (!EquipmentManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("EquipItem: EquipmentManager not found"));
		return false;
	}

	// 检查槽位是否已被占用
	if (Slots[SlotIndex].ItemInstance != nullptr)
	{
		// 槽位已被占用，先卸下旧装备
		UnequipSlot(TargetSlotTag);
	}

	// 从 Inventory 移出物品
	if (InventoryManager)
	{
		InventoryManager->RemoveItemInstance(ItemInstance);
	}

	// 放入槽位
	SetSlotItem_Internal(SlotIndex, ItemInstance);

	// 创建装备实例并装备
	UYcEquipmentInstance* EquipmentInstance = EquipmentManager->CreateEquipment(ItemInstance);
	if (EquipmentInstance)
	{
		EquipmentManager->EquipItem(EquipmentInstance);
	}

	// 广播消息
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(GetWorld());
	FYcEquipmentSlotChangedMessage Message;
	Message.Owner = GetOwner();
	Message.SlotTag = TargetSlotTag;
	Message.ItemInstance = ItemInstance;
	MessageSubsystem.BroadcastMessage(TAG_Yc_EquipmentSlot_Message_SlotChanged, Message);

	UE_LOG(LogTemp, Log, TEXT("EquipItem: Item '%s' equipped to slot '%s'"), 
		*ItemInstance->GetItemRegistryId().ToString(), *TargetSlotTag.ToString());

	return true;
}

UYcInventoryItemInstance* UYcEquipmentSlotComponent::UnequipSlot(FGameplayTag SlotTag)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("UnequipSlot: Must be called on server"));
		return nullptr;
	}

	if (!SlotTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("UnequipSlot: SlotTag is invalid"));
		return nullptr;
	}

	// 查找槽位索引
	int32 SlotIndex = FindSlotIndex(SlotTag);
	if (SlotIndex == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("UnequipSlot: Slot %s not found"), *SlotTag.ToString());
		return nullptr;
	}

	// 获取槽位中的物品
	UYcInventoryItemInstance* ItemInstance = Slots[SlotIndex].ItemInstance;
	if (!ItemInstance)
	{
		UE_LOG(LogTemp, Verbose, TEXT("UnequipSlot: Slot '%s' is already empty"), *SlotTag.ToString());
		return nullptr;
	}

	// 获取 EquipmentManager
	UYcEquipmentManagerComponent* EquipmentManager = GetEquipmentManager();
	if (EquipmentManager)
	{
		// 查找并卸下装备实例
		if (UYcEquipmentInstance* EquipmentInstance = EquipmentManager->FindEquipmentByItem(ItemInstance))
		{
			EquipmentManager->UnequipItem(EquipmentInstance);
			EquipmentManager->DestroyEquipment(EquipmentInstance);
		}
	}

	// 清空槽位
	SetSlotItem_Internal(SlotIndex, nullptr);

	// 回归 Inventory
	UYcInventoryManagerComponent* InventoryManager = GetInventoryManager();
	if (InventoryManager)
	{
		InventoryManager->AddItemInstance(ItemInstance, 1);
	}

	// 广播消息
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(GetWorld());
	FYcEquipmentSlotChangedMessage Message;
	Message.Owner = GetOwner();
	Message.SlotTag = SlotTag;
	Message.ItemInstance = nullptr;
	MessageSubsystem.BroadcastMessage(TAG_Yc_EquipmentSlot_Message_SlotChanged, Message);

	UE_LOG(LogTemp, Log, TEXT("UnequipSlot: Item '%s' unequipped from slot '%s'"), 
		*ItemInstance->GetItemRegistryId().ToString(), *SlotTag.ToString());

	return ItemInstance;
}

void UYcEquipmentSlotComponent::ServerEquipItem_Implementation(UYcInventoryItemInstance* ItemInstance)
{
	EquipItem(ItemInstance);
}

void UYcEquipmentSlotComponent::ServerUnequipSlot_Implementation(FGameplayTag SlotTag)
{
	UnequipSlot(SlotTag);
}

void UYcEquipmentSlotComponent::UnequipAll()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	// 收集所有已占用的槽位
	TArray<FGameplayTag> OccupiedSlots = GetOccupiedSlots();
	
	// 逐个卸下（从后往前卸，避免迭代时修改）
	for (int32 i = OccupiedSlots.Num() - 1; i >= 0; --i)
	{
		UnequipSlot(OccupiedSlots[i]);
	}
}

// ============================================================================
// 查询接口
// ============================================================================

bool UYcEquipmentSlotComponent::IsSlotOccupied(FGameplayTag SlotTag) const
{
	return GetItemInSlot(SlotTag) != nullptr;
}

UYcInventoryItemInstance* UYcEquipmentSlotComponent::GetItemInSlot(FGameplayTag SlotTag) const
{
	int32 SlotIndex = FindSlotIndex(SlotTag);
	if (SlotIndex != INDEX_NONE && Slots.IsValidIndex(SlotIndex))
	{
		return Slots[SlotIndex].ItemInstance;
	}
	return nullptr;
}

TArray<FGameplayTag> UYcEquipmentSlotComponent::GetOccupiedSlots() const
{
	TArray<FGameplayTag> OccupiedSlots;
	for (const FYcEquipmentSlot& Slot : Slots)
	{
		if (Slot.ItemInstance != nullptr && Slot.SlotTag.IsValid())
		{
			OccupiedSlots.Add(Slot.SlotTag);
		}
	}
	return OccupiedSlots;
}

bool UYcEquipmentSlotComponent::GetSlotConfig(FGameplayTag SlotTag, FYcEquipmentSlot& OutSlotConfig) const
{
	int32 SlotIndex = FindSlotIndex(SlotTag);
	if (SlotIndex != INDEX_NONE && Slots.IsValidIndex(SlotIndex))
	{
		OutSlotConfig = Slots[SlotIndex];
		return true;
	}
	return false;
}

// ============================================================================
// 静态查找函数
// ============================================================================

UYcEquipmentSlotComponent* UYcEquipmentSlotComponent::FindEquipmentSlotComponent(const AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}

	// 如果是 Pawn，直接查找
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		return Pawn->FindComponentByClass<UYcEquipmentSlotComponent>();
	}

	// 如果是 Controller，从 Pawn 查找
	if (const AController* Controller = Cast<AController>(Actor))
	{
		if (const APawn* Pawn = Controller->GetPawn())
		{
			return Pawn->FindComponentByClass<UYcEquipmentSlotComponent>();
		}
	}

	return nullptr;
}

// ============================================================================
// 网络复制回调
// ============================================================================

void UYcEquipmentSlotComponent::OnRep_Slots()
{
	// 客户端同步槽位变化
	// 可以在这里触发 UI 更新
}

// ============================================================================
// 内部实现
// ============================================================================

int32 UYcEquipmentSlotComponent::FindSlotIndex(FGameplayTag SlotTag) const
{
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i].SlotTag == SlotTag)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

void UYcEquipmentSlotComponent::SetSlotItem_Internal(int32 SlotIndex, UYcInventoryItemInstance* Item)
{
	if (!Slots.IsValidIndex(SlotIndex))
	{
		return;
	}

	Slots[SlotIndex].ItemInstance = Item;
	MARK_PROPERTY_DIRTY_FROM_NAME(UYcEquipmentSlotComponent, Slots, this);
}

UYcInventoryManagerComponent* UYcEquipmentSlotComponent::GetInventoryManager() const
{
	return UYcInventoryManagerComponent::FindInventoryManager(GetOwner());
}

UYcEquipmentManagerComponent* UYcEquipmentSlotComponent::GetEquipmentManager() const
{
	return UYcEquipmentManagerComponent::FindEquipmentManager(GetOwner());
}

FGameplayTag UYcEquipmentSlotComponent::GetEquipmentSlotTag(UYcInventoryItemInstance* ItemInstance) const
{
	if (!ItemInstance)
	{
		return FGameplayTag();
	}

	// 从 Equippable Fragment 获取装备定义
	const FInventoryFragment_Equippable* Equippable = ItemInstance->GetTypedFragment<FInventoryFragment_Equippable>();
	if (Equippable)
	{
		return Equippable->EquipmentDef.EquipmentSlot;
	}

	return FGameplayTag();
}
