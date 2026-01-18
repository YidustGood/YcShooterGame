// Copyright (c) 2025 YiChen. All Rights Reserved.

/**
 * ============================================================================
 * UYcQuickBarComponent 实现
 * ============================================================================
 * 
 * 本文件实现了带客户端预测的快捷物品栏切换机制。
 * 
 * 【核心设计理念】
 * 1. 预创建：物品加入QuickBar时就创建装备实例，消除首次装备延迟
 * 2. 状态驱动：通过 EquipmentState 控制装备的激活/停用
 * 3. 客户端预测：主控客户端立即显示切换效果，无需等待服务器响应
 * 4. 状态校正：服务器响应后，客户端校正预测状态
 * 
 * ============================================================================
 */

#include "YcQuickBarComponent.h"

#include "NativeGameplayTags.h"
#include "YcEquipmentInstance.h"
#include "YcEquipmentManagerComponent.h"
#include "YcInventoryItemInstance.h"
#include "YiChenEquipment.h"
#include "Fragments/InventoryFragment_Equippable.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcQuickBarComponent)

// ============================================================================
// Gameplay Tags
// ============================================================================

UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Yc_QuickBar_Message_SlotsChanged, "Yc.QuickBar.Message.SlotsChanged");
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_Yc_QuickBar_Message_ActiveIndexChanged, "Yc.QuickBar.Message.ActiveIndexChanged");

// ============================================================================
// 构造函数和生命周期
// ============================================================================

UYcQuickBarComponent::UYcQuickBarComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsReplicatedByDefault(true);
}

void UYcQuickBarComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	FDoRepLifetimeParams SharedParams;
	SharedParams.bIsPushBased = true;

	DOREPLIFETIME_WITH_PARAMS_FAST(UYcQuickBarComponent, Slots, SharedParams);
	DOREPLIFETIME_WITH_PARAMS_FAST(UYcQuickBarComponent, ActiveSlotIndex, SharedParams);
}

void UYcQuickBarComponent::BeginPlay()
{
	// 初始化插槽数组
	if (Slots.Num() < NumSlots)
	{
		Slots.AddDefaulted(NumSlots - Slots.Num());
		MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuickBarComponent, Slots, this);
	}

	Super::BeginPlay();
}

// ============================================================================
// 客户端预测接口
// ============================================================================

void UYcQuickBarComponent::SetActiveSlotIndex_WithPrediction(const int32 NewIndex)
{
	const int32 CurrentEffectiveIndex = GetActiveSlotIndex();
	if (!Slots.IsValidIndex(NewIndex) || CurrentEffectiveIndex == NewIndex)
	{
		return;
	}
	
	// 主控客户端：执行本地预测
	if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		// ========================================================================
		// 边界情况处理：装备实例尚未同步到客户端
		// ========================================================================
		// 场景：玩家拾取物品后立即按键装备，但由于网络延迟（如300ms），
		// EquipmentInstance 还在传输中，尚未到达客户端。
		// 
		// 处理策略：
		// - 如果目标装备实例不存在，跳过本次请求（不预测，不发RPC）
		// - 玩家发现没有响应会自然地再次按键
		// - 等装备实例到达后，下次按键就能正常工作
		// 
		// 这种设计的优势：
		// - 代码简洁，不需要复杂的"等待队列"或"延迟重试"机制
		// - 高延迟是极端情况，为此增加复杂度不值得
		// - 符合用户直觉：按键没反应就再按一次
		// ========================================================================
		if (!CanExecutePrediction(NewIndex))
		{
			UE_LOG(LogYcEquipment, Verbose, 
				TEXT("SetActiveSlotIndex_WithPrediction: Equipment not yet replicated for slot %d, skipping"), NewIndex);
			return;
		}
		
		ExecuteLocalPrediction(NewIndex);
		PendingRequestCount++;
	}
	
	// 发送 Server RPC
	ServerSetActiveSlotIndex(NewIndex);
}

