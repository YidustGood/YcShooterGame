// Copyright (c) 2025 YiChen. All Rights Reserved.

/**
 * ============================================================================
 * UYcQuickBarComponent 实现
 * ============================================================================
 * 
 * 本文件实现了带客户端预测的快捷物品栏切换机制。
 * 
 * 【核心设计理念】
 * 1. 服务器权威：所有实际的装备/卸载逻辑由服务器执行
 * 2. 客户端预测：主控客户端立即显示切换效果，无需等待服务器响应
 * 3. 状态校正：服务器响应后，客户端校正预测状态（如有差异则回滚）
 * 
 * 【预测机制详解】
 * 
 * 场景1：正常切换（预测成功）
 * ─────────────────────────────────────────────────────────────────────────
 * 时间线:  T0          T1              T2              T3
 *          │           │               │               │
 * 客户端:  输入切换 → 本地预测执行 → (等待中...) → 收到确认，清除预测状态
 *          │           │               │               │
 * 服务器:  (等待中) → 收到RPC → 验证&执行 → 复制属性
 * 
 * 场景2：预测失败（物品被移除）
 * ─────────────────────────────────────────────────────────────────────────
 * 时间线:  T0          T1              T2              T3
 *          │           │               │               │
 * 客户端:  输入切换 → 本地预测执行 → (等待中...) → 收到拒绝，回滚状态
 *          │           │               │               │
 * 服务器:  (等待中) → 收到RPC → 验证失败 → 复制原状态
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
#include "Net/UnrealNetwork.h"
#include "Net/Core/PushModel/PushModel.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcQuickBarComponent)

// ============================================================================
// Gameplay Tags 定义
// ============================================================================
// 用于 UGameplayMessageSubsystem 进行消息广播
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

	// 使用 PushModel 方式进行网络复制，只在属性变化时才发送
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
// 客户端预测接口实现
// ============================================================================

void UYcQuickBarComponent::SetActiveSlotIndex_WithPrediction(const int32 NewIndex)
{
	// 验证索引有效性（使用当前有效索引，考虑预测状态）
	const int32 CurrentEffectiveIndex = GetActiveSlotIndex();
	if (!Slots.IsValidIndex(NewIndex) || CurrentEffectiveIndex == NewIndex)
	{
		return;
	}
	
	// 主控客户端：执行本地预测
	if (IsLocallyControlled() && !GetOwner()->HasAuthority())
	{
		ExecuteLocalPrediction(NewIndex);
		// 增加待确认请求计数
		PendingRequestCount++;
	}
	
	// 发送 Server RPC（所有端都需要发送，包括 Listen Server 的主机玩家）
	ServerSetActiveSlotIndex(NewIndex);
}

void UYcQuickBarComponent::DeactivateSlotIndex_WithPrediction(const int32 NewIndex)
{
	// 验证：只能取消当前激活的插槽（使用当前有效索引）
	const int32 CurrentEffectiveIndex = GetActiveSlotIndex();
	if (!Slots.IsValidIndex(NewIndex) || CurrentEffectiveIndex != NewIndex)
	{
		return;
	}
	
	// 主控客户端：执行本地预测
	if (IsLocallyControlled() && !GetOwner()->HasAuthority())
	{
		ExecuteLocalPredictionDeactivate(NewIndex);
		// 增加待确认请求计数
		PendingRequestCount++;
	}
	
	// 发送 Server RPC
	ServerDeactivateSlotIndex(NewIndex);
}

// ============================================================================
// Server RPC 实现
// ============================================================================

void UYcQuickBarComponent::ServerSetActiveSlotIndex_Implementation(int32 NewIndex)
{
	// 服务器验证和执行
	if (Slots.IsValidIndex(NewIndex) && (ActiveSlotIndex != NewIndex))
	{
		// 取消装备当前插槽的物品
		UnequipItemInSlot();

		// 修改当前激活的 SlotIndex
		SetActiveSlotIndex_Internal(NewIndex);

		// 装备新激活的插槽
		EquipItemInSlot();
	}
	// @TODO ActiveSlotIndex在预测激活装备流程中似乎没有修改也不会触发OnRep, 导致主控端无法收到验证失败回滚的消息吧
	// 如果验证失败，不做任何操作
	// 客户端会在收到复制的 ActiveSlotIndex 后自动回滚
}

void UYcQuickBarComponent::ServerDeactivateSlotIndex_Implementation(int32 NewIndex)
{
	if (Slots.IsValidIndex(NewIndex) && (ActiveSlotIndex == NewIndex))
	{
		// 取消装备当前插槽的物品
		UnequipItemInSlot();

		// 修改当前激活的 SlotIndex 为 -1
		SetActiveSlotIndex_Internal(-1);
	}
}

// ============================================================================
// 兼容接口实现（保留原有行为）
// ============================================================================

void UYcQuickBarComponent::SetActiveSlotIndex_Implementation(const int32 NewIndex)
{
	if (Slots.IsValidIndex(NewIndex) && (ActiveSlotIndex != NewIndex))
	{
		UnequipItemInSlot();
		SetActiveSlotIndex_Internal(NewIndex);
		EquipItemInSlot();
	}
}

void UYcQuickBarComponent::DeactivateSlotIndex_Implementation(const int32 NewIndex)
{
	if (Slots.IsValidIndex(NewIndex) && (ActiveSlotIndex == NewIndex))
	{
		UnequipItemInSlot();
		SetActiveSlotIndex_Internal(-1);
	}
}

// ============================================================================
// 循环切换实现
// ============================================================================

void UYcQuickBarComponent::CycleActiveSlotForward()
{
	if (Slots.Num() < 2) return;
	
	// 使用当前有效的索引（考虑预测状态）
	const int32 CurrentIndex = GetActiveSlotIndex();
	const int32 OldIndex = (CurrentIndex < 0 ? Slots.Num() - 1 : CurrentIndex);
	int32 NewIndex = CurrentIndex;
	
	do
	{
		NewIndex = (NewIndex + 1) % Slots.Num();
		if (Slots[NewIndex] != nullptr)
		{
			// 使用带预测的版本
			SetActiveSlotIndex_WithPrediction(NewIndex);
			return;
		}
	}
	while (NewIndex != OldIndex);
}

void UYcQuickBarComponent::CycleActiveSlotBackward()
{
	if (Slots.Num() < 2) return;

	// 使用当前有效的索引（考虑预测状态）
	const int32 CurrentIndex = GetActiveSlotIndex();
	const int32 OldIndex = (CurrentIndex < 0 ? Slots.Num() - 1 : CurrentIndex);
	int32 NewIndex = CurrentIndex;
	
	do
	{
		NewIndex = (NewIndex - 1 + Slots.Num()) % Slots.Num();
		if (Slots[NewIndex] != nullptr)
		{
			// 使用带预测的版本
			SetActiveSlotIndex_WithPrediction(NewIndex);
			return;
		}
	}
	while (NewIndex != OldIndex);
}

// ============================================================================
// 查询接口实现
// ============================================================================

int32 UYcQuickBarComponent::GetActiveSlotIndex() const
{
	// 如果有待确认的预测切换，返回预测的索引以保持 UI 一致性
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

	// 如果要移除的是目前激活的插槽，先将该插槽的物品取消装备，并重置 ActiveSlotIndex
	if (ActiveSlotIndex == SlotIndex)
	{
		UnequipItemInSlot();
		SetActiveSlotIndex_Internal(-1);
	}
	
	// 如果客户端正在预测切换到这个插槽，清除预测状态
	if (bHasPendingSlotChange && PendingSlotIndex == SlotIndex)
	{
		bHasPendingSlotChange = false;
		PendingSlotIndex = -1;
		// 如果有预测显示的装备，隐藏它
		if (PredictedEquippedInst)
		{
			PredictedEquippedInst->HideEquipmentActors();
			PredictedEquippedInst->HideLocalEquipmentActors();
			PredictedEquippedInst = nullptr;
		}
	}

	if (Slots.IsValidIndex(SlotIndex))
	{
		Result = Slots[SlotIndex];

		if (Result != nullptr)
		{
			// 清理该物品的缓存装备实例（真正销毁 Actors）
			if (UYcEquipmentManagerComponent* EquipmentManager = FindEquipmentManager())
			{
				EquipmentManager->ClearCachedEquipmentForItem(Result);
			}
			
			SetSlotsByIndex_Internal(SlotIndex, nullptr);
		}
	}

	return Result;
}

// ============================================================================
// 客户端预测核心实现
// ============================================================================

void UYcQuickBarComponent::ExecuteLocalPrediction(const int32 NewIndex)
{
	UE_LOG(LogYcEquipment, Log, TEXT("ExecuteLocalPrediction: 预测切换到插槽 %d (当前预测: %d, 服务器确认: %d)"), 
		NewIndex, PendingSlotIndex, ActiveSlotIndex);
	
	// ========================================================================
	// 关键：先隐藏当前显示的装备
	// 需要隐藏的是"当前实际显示的"装备，可能是：
	// 1. 上一次预测显示的装备 (PredictedEquippedInst)
	// 2. 服务器确认的装备 (EquippedInst)
	// ========================================================================
	
	// 隐藏预测显示的装备
	if (PredictedEquippedInst)
	{
		PredictedEquippedInst->HideEquipmentActors();
		PredictedEquippedInst->HideLocalEquipmentActors();
		PredictedEquippedInst = nullptr;
	}
	
	// 隐藏服务器确认的装备
	if (EquippedInst)
	{
		EquippedInst->HideEquipmentActors();
		EquippedInst->HideLocalEquipmentActors();
	}
	
	// 设置新的预测状态
	bHasPendingSlotChange = true;
	PendingSlotIndex = NewIndex;
	
	// 执行本地预测装备（只处理视觉效果）
	EquipItemInSlot_Predicted(NewIndex);
	
	// 广播消息，让 UI 立即响应
	FYcQuickBarActiveIndexChangedMessage Message;
	Message.Owner = GetOwner();
	Message.ActiveIndex = NewIndex;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(TAG_Yc_QuickBar_Message_ActiveIndexChanged, Message);
}

void UYcQuickBarComponent::ExecuteLocalPredictionDeactivate(int32 NewIndex)
{
	UE_LOG(LogYcEquipment, Log, TEXT("ExecuteLocalPredictionDeactivate: 预测取消激活插槽 %d"), NewIndex);
	
	// 设置预测状态
	bHasPendingSlotChange = true;
	PendingSlotIndex = -1;
	
	// 执行本地预测卸载
	UnequipItemInSlot_Predicted();
	
	// 广播消息
	FYcQuickBarActiveIndexChangedMessage Message;
	Message.Owner = GetOwner();
	Message.ActiveIndex = -1;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(TAG_Yc_QuickBar_Message_ActiveIndexChanged, Message);
}

void UYcQuickBarComponent::ReconcilePrediction(int32 ServerIndex)
{
	// 减少待确认请求计数
	if (PendingRequestCount > 0)
	{
		PendingRequestCount--;
	}
	
	if (!bHasPendingSlotChange)
	{
		// 没有待确认的预测，直接返回
		return;
	}
	
	UE_LOG(LogYcEquipment, Log, TEXT("ReconcilePrediction: 服务器索引=%d, 预测索引=%d, 剩余请求=%d"), 
		ServerIndex, PendingSlotIndex, PendingRequestCount);
	
	// ========================================================================
	// 快速连续切换装备处理：
	// 如果还有待确认的请求，说明服务器返回的是中间状态，不是最终状态
	// 此时不应该回滚，继续等待后续确认
	// ========================================================================
	if (PendingRequestCount > 0)
	{
		// 还有请求在路上，忽略这次中间状态的确认
		UE_LOG(LogYcEquipment, Log, TEXT("ReconcilePrediction: 还有 %d 个请求待确认，忽略中间状态"), PendingRequestCount);
		return;
	}
	
	// 所有请求都已确认，检查最终状态是否与预测一致
	if (ServerIndex == PendingSlotIndex)
	{
		// 预测成功！服务器确认了我们的预测
		UE_LOG(LogYcEquipment, Log, TEXT("ReconcilePrediction: 预测成功，清除预测状态"));
		
		// 清除预测状态
		bHasPendingSlotChange = false;
		PendingSlotIndex = -1;
		PredictedEquippedInst = nullptr;
	}
	else
	{
		// 预测失败！需要回滚
		UE_LOG(LogYcEquipment, Warning, TEXT("ReconcilePrediction: 预测失败，回滚到服务器状态 %d"), ServerIndex);
		RollbackPrediction(ServerIndex);
	}
}

void UYcQuickBarComponent::RollbackPrediction(int32 ServerIndex)
{
	// 隐藏预测显示的装备
	if (PredictedEquippedInst)
	{
		PredictedEquippedInst->HideEquipmentActors();
		PredictedEquippedInst->HideLocalEquipmentActors();
		PredictedEquippedInst = nullptr;
	}
	
	// 清理预测时设置的 EquippedInst
	// 服务器会通过 EquipmentList 复制来设置正确的装备状态
	EquippedInst = nullptr;
	
	// 清除预测状态
	bHasPendingSlotChange = false;
	PendingSlotIndex = -1;
	PendingRequestCount = 0;  // 重置请求计数
	
	// 广播正确的状态，让 UI 回滚
	FYcQuickBarActiveIndexChangedMessage Message;
	Message.Owner = GetOwner();
	Message.ActiveIndex = ServerIndex;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(TAG_Yc_QuickBar_Message_ActiveIndexChanged, Message);
}

void UYcQuickBarComponent::EquipItemInSlot_Predicted(const int32 SlotIndex)
{
	if (!Slots.IsValidIndex(SlotIndex))
	{
		return;
	}
	
	UYcInventoryItemInstance* SlotItem = Slots[SlotIndex];
	if (!IsValid(SlotItem))
	{
		return;
	}

	// 获得装备管理组件
	const UYcEquipmentManagerComponent* EquipmentManager = FindEquipmentManager();
	if (!EquipmentManager)
	{
		return;
	}
	
	// ========================================================================
	// 客户端预测策略：
	// 1. 尝试从客户端缓存中获取之前卸载的装备实例
	// 2. 如果找到，显示它并设置 EquippedInst（用于后续卸载预测）
	// 3. 如果没找到（第一次装备），只更新 UI，等待服务器创建装备实例对象同步到客户端
	// ========================================================================
	
	UYcEquipmentInstance* CachedInstance = EquipmentManager->FindCachedEquipmentForItem(SlotItem);
	if (CachedInstance)
	{
		// 找到缓存的装备实例，显示它
		CachedInstance->ShowEquipmentActors();
		CachedInstance->ShowLocalEquipmentActors();
		PredictedEquippedInst = CachedInstance;
		
		// 同时设置 EquippedInst，这样卸载预测时可以直接使用
		// 预测失败时会在 RollbackPrediction 中清理
		EquippedInst = CachedInstance;
		
		UE_LOG(LogYcEquipment, Log, TEXT("EquipItemInSlot_Predicted: 从缓存中找到装备实例，显示它"));
	}
	else
	{
		// 没有缓存（第一次装备该物品）
		// 客户端无法预测显示武器模型，但 UI 状态已经更新
		// 新武器的显示由服务器通过 EquipmentList 复制来完成
		UE_LOG(LogYcEquipment, Log, TEXT("EquipItemInSlot_Predicted: 未找到缓存，等待服务器创建装备实例"));
	}
}

void UYcQuickBarComponent::UnequipItemInSlot_Predicted()
{
	// 隐藏预测显示的装备
	if (PredictedEquippedInst)
	{
		PredictedEquippedInst->HideEquipmentActors();
		PredictedEquippedInst->HideLocalEquipmentActors();
		PredictedEquippedInst = nullptr;
	}
	
	// 隐藏当前装备的实例（可能是服务器复制过来的）
	if (EquippedInst)
	{
		EquippedInst->HideEquipmentActors();
		EquippedInst->HideLocalEquipmentActors();
		// 不清空 EquippedInst，因为它可能还需要用于后续操作
		return;
	}
	
	// ========================================================================
	// 如果 EquippedInst 为 null，尝试从 EquipmentManager 的装备列表中查找
	// 这种情况发生在：服务器复制了装备，但客户端的 EquippedInst 还没设置
	// ========================================================================
	
	// 获取当前应该隐藏的物品（使用服务器确认的索引，而不是预测索引）
	UYcInventoryItemInstance* CurrentSlotItem = Slots.IsValidIndex(ActiveSlotIndex) ? Slots[ActiveSlotIndex] : nullptr;
	if (!CurrentSlotItem)
	{
		return;
	}
	
	UYcEquipmentManagerComponent* EquipmentManager = FindEquipmentManager();
	if (!EquipmentManager)
	{
		return;
	}
	
	// 遍历装备列表查找
	TArray<UYcEquipmentInstance*> AllEquipments = EquipmentManager->GetEquipmentInstancesOfType(UYcEquipmentInstance::StaticClass());
	for (UYcEquipmentInstance* ExistingInst : AllEquipments)
	{
		if (ExistingInst && ExistingInst->GetAssociatedItem() == CurrentSlotItem)
		{
			ExistingInst->HideEquipmentActors();
			// 设置 EquippedInst，下次就不用再查找了
			EquippedInst = ExistingInst;
			UE_LOG(LogYcEquipment, Log, TEXT("UnequipItemInSlot_Predicted: 从装备列表中找到并隐藏装备实例"));
			return;
		}
	}
}

// ============================================================================
// 内部实现
// ============================================================================

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

bool UYcQuickBarComponent::IsLocallyControlled() const
{
	// 支持 Controller 和 Pawn 两种挂载方式
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

// ============================================================================
// 网络复制回调
// ============================================================================

void UYcQuickBarComponent::OnRep_Slots()
{
	// 使用 GameplayMessageRouter 插件中的 UGameplayMessageSubsystem 子系统进行消息广播
	FYcQuickBarSlotsChangedMessage Message;
	Message.Owner = GetOwner();
	Message.Slots = Slots;

	UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
	MessageSystem.BroadcastMessage(TAG_Yc_QuickBar_Message_SlotsChanged, Message);
}

void UYcQuickBarComponent::OnRep_ActiveSlotIndex(int32 InLastActiveSlotIndex)
{
	LastActiveSlotIndex = InLastActiveSlotIndex;
	
	// 处理客户端预测校正
	// 只在主控客户端（AutonomousProxy）上执行预测校正
	if (IsLocallyControlled() && !GetOwner()->HasAuthority())
	{
		ReconcilePrediction(ActiveSlotIndex);
	}
	
	// 如果没有待确认的预测，或者预测已经被校正，广播消息
	// 注意：如果有预测状态，消息已经在 ExecuteLocalPrediction 中广播过了
	if (!bHasPendingSlotChange)
	{
		FYcQuickBarActiveIndexChangedMessage Message;
		Message.Owner = GetOwner();
		Message.ActiveIndex = ActiveSlotIndex;

		UGameplayMessageSubsystem& MessageSystem = UGameplayMessageSubsystem::Get(this);
		MessageSystem.BroadcastMessage(TAG_Yc_QuickBar_Message_ActiveIndexChanged, Message);
	}
}