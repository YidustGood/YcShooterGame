// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcQuickBarComponent.h"
#include "NativeGameplayTags.h"
#include "YcEquipmentManagerComponent.h"
#include "YcInventoryItemInstance.h"
#include "YiChenEquipment.h"
#include "Fragments/InventoryFragment_Equippable.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcQuickBarComponent)

// 声明Gameplay tag，用于同UGameplayMessageSubsystem进行消息广播,需要导入头文件 #include "NativeGameplayTags.h"
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Yc_QuickBar_Message_SlotsChanged, "Yc.QuickBar.Message.SlotsChanged");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Yc_QuickBar_Message_ActiveIndexChanged, "Yc.QuickBar.Message.ActiveIndexChanged");

UYcQuickBarComponent::UYcQuickBarComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// PrimaryActorTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UYcQuickBarComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	// 使用PushModel方式进行网络复制
	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(UYcQuickBarComponent, Slots, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UYcQuickBarComponent, ActiveSlotIndex, SharedParams);
}

void UYcQuickBarComponent::BeginPlay()
{
	if (Slots.Num() < NumSlots)
	{
		Slots.AddDefaulted(NumSlots - Slots.Num()); //开辟出指定数量的插槽，用默认对象填充
		MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuickBarComponent, Slots, this);
	}

	Super::BeginPlay();
}

void UYcQuickBarComponent::SetActiveSlotIndex_Implementation(const int32 NewIndex)
{
	if (Slots.IsValidIndex(NewIndex) && (ActiveSlotIndex != NewIndex))
	{
		// 取消装备当前插槽的物品
		UnequipItemInSlot();

		// 修改当前激活的SlotIndex
		SetActiveSlotIndex_Internal(NewIndex);

		// 装备当前激活的插槽
		EquipItemInSlot();
	}
}

void UYcQuickBarComponent::DeactivateSlotIndex_Implementation(const int32 NewIndex)
{
	if (Slots.IsValidIndex(NewIndex) && (ActiveSlotIndex == NewIndex))
	{
		// 取消装备当前插槽的物品
		UnequipItemInSlot();

		// 修改当前激活的SlotIndex
		SetActiveSlotIndex_Internal(-1);
	}
}

void UYcQuickBarComponent::CycleActiveSlotForward()
{
	if (Slots.Num() < 2) return;
	const int32 OldIndex = (ActiveSlotIndex < 0 ? Slots.Num() - 1 : ActiveSlotIndex);
	int32 NewIndex = ActiveSlotIndex;
	do
	{
		NewIndex = (NewIndex + 1) % Slots.Num();
		if (Slots[NewIndex] != nullptr)
		{
			SetActiveSlotIndex(NewIndex);
			return;
		}
	}
	while (NewIndex != OldIndex);
}

void UYcQuickBarComponent::CycleActiveSlotBackward()
{
	if (Slots.Num() < 2) return;

	const int32 OldIndex = (ActiveSlotIndex < 0 ? Slots.Num() - 1 : ActiveSlotIndex);
	int32 NewIndex = ActiveSlotIndex;
	do
	{
		NewIndex = (NewIndex - 1 + Slots.Num()) % Slots.Num();
		if (Slots[NewIndex] != nullptr)
		{
			SetActiveSlotIndex(NewIndex);
			return;
		}
	}
	while (NewIndex != OldIndex);
}

UYcInventoryItemInstance* UYcQuickBarComponent::GetActiveSlotItem() const
{
	return Slots.IsValidIndex(ActiveSlotIndex) ? Slots[ActiveSlotIndex] : nullptr;
}

int32 UYcQuickBarComponent::GetNextFreeItemSlot() const
{
	int32 SlotIndex = 0;
	for (TObjectPtr<UYcInventoryItemInstance> ItemPtr : Slots)
	{
		if (ItemPtr == nullptr) return SlotIndex;
		++SlotIndex;
	}

	return INDEX_NONE;
}

UYcInventoryItemInstance* UYcQuickBarComponent::GetSlotItem(const int32 ItemIndex) const
{
	return Slots.IsValidIndex(ItemIndex) ? Slots[ItemIndex] : nullptr;
}

bool UYcQuickBarComponent::AddItemToSlot(const int32 SlotIndex, UYcInventoryItemInstance* Item)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogYcEquipment, Warning, TEXT("请在权威服务器上调用AddItemToSlot()函数向QuickBar添加物品."));
		return false;
	}

	if (Slots.IsValidIndex(SlotIndex) && Item != nullptr && Slots[SlotIndex] == nullptr)
	{
		SetSlotsByIndex_Internal(SlotIndex, Item);
		return true;
	}
	return false;
}