void UYcQuickBarComponent::DeactivateSlotIndex_WithPrediction(const int32 NewIndex)
{
	const int32 CurrentEffectiveIndex = GetActiveSlotIndex();
	if (!Slots.IsValidIndex(NewIndex) || CurrentEffectiveIndex != NewIndex)
	{
		return;
	}
	
	// 主控客户端：执行本地预测
	if (IsLocallyControlled() && !GetOwner()->HasAuthority())
	{
		// 同 SetActiveSlotIndex_WithPrediction 的边界情况处理
		// 如果当前装备实例不存在，跳过预测
		if (!CanExecutePrediction(NewIndex))
		{
			UE_LOG(LogYcEquipment, Verbose, 
				TEXT("DeactivateSlotIndex_WithPrediction: Equipment not yet replicated for slot %d, skipping"), NewIndex);
			return;
		}
		
		ExecuteLocalPredictionDeactivate(NewIndex);
		PendingRequestCount++;
	}
	
	// 发送 Server RPC
	ServerDeactivateSlotIndex(NewIndex);
}

// ============================================================================
// Server RPC
// ============================================================================

void UYcQuickBarComponent::ServerSetActiveSlotIndex_Implementation(int32 NewIndex)
{
	if (!Slots.IsValidIndex(NewIndex) || ActiveSlotIndex == NewIndex)
	{
		return;
	}
	
	// 停用当前装备
	if (Slots.IsValidIndex(ActiveSlotIndex))
	{
		DeactivateSlotEquipment(ActiveSlotIndex);
	}
	
	// 更新索引
	SetActiveSlotIndex_Internal(NewIndex);
	
	// 激活新装备
	ActivateSlotEquipment(NewIndex);
}

void UYcQuickBarComponent::ServerDeactivateSlotIndex_Implementation(int32 NewIndex)
{
	if (!Slots.IsValidIndex(NewIndex) || ActiveSlotIndex != NewIndex)
	{
		return;
	}
	
	// 停用当前装备
	DeactivateSlotEquipment(NewIndex);
	
	// 更新索引
	SetActiveSlotIndex_Internal(-1);
}

// ============================================================================
// 循环切换
// ============================================================================

void UYcQuickBarComponent::CycleActiveSlotForward()
{
	if (Slots.Num() < 2) return;
	
	const int32 CurrentIndex = GetActiveSlotIndex();
	const int32 StartIndex = (CurrentIndex < 0 ? Slots.Num() - 1 : CurrentIndex);
	int32 NewIndex = CurrentIndex;
	
	do
	{
		NewIndex = (NewIndex + 1) % Slots.Num();
		if (Slots[NewIndex] != nullptr)
		{
			SetActiveSlotIndex_WithPrediction(NewIndex);
			return;
		}
	}
	while (NewIndex != StartIndex);
}

void UYcQuickBarComponent::CycleActiveSlotBackward()
{
	if (Slots.Num() < 2) return;

	const int32 CurrentIndex = GetActiveSlotIndex();
	const int32 StartIndex = (CurrentIndex < 0 ? Slots.Num() - 1 : CurrentIndex);
	int32 NewIndex = CurrentIndex;
	
	do
	{
		NewIndex = (NewIndex - 1 + Slots.Num()) % Slots.Num();
		if (Slots[NewIndex] != nullptr)
		{
			SetActiveSlotIndex_WithPrediction(NewIndex);
			return;
		}
	}
	while (NewIndex != StartIndex);
}

// ============================================================================
// 查询接口
// ============================================================================

int32 UYcQuickBarComponent::GetActiveSlotIndex() const
{
	if (bHasPendingSlotChange)
	{
		return PendingSlotIndex;
	}
	return ActiveSlotIndex;
}

UYcInventoryItemInstance* UYcQuickBarComponent::GetActiveSlotItem() const
{
	const int32 EffectiveIndex = GetActiveSlotIndex();
	return Slots.IsValidIndex(EffectiveIndex) ? Slots[EffectiveIndex] : nullptr;
}

int32 UYcQuickBarComponent::GetNextFreeItemSlot() const
{
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		if (Slots[i] == nullptr)
		{
			return i;
		}
	}
	return INDEX_NONE;
}

UYcInventoryItemInstance* UYcQuickBarComponent::GetSlotItem(const int32 ItemIndex) const
{
	return Slots.IsValidIndex(ItemIndex) ? Slots[ItemIndex] : nullptr;
}

UYcEquipmentInstance* UYcQuickBarComponent::GetSlotEquipmentInstance(int32 ItemIndex) const
{
	if (!Slots.IsValidIndex(ItemIndex) || !Slots[ItemIndex])
	{
		return nullptr;
	}
	
	const UYcEquipmentManagerComponent* EquipmentManager = FindEquipmentManager();
	if (!EquipmentManager)
	{
		return nullptr;
	}
	
	return EquipmentManager->FindEquipmentByItem(Slots[ItemIndex]);
}

