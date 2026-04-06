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
#include "YcInventoryOperationRouterComponent.h"
#include "Fragments/InventoryFragment_Equippable.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"
#include "NativeGameplayTags.h"
#include "YiChenEquipment.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcEquipmentSlotComponent)

// ============================================================================
// Gameplay Tags
// ============================================================================

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Yc_EquipmentSlot_Message_SlotChanged, "Yc.EquipmentSlot.Message.SlotChanged");

namespace
{
	static const FName OpType_Equipment_Equip(TEXT("Equipment.Equip"));
	static const FName OpType_Equipment_Unequip(TEXT("Equipment.Unequip"));
}

// ============================================================================
// 构造函数和生命周期
// ============================================================================

UYcEquipmentSlotComponent::UYcEquipmentSlotComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
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

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		RegisterInventoryOperationHandlers();
		SetComponentTickEnabled(!bOperationHandlersRegistered);
	}
}

void UYcEquipmentSlotComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterInventoryOperationHandlers();
	Super::EndPlay(EndPlayReason);
}

void UYcEquipmentSlotComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!GetOwner() || !GetOwner()->HasAuthority() || bOperationHandlersRegistered)
	{
		return;
	}

	RegisterInventoryOperationHandlers();
	if (bOperationHandlersRegistered)
	{
		SetComponentTickEnabled(false);
	}
}

void UYcEquipmentSlotComponent::RegisterInventoryOperationHandlers()
{
	if (bOperationHandlersRegistered)
	{
		return;
	}

	UYcInventoryOperationRouterComponent* Router = UYcInventoryOperationRouterComponent::FindOrCreateRouter(GetOwner());
	if (!Router)
	{
		UE_LOG(LogYcEquipment, Verbose, TEXT("RegisterInventoryOperationHandlers deferred: router missing (likely not possessed yet)."));
		return;
	}

	FYcInventoryOperationHandler EquipHandler;
	EquipHandler.Validate.BindUObject(this, &UYcEquipmentSlotComponent::ValidateEquipOperation);
	EquipHandler.Execute.BindUObject(this, &UYcEquipmentSlotComponent::ExecuteEquipOperation);
	EquipHandler.BuildDelta.BindUObject(this, &UYcEquipmentSlotComponent::BuildEquipOperationDelta);
	EquipHandler.ProjectState.BindUObject(this, &UYcEquipmentSlotComponent::ProjectEquipOperationState);
	Router->RegisterOperationHandler(OpType_Equipment_Equip, EquipHandler);

	FYcInventoryOperationHandler UnequipHandler;
	UnequipHandler.Validate.BindUObject(this, &UYcEquipmentSlotComponent::ValidateUnequipOperation);
	UnequipHandler.Execute.BindUObject(this, &UYcEquipmentSlotComponent::ExecuteUnequipOperation);
	UnequipHandler.BuildDelta.BindUObject(this, &UYcEquipmentSlotComponent::BuildUnequipOperationDelta);
	UnequipHandler.ProjectState.BindUObject(this, &UYcEquipmentSlotComponent::ProjectUnequipOperationState);
	Router->RegisterOperationHandler(OpType_Equipment_Unequip, UnequipHandler);
	bOperationHandlersRegistered = true;
}

void UYcEquipmentSlotComponent::UnregisterInventoryOperationHandlers()
{
	if (UYcInventoryOperationRouterComponent* Router = UYcInventoryOperationRouterComponent::FindRouter(GetOwner()))
	{
		Router->UnregisterOperationHandler(OpType_Equipment_Equip);
		Router->UnregisterOperationHandler(OpType_Equipment_Unequip);
	}
	bOperationHandlersRegistered = false;
}

bool UYcEquipmentSlotComponent::ValidateEquipOperation(const FYcInventoryOperation& Operation, FString& OutReason) const
{
	if (!Operation.ItemInstance)
	{
		OutReason = TEXT("Equipment.Equip missing item.");
		return false;
	}
	if (!Operation.SourceInventory)
	{
		OutReason = TEXT("Equipment.Equip missing source inventory.");
		return false;
	}

	UYcInventoryManagerComponent* CurrentSource = UYcInventoryManagerComponent::FindInventoryManagerByItem(Operation.ItemInstance);
	if (!CurrentSource)
	{
		OutReason = TEXT("Equipment.Equip source inventory missing.");
		return false;
	}
	if (CurrentSource != Operation.SourceInventory)
	{
		OutReason = TEXT("Equipment.Equip source changed.");
		return false;
	}
	return true;
}