UYcInventoryItemInstance* UYcQuickBarComponent::RemoveItemFromSlot(int32 SlotIndex)
{
	UYcInventoryItemInstance* Result = nullptr;

	if (ActiveSlotIndex == SlotIndex) // 如果要移除的是目前激活的插槽，那么先将该插槽的物品取消装备，并重置ActiveSlotIndex
	{
		UnequipItemInSlot();
		SetActiveSlotIndex_Internal(-1);
	}

	if (Slots.IsValidIndex(SlotIndex))
	{
		Result = Slots[SlotIndex];

		if (Result != nullptr)
		{
			SetSlotsByIndex_Internal(SlotIndex, nullptr);
		}
	}

	return Result;
}

void UYcQuickBarComponent::EquipItemInSlot()
{
	check(Slots.IsValidIndex(ActiveSlotIndex));
	check(EquippedInst == nullptr);

	// 从插槽列中获取要激活的插槽物品
	UYcInventoryItemInstance* SlotItem = Slots[ActiveSlotIndex];
	if (!IsValid(SlotItem)) return;

	// 在库存物品对象上查找装备定义片段
	const FInventoryFragment_Equippable* Equippable = SlotItem->GetTypedFragment<FInventoryFragment_Equippable>();
	if (!Equippable) return;

	// 获得装备管理组件
	UYcEquipmentManagerComponent* EquipmentManager = FindEquipmentManager();
	
	if (!EquipmentManager) return;
	// 装备指定类型的装备
	EquippedInst = EquipmentManager->EquipItemByEquipmentDef(Equippable->EquipmentDef, SlotItem);
}

void UYcQuickBarComponent::UnequipItemInSlot()
{
	if (UYcEquipmentManagerComponent* EquipmentManager = FindEquipmentManager())
	{
		if (EquippedInst == nullptr) return;

		EquipmentManager->UnequipItem(EquippedInst);
		EquippedInst = nullptr;
	}
}


void UYcQuickBarComponent::SetSlotsByIndex_Internal(const int32 Index, UYcInventoryItemInstance* InItem)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Slots.IsValidIndex(Index)) return;
	UYcInventoryItemInstance* OldItem = Slots[Index];
	Slots[Index] = InItem ? InItem : nullptr;

	if (Slots[Index] == OldItem) return; //	当真的发生了变化才进行网络同步和通知

	MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuickBarComponent, Slots, this);
	OnRep_Slots(); // 由于C++中服务端修改了属性不会调用OnRep函数，所以需要手动调用一下。
}

void UYcQuickBarComponent::SetActiveSlotIndex_Internal(int32 NewIndex)
{
	if (NewIndex > NumSlots) return;
	const int32 OldIndex = ActiveSlotIndex;
	ActiveSlotIndex = NewIndex;
	if (ActiveSlotIndex == OldIndex) return;

	if (GetOwner() && GetOwner()->HasAuthority())
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuickBarComponent, ActiveSlotIndex, this);
	}
	OnRep_ActiveSlotIndex(OldIndex);
}

UYcEquipmentManagerComponent* UYcQuickBarComponent::FindEquipmentManager() const
{
	if (const AController* OwnerController = Cast<AController>(GetOwner()))
	{
		if (const APawn* Pawn = OwnerController->GetPawn())
		{
			if (UYcEquipmentManagerComponent* EquipmentManager = Pawn->FindComponentByClass<UYcEquipmentManagerComponent>()) return EquipmentManager;
		}
	}

	if (const AActor* Actor = GetOwner())
	{
		return Actor->FindComponentByClass<UYcEquipmentManagerComponent>();
	}
	UE_LOG(LogYcEquipment, Warning, TEXT("未能找到EquipmentManagerComponent,请检查Pawn或者PlayerController是否拥有装备管理组件."));
	return nullptr;
}

void UYcQuickBarComponent::OnRep_Slots()
{
	// 使用 GameplayMessageRouter插件中的UGameplayMessageSubsystem子系统进行消息广播
	FYcQuickBarSlotsChangedMessage Message;
	Message.Owner = GetOwner();
	Message.Slots = Slots;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(TAG_Yc_QuickBar_Message_SlotsChanged, Message);
}

void UYcQuickBarComponent::OnRep_ActiveSlotIndex(int32 InLastActiveSlotIndex)
{
	LastActiveSlotIndex = InLastActiveSlotIndex;
	FYcQuickBarActiveIndexChangedMessage Message;
	Message.Owner = GetOwner();
	Message.ActiveIndex = ActiveSlotIndex;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(TAG_Yc_QuickBar_Message_ActiveIndexChanged, Message);
}