UYcEquipmentInstance* UYcQuickBarComponent::GetActiveEquipmentInstance() const
{
	return GetSlotEquipmentInstance(GetActiveSlotIndex());
}

// ============================================================================
// 物品管理接口
// ============================================================================

bool UYcQuickBarComponent::AddItemToSlot(const int32 SlotIndex, UYcInventoryItemInstance* Item)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogYcEquipment, Warning, TEXT("AddItemToSlot: Must be called on server"));
		return false;
	}

	if (!Slots.IsValidIndex(SlotIndex) || !Item || Slots[SlotIndex] != nullptr)
	{
		return false;
	}
	
	// 设置插槽物品
	SetSlotItem_Internal(SlotIndex, Item);
	
	// 预创建装备实例（如果物品可装备）
	CreateSlotEquipment(SlotIndex);
	
	return true;
}

UYcInventoryItemInstance* UYcQuickBarComponent::RemoveItemFromSlot(int32 SlotIndex)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogYcEquipment, Warning, TEXT("RemoveItemFromSlot: Must be called on server"));
		return nullptr;
	}
	
	if (!Slots.IsValidIndex(SlotIndex))
	{
		return nullptr;
	}
	
	UYcInventoryItemInstance* RemovedItem = Slots[SlotIndex];
	if (!RemovedItem)
	{
		return nullptr;
	}
	
	// 如果是当前激活的插槽，先停用
	if (ActiveSlotIndex == SlotIndex)
	{
		DeactivateSlotEquipment(SlotIndex);
		SetActiveSlotIndex_Internal(-1);
	}
	
	// 销毁装备实例
	DestroySlotEquipment(SlotIndex);
	
	// 清除插槽
	SetSlotItem_Internal(SlotIndex, nullptr);
	
	// 清除客户端预测状态（如果有）
	if (bHasPendingSlotChange && PendingSlotIndex == SlotIndex)
	{
		bHasPendingSlotChange = false;
		PendingSlotIndex = -1;
	}
	
	return RemovedItem;
}

// ============================================================================
// 客户端预测实现
// ============================================================================

void UYcQuickBarComponent::ExecuteLocalPrediction(const int32 NewIndex)
{
	UE_LOG(LogYcEquipment, Verbose, TEXT("ExecuteLocalPrediction: %d -> %d"), GetActiveSlotIndex(), NewIndex);
	
	const UYcEquipmentManagerComponent* EquipmentManager = FindEquipmentManager();
	if (!EquipmentManager)
	{
		return;
	}
	
	// 隐藏当前装备
	const int32 CurrentIndex = bHasPendingSlotChange ? PendingSlotIndex : ActiveSlotIndex;
	if (Slots.IsValidIndex(CurrentIndex) && Slots[CurrentIndex])
	{
		if (UYcEquipmentInstance* CurrentEquipment = EquipmentManager->FindEquipmentByItem(Slots[CurrentIndex]))
		{
			// 设置状态为未装备（会触发 OnRep 和 OnUnequipped）
			CurrentEquipment->SetEquipmentState(EYcEquipmentState::Unequipped);
		}
	}
	
	// 显示新装备
	if (Slots.IsValidIndex(NewIndex) && Slots[NewIndex])
	{
		if (UYcEquipmentInstance* NewEquipment = EquipmentManager->FindEquipmentByItem(Slots[NewIndex]))
		{
			// 设置状态为已装备（会触发 OnRep 和 OnEquipped）
			NewEquipment->SetEquipmentState(EYcEquipmentState::Equipped);
		}
	}
	
	// 设置预测状态
	bHasPendingSlotChange = true;
	PendingSlotIndex = NewIndex;
	
	// 广播消息让 UI 立即响应
	FYcQuickBarActiveIndexChangedMessage Message;
	Message.Owner = GetOwner();
	Message.ActiveIndex = NewIndex;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(TAG_Yc_QuickBar_Message_ActiveIndexChanged, Message);
}