bool UYcEquipmentSlotComponent::ExecuteEquipOperation(const FYcInventoryOperation& Operation, FString& OutReason)
{
	if (!EquipItem(Operation.ItemInstance))
	{
		OutReason = TEXT("EquipItem failed.");
		return false;
	}
	OutReason = TEXT("Equipment equip success.");
	return true;
}

void UYcEquipmentSlotComponent::BuildEquipOperationDelta(const FYcInventoryOperation& Operation, const bool bSuccess, FYcInventoryOperationDelta& OutDelta) const
{
	OutDelta.Summary = bSuccess ? TEXT("Equipment equip success.") : TEXT("Equipment equip failed.");
	if (Operation.SlotTag.IsValid())
	{
		OutDelta.AffectedSlots.Add(Operation.SlotTag.GetTagName());
	}
	if (Operation.ItemInstance)
	{
		OutDelta.AffectedItemIds.Add(Operation.ItemInstance->GetItemInstId());
	}
}

void UYcEquipmentSlotComponent::ProjectEquipOperationState(const FYcInventoryOperation& Operation, FYcInventoryProjectedState& InOutProjectedState) const
{
	if (Operation.SlotTag.IsValid())
	{
		InOutProjectedState.EquipmentSlots.Add(Operation.SlotTag, Operation.ItemInstance);
	}
}

bool UYcEquipmentSlotComponent::ValidateUnequipOperation(const FYcInventoryOperation& Operation, FString& OutReason) const
{
	if (!Operation.SlotTag.IsValid())
	{
		OutReason = TEXT("Equipment.Unequip missing SlotTag.");
		return false;
	}
	return true;
}

bool UYcEquipmentSlotComponent::ExecuteUnequipOperation(const FYcInventoryOperation& Operation, FString& OutReason)
{
	if (!UnequipSlot(Operation.SlotTag))
	{
		OutReason = TEXT("UnequipSlot failed.");
		return false;
	}
	OutReason = TEXT("Equipment unequip success.");
	return true;
}

void UYcEquipmentSlotComponent::BuildUnequipOperationDelta(const FYcInventoryOperation& Operation, const bool bSuccess, FYcInventoryOperationDelta& OutDelta) const
{
	OutDelta.Summary = bSuccess ? TEXT("Equipment unequip success.") : TEXT("Equipment unequip failed.");
	if (Operation.SlotTag.IsValid())
	{
		OutDelta.AffectedSlots.Add(Operation.SlotTag.GetTagName());
	}
	if (Operation.ItemInstance)
	{
		OutDelta.AffectedItemIds.Add(Operation.ItemInstance->GetItemInstId());
	}
}

void UYcEquipmentSlotComponent::ProjectUnequipOperationState(const FYcInventoryOperation& Operation, FYcInventoryProjectedState& InOutProjectedState) const
{
	if (Operation.SlotTag.IsValid())
	{
		InOutProjectedState.EquipmentSlots.Add(Operation.SlotTag, nullptr);
	}
}

// ============================================================================
// 装备接口
// ============================================================================

bool UYcEquipmentSlotComponent::EquipItem(UYcInventoryItemInstance* ItemInstance)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogYcEquipment, Warning, TEXT("EquipItem: Must be called on server"));
		return false;
	}

	if (!ItemInstance)
	{
		UE_LOG(LogYcEquipment, Warning, TEXT("EquipItem: ItemInstance is null"));
		return false;
	}

	// 获取物品应装备到的槽位
	FGameplayTag TargetSlotTag = GetEquipmentSlotTag(ItemInstance);
	if (!TargetSlotTag.IsValid())
	{
		UE_LOG(LogYcEquipment, Warning, TEXT("EquipItem: Item '%s' has no equipment slot defined. Check EquipmentDefinition.EquipmentSlot in DataTable."), 
			*ItemInstance->GetItemRegistryId().ToString());
		return false;
	}

	// 查找槽位索引
	int32 SlotIndex = FindSlotIndex(TargetSlotTag);
	if (SlotIndex == INDEX_NONE)
	{
		UE_LOG(LogYcEquipment, Warning, TEXT("EquipItem: Slot '%s' not found in configuration for item '%s'"), 
			*TargetSlotTag.ToString(), *ItemInstance->GetItemRegistryId().ToString());
		return false;
	}

	// 获取 Inventory 和 EquipmentManager
	UYcInventoryManagerComponent* InventoryManager = UYcInventoryManagerComponent::FindInventoryManagerByItem(ItemInstance);
	if (InventoryManager == nullptr)
	{
		InventoryManager = GetInventoryManager();
	}
	UYcEquipmentManagerComponent* EquipmentManager = GetEquipmentManager();
	
	if (!EquipmentManager)
	{
		UE_LOG(LogYcEquipment, Warning, TEXT("EquipItem: EquipmentManager not found"));
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
		if (!InventoryManager->RemoveItemInstance(ItemInstance))
		{
			UE_LOG(LogYcEquipment, Warning, TEXT("EquipItem: Failed to remove item '%s' from source inventory"),
				*ItemInstance->GetItemRegistryId().ToString());
			return false;
		}
	}
	else
	{
		UE_LOG(LogYcEquipment, Warning, TEXT("EquipItem: Source InventoryManager not found for item '%s'"),
			*ItemInstance->GetItemRegistryId().ToString());
		return false;
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

	UE_LOG(LogYcEquipment, Log, TEXT("EquipItem: Item '%s' equipped to slot '%s'"), 
		*ItemInstance->GetItemRegistryId().ToString(), *TargetSlotTag.ToString());

	return true;
}

UYcInventoryItemInstance* UYcEquipmentSlotComponent::UnequipSlot(FGameplayTag SlotTag)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogYcEquipment, Warning, TEXT("UnequipSlot: Must be called on server"));
		return nullptr;
	}

	if (!SlotTag.IsValid())
	{
		UE_LOG(LogYcEquipment, Warning, TEXT("UnequipSlot: SlotTag is invalid"));
		return nullptr;
	}

	// 查找槽位索引
	int32 SlotIndex = FindSlotIndex(SlotTag);
	if (SlotIndex == INDEX_NONE)
	{
		UE_LOG(LogYcEquipment, Warning, TEXT("UnequipSlot: Slot %s not found"), *SlotTag.ToString());
		return nullptr;
	}

	// 获取槽位中的物品
	UYcInventoryItemInstance* ItemInstance = Slots[SlotIndex].ItemInstance;
	if (!ItemInstance)
	{
		UE_LOG(LogYcEquipment, Verbose, TEXT("UnequipSlot: Slot '%s' is already empty"), *SlotTag.ToString());
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

	UE_LOG(LogYcEquipment, Log, TEXT("UnequipSlot: Item '%s' unequipped from slot '%s'"), 
		*ItemInstance->GetItemRegistryId().ToString(), *SlotTag.ToString());

	return ItemInstance;
}

void UYcEquipmentSlotComponent::ServerEquipItem_Implementation(UYcInventoryItemInstance* ItemInstance)
{
	if (UYcInventoryManagerComponent* InventoryManager = GetInventoryManager())
	{
		FYcInventoryOperation Op;
		Op.OpType = FName(TEXT("Equipment.Equip"));
		Op.ItemInstance = ItemInstance;
		Op.SlotTag = GetEquipmentSlotTag(ItemInstance);
		Op.RequestActor = GetOwner();
		Op.SourceInventory = InventoryManager;
		Op.TargetInventory = InventoryManager;
		if (UYcInventoryOperationRouterComponent* Router = UYcInventoryOperationRouterComponent::FindOrCreateRouter(GetOwner()))
		{
			Router->SubmitInventoryOperation(InventoryManager, Op, false);
			return;
		}

		UE_LOG(LogYcEquipment, Warning, TEXT("ServerEquipItem rejected: inventory router missing."));
		return;
	}

	EquipItem(ItemInstance);
}

void UYcEquipmentSlotComponent::ServerUnequipSlot_Implementation(FGameplayTag SlotTag)
{
	if (UYcInventoryManagerComponent* InventoryManager = GetInventoryManager())
	{
		FYcInventoryOperation Op;
		Op.OpType = FName(TEXT("Equipment.Unequip"));
		Op.SlotTag = SlotTag;
		Op.RequestActor = GetOwner();
		Op.SourceInventory = InventoryManager;
		Op.TargetInventory = InventoryManager;
		if (UYcInventoryOperationRouterComponent* Router = UYcInventoryOperationRouterComponent::FindOrCreateRouter(GetOwner()))
		{
			Router->SubmitInventoryOperation(InventoryManager, Op, false);
			return;
		}

		UE_LOG(LogYcEquipment, Warning, TEXT("ServerUnequipSlot rejected: inventory router missing."));
		return;
	}

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
	// 客户端收到槽位复制后，广播当前状态驱动UI刷新。
	// 注意：服务端在Equip/Unequip时的Broadcast不会自动跨网络到客户端。
	UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(GetWorld());
	for (const FYcEquipmentSlot& Slot : Slots)
	{
		if (!Slot.SlotTag.IsValid())
		{
			continue;
		}

		FYcEquipmentSlotChangedMessage Message;
		Message.Owner = GetOwner();
		Message.SlotTag = Slot.SlotTag;
		Message.ItemInstance = Slot.ItemInstance;
		MessageSubsystem.BroadcastMessage(TAG_Yc_EquipmentSlot_Message_SlotChanged, Message);
	}
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