void UYcQuickBarComponent::ExecuteLocalPredictionDeactivate(int32 NewIndex)
{
	UE_LOG(LogYcEquipment, Verbose, TEXT("ExecuteLocalPredictionDeactivate: %d"), NewIndex);
	
	const UYcEquipmentManagerComponent* EquipmentManager = FindEquipmentManager();
	if (!EquipmentManager)
	{
		return;
	}
	
	// 隐藏当前装备
	if (Slots.IsValidIndex(NewIndex) && Slots[NewIndex])
	{
		if (UYcEquipmentInstance* Equipment = EquipmentManager->FindEquipmentByItem(Slots[NewIndex]))
		{
			// 设置状态为未装备（会触发 OnRep 和 OnUnequipped）
			Equipment->SetEquipmentState(EYcEquipmentState::Unequipped);
		}
	}
	
	// 将本次卸下的插槽记录为上一次激活的插槽
	LastActiveSlotIndex = NewIndex;
	
	// 设置预测状态
	bHasPendingSlotChange = true;
	PendingSlotIndex = -1;
	
	// 广播消息
	FYcQuickBarActiveIndexChangedMessage Message;
	Message.Owner = GetOwner();
	Message.ActiveIndex = -1;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(TAG_Yc_QuickBar_Message_ActiveIndexChanged, Message);
}

void UYcQuickBarComponent::ReconcilePrediction(int32 ServerIndex)
{
	if (PendingRequestCount > 0)
	{
		PendingRequestCount--;
	}
	
	if (!bHasPendingSlotChange)
	{
		return;
	}
	
	UE_LOG(LogYcEquipment, Verbose, TEXT("ReconcilePrediction: Server=%d, Pending=%d, Remaining=%d"), 
		ServerIndex, PendingSlotIndex, PendingRequestCount);
	
	// 快速连续切换：等待最终状态
	if (PendingRequestCount > 0)
	{
		return;
	}
	
	// 检查预测是否成功
	if (ServerIndex == PendingSlotIndex)
	{
		// 预测成功，清除预测状态
		UE_LOG(LogYcEquipment, Verbose, TEXT("ReconcilePrediction: Prediction confirmed"));
		bHasPendingSlotChange = false;
		PendingSlotIndex = -1;
	}
	else
	{
		// 预测失败，回滚
		UE_LOG(LogYcEquipment, Warning, TEXT("ReconcilePrediction: Prediction failed, rolling back"));
		RollbackPrediction(ServerIndex);
	}
}

void UYcQuickBarComponent::RollbackPrediction(int32 ServerIndex)
{
	const UYcEquipmentManagerComponent* EquipmentManager = FindEquipmentManager();
	if (!EquipmentManager)
	{
		return;
	}
	
	// 隐藏预测显示的装备
	if (Slots.IsValidIndex(PendingSlotIndex) && Slots[PendingSlotIndex])
	{
		if (UYcEquipmentInstance* PredictedEquipment = EquipmentManager->FindEquipmentByItem(Slots[PendingSlotIndex]))
		{
			// 设置状态为未装备（会触发 OnRep 和 OnUnequipped）
			PredictedEquipment->SetEquipmentState(EYcEquipmentState::Unequipped);
		}
	}
	
	// 显示服务器确认的装备
	if (Slots.IsValidIndex(ServerIndex) && Slots[ServerIndex])
	{
		if (UYcEquipmentInstance* ServerEquipment = EquipmentManager->FindEquipmentByItem(Slots[ServerIndex]))
		{
			// 设置状态为已装备（会触发 OnRep 和 OnEquipped）
			ServerEquipment->SetEquipmentState(EYcEquipmentState::Equipped);
		}
	}
	
	// 回滚 LastActiveSlotIndex：预测失败时，LastActiveSlotIndex 应该恢复到预测前的状态
	// 由于预测时我们把 LastActiveSlotIndex 设为了预测前的 CurrentIndex，
	// 而服务器的真实 LastActiveSlotIndex 我们无法得知，
	// 最安全的做法是将其设为服务器当前激活索引之前的值（即我们预测前的 ActiveSlotIndex）
	// 但由于回滚场景复杂，这里简单地将其设为 -1 表示未知
	// 实际上，回滚是极端情况，LastActiveSlotIndex 主要用于 UI 显示，影响有限
	LastActiveSlotIndex = -1;
	
	// 清除预测状态
	bHasPendingSlotChange = false;
	PendingSlotIndex = -1;
	PendingRequestCount = 0;
	
	// 广播正确的状态
	FYcQuickBarActiveIndexChangedMessage Message;
	Message.Owner = GetOwner();
	Message.ActiveIndex = ServerIndex;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(TAG_Yc_QuickBar_Message_ActiveIndexChanged, Message);
}

// ============================================================================
// 内部实现
// ============================================================================

void UYcQuickBarComponent::ActivateSlotEquipment(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex) || !Slots[SlotIndex])
	{
		return;
	}
	
	UYcEquipmentManagerComponent* EquipmentManager = FindEquipmentManager();
	if (!EquipmentManager)
	{
		return;
	}
	
	UYcEquipmentInstance* Equipment = EquipmentManager->FindEquipmentByItem(Slots[SlotIndex]);
	if (Equipment)
	{
		EquipmentManager->EquipItem(Equipment);
	}
}

void UYcQuickBarComponent::DeactivateSlotEquipment(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex) || !Slots[SlotIndex])
	{
		return;
	}
	
	UYcEquipmentManagerComponent* EquipmentManager = FindEquipmentManager();
	if (!EquipmentManager)
	{
		return;
	}
	
	UYcEquipmentInstance* Equipment = EquipmentManager->FindEquipmentByItem(Slots[SlotIndex]);
	if (Equipment)
	{
		EquipmentManager->UnequipItem(Equipment);
	}
	
	// 将本次卸下的插槽记录为上一次激活的插槽
	LastActiveSlotIndex = SlotIndex;
}

void UYcQuickBarComponent::CreateSlotEquipment(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex) || !Slots[SlotIndex])
	{
		return;
	}
	
	UYcInventoryItemInstance* Item = Slots[SlotIndex];
	
	// 检查物品是否可装备
	const FInventoryFragment_Equippable* Equippable = Item->GetTypedFragment<FInventoryFragment_Equippable>();
	if (!Equippable)
	{
		return;
	}
	
	UYcEquipmentManagerComponent* EquipmentManager = FindEquipmentManager();
	if (!EquipmentManager)
	{
		return;
	}
	
	// 创建装备实例（Inactive状态，Actors隐藏）
	EquipmentManager->CreateEquipment(Equippable->EquipmentDef, Item);
}

void UYcQuickBarComponent::DestroySlotEquipment(int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex) || !Slots[SlotIndex])
	{
		return;
	}
	
	UYcEquipmentManagerComponent* EquipmentManager = FindEquipmentManager();
	if (!EquipmentManager)
	{
		return;
	}
	
	UYcEquipmentInstance* Equipment = EquipmentManager->FindEquipmentByItem(Slots[SlotIndex]);
	if (Equipment)
	{
		EquipmentManager->DestroyEquipment(Equipment);
	}
}

void UYcQuickBarComponent::SetSlotItem_Internal(const int32 Index, UYcInventoryItemInstance* Item)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || !Slots.IsValidIndex(Index))
	{
		return;
	}
	
	UYcInventoryItemInstance* OldItem = Slots[Index];
	Slots[Index] = Item;

	if (Slots[Index] != OldItem)
	{
		MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuickBarComponent, Slots, this);
		OnRep_Slots();
	}
}

void UYcQuickBarComponent::SetActiveSlotIndex_Internal(int32 NewIndex)
{
	if (NewIndex >= NumSlots)
	{
		return;
	}
	
	const int32 OldIndex = ActiveSlotIndex;
	ActiveSlotIndex = NewIndex;
	
	if (ActiveSlotIndex != OldIndex)
	{
		if (GetOwner() && GetOwner()->HasAuthority())
		{
			MARK_PROPERTY_DIRTY_FROM_NAME(UYcQuickBarComponent, ActiveSlotIndex, this);
		}
		OnRep_ActiveSlotIndex(OldIndex);
	}
}

UYcEquipmentManagerComponent* UYcQuickBarComponent::FindEquipmentManager() const
{
	if (const AController* OwnerController = Cast<AController>(GetOwner()))
	{
		if (const APawn* Pawn = OwnerController->GetPawn())
		{
			return Pawn->FindComponentByClass<UYcEquipmentManagerComponent>();
		}
	}

	if (const AActor* Actor = GetOwner())
	{
		return Actor->FindComponentByClass<UYcEquipmentManagerComponent>();
	}
	
	return nullptr;
}

bool UYcQuickBarComponent::IsLocallyControlled() const
{
	if (const AController* OwnerController = Cast<AController>(GetOwner()))
	{
		return OwnerController->IsLocalController();
	}
	if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		return OwnerPawn->IsLocallyControlled();
	}
	return false;
}

bool UYcQuickBarComponent::CanExecutePrediction(int32 SlotIndex) const
{
	// 检查插槽是否有效且有物品
	if (!Slots.IsValidIndex(SlotIndex) || !Slots[SlotIndex])
	{
		return false;
	}
	
	// 检查装备管理组件是否存在
	const UYcEquipmentManagerComponent* EquipmentManager = FindEquipmentManager();
	if (!EquipmentManager)
	{
		return false;
	}
	
	// 检查装备实例是否已同步到客户端
	// 如果装备实例为空，说明服务器创建的实例还在网络传输中
	UYcEquipmentInstance* Equipment = EquipmentManager->FindEquipmentByItem(Slots[SlotIndex]);
	return Equipment != nullptr;
}

// ============================================================================
// 网络复制回调
// ============================================================================

void UYcQuickBarComponent::OnRep_Slots()
{
	FYcQuickBarSlotsChangedMessage Message;
	Message.Owner = GetOwner();
	Message.Slots = Slots;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(TAG_Yc_QuickBar_Message_SlotsChanged, Message);
}

void UYcQuickBarComponent::OnRep_ActiveSlotIndex(int32 OldActiveSlotIndex)
{
	// 主控客户端：处理预测校正
	if (GetOwner() && GetOwner()->GetLocalRole() == ROLE_AutonomousProxy)
	{
		// 这里只需要处理预测校正
		ReconcilePrediction(ActiveSlotIndex);
	}
	
	// 如果没有预测状态，广播消息
	if (!bHasPendingSlotChange)
	{
		FYcQuickBarActiveIndexChangedMessage Message;
		Message.Owner = GetOwner();
		Message.ActiveIndex = ActiveSlotIndex;

		UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
		MessageSystem.BroadcastMessage(TAG_Yc_QuickBar_Message_ActiveIndexChanged, Message);
	}
}

UYcQuickBarComponent* UYcQuickBarComponent::FindQuickBarComponent(const AActor* Actor)
{
	if (!Actor)
	{
		return nullptr;
	}
	
	// 1. 直接从 Actor 上查找（可能是 Controller 或 PlayerState）
	if (UYcQuickBarComponent* QuickBar = Actor->FindComponentByClass<UYcQuickBarComponent>())
	{
		return QuickBar;
	}
	
	// 2. 如果是 Pawn，尝试从其 Controller 上查找
	if (const APawn* Pawn = Cast<APawn>(Actor))
	{
		if (const AController* Controller = Pawn->GetController())
		{
			if (UYcQuickBarComponent* QuickBar = Controller->FindComponentByClass<UYcQuickBarComponent>())
			{
				return QuickBar;
			}
		}
		
		// 3. 尝试从 Pawn 的 PlayerState 上查找
		if (const APlayerState* PlayerState = Pawn->GetPlayerState())
		{
			if (UYcQuickBarComponent* QuickBar = PlayerState->FindComponentByClass<UYcQuickBarComponent>())
			{
				return QuickBar;
			}
		}
	}
	
	// 4. 如果是 Controller，尝试从其 Pawn 上查找
	if (const AController* Controller = Cast<AController>(Actor))
	{
		if (const APawn* Pawn = Controller->GetPawn())
		{
			if (UYcQuickBarComponent* QuickBar = Pawn->FindComponentByClass<UYcQuickBarComponent>())
			{
				return QuickBar;
			}
		}
		
		// 5. 尝试从 Controller 的 PlayerState 上查找
		if (const APlayerState* PlayerState = Controller->PlayerState)
		{
			if (UYcQuickBarComponent* QuickBar = PlayerState->FindComponentByClass<UYcQuickBarComponent>())
			{
				return QuickBar;
			}
		}
	}
	
	// 6. 如果是 PlayerState，尝试从其 Pawn 上查找
	if (const APlayerState* PlayerState = Cast<APlayerState>(Actor))
	{
		if (const APawn* Pawn = PlayerState->GetPawn())
		{
			if (UYcQuickBarComponent* QuickBar = Pawn->FindComponentByClass<UYcQuickBarComponent>())
			{
				return QuickBar;
			}
		}
		
		// 7. 尝试从 PlayerState 的 Controller 上查找
		if (const AController* Controller = Cast<AController>(PlayerState->GetOwner()))
		{
			if (UYcQuickBarComponent* QuickBar = Controller->FindComponentByClass<UYcQuickBarComponent>())
			{
				return QuickBar;
			}
		}
	}
	
	return nullptr;
}