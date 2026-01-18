// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Components/ControllerComponent.h"
#include "YcQuickBarComponent.generated.h"

class UYcInventoryItemInstance;
class UYcEquipmentInstance;
class UYcEquipmentManagerComponent;

/**
 * ============================================================================
 * 快捷物品栏组件 (QuickBar Component)
 * ============================================================================
 * 
 * 功能说明：
 * - 提供插槽用于关联库存物品，实现可以快速装备或使用的互斥物品管理
 * - 例如：库存中的武器、道具等，互斥意味着同一时间只能激活一个插槽
 * 
 * ============================================================================
 * 网络架构：客户端预测 + 服务器权威
 * ============================================================================
 * 
 * 为了优化武器切换的响应速度，本组件采用客户端视觉效果预测机制：
 * 
 * 1. 预创建机制：物品加入QuickBar时就创建装备实例和Actors（隐藏状态）
 *    - 消除首次装备的延迟感
 *    - 客户端通过网络同步获得装备实例，可用于预测显示
 * 
 * 2. 客户端预测：切换武器时客户端立即显示/隐藏装备Actors
 *    - 视觉上无延迟
 *    - GA授予仍需等待服务器确认
 * 
 * 3. 状态校正：服务器通过 EquipmentState 复制确认/拒绝预测
 *    - 预测成功：清除预测状态
 *    - 预测失败：回滚到服务器状态
 * 
 * 【切换流程】：
 *                    ┌─────────────────────────────────────────────────────────┐
 *                    │                    客户端 (AutonomousProxy)              │
 *                    │  ┌─────────────────────────────────────────────────────┐ │
 *                    │  │ 1. 玩家输入切换武器                                   │ │
 *                    │  │ 2. 本地预测：立即执行视觉切换                         │ │
 *                    │  │    - 隐藏当前装备Actors                              │ │
 *                    │  │    - 显示新装备Actors                                │ │
 *                    │  │    - 更新 PendingSlotIndex                           │ │
 *                    │  │ 3. 发送 Server RPC                                   │ │
 *                    │  └─────────────────────────────────────────────────────┘ │
 *                    └─────────────────────────────────────────────────────────┘
 *                                              │
 *                                              ▼ Server RPC
 *                    ┌─────────────────────────────────────────────────────────┐
 *                    │                    服务器 (Authority)                    │
 *                    │  ┌─────────────────────────────────────────────────────┐ │
 *                    │  │ 4. 验证请求合法性                                    │ │
 *                    │  │ 5. 执行状态切换                                      │ │
 *                    │  │    - UnequipItem(旧)                                │ │
 *                    │  │    - EquipItem(新)                                  │ │
 *                    │  │ 6. 更新 ActiveSlotIndex                             │ │
 *                    │  │ 7. EquipmentState 变化触发复制                       │ │
 *                    │  └─────────────────────────────────────────────────────┘ │
 *                    └─────────────────────────────────────────────────────────┘
 *                                              │
 *                                              ▼ 属性复制
 *                    ┌─────────────────────────────────────────────────────────┐
 *                    │                    客户端 (AutonomousProxy)              │
 *                    │  ┌─────────────────────────────────────────────────────┐ │
 *                    │  │ 8. OnRep_ActiveSlotIndex() 触发                      │ │
 *                    │  │ 9. 比较服务器状态与预测状态                          │ │
 *                    │  │    - 一致：清除预测状态                              │ │
 *                    │  │    - 不一致：回滚到服务器状态                        │ │
 *                    │  └─────────────────────────────────────────────────────┘ │
 *                    └─────────────────────────────────────────────────────────┘
 * 
 * ============================================================================
 */
UCLASS(ClassGroup=(YiChenGameCore), Blueprintable, meta=(BlueprintSpawnableComponent))
class YICHENEQUIPMENT_API UYcQuickBarComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	UYcQuickBarComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	// ========================================================================
	// 主要接口 - 客户端预测版本（推荐使用）
	// ========================================================================
	
	/**
	 * 激活指定序号的插槽物品（带客户端预测）
	 * @param NewIndex 要激活的插槽序号
	 */
	UFUNCTION(BlueprintCallable, Category = "QuickBar")
	void SetActiveSlotIndex_WithPrediction(int32 NewIndex);
	
	/**
	 * 取消激活指定序号的插槽物品（带客户端预测）
	 * @param NewIndex 要取消激活的插槽序号
	 */
	UFUNCTION(BlueprintCallable, Category = "QuickBar")
	void DeactivateSlotIndex_WithPrediction(int32 NewIndex);

	// ========================================================================
	// Server RPC
	// ========================================================================
	
	UFUNCTION(Server, Reliable)
	void ServerSetActiveSlotIndex(int32 NewIndex);
	
	UFUNCTION(Server, Reliable)
	void ServerDeactivateSlotIndex(int32 NewIndex);
	
	// ========================================================================
	// 循环切换接口
	// ========================================================================
	
	/** 向前循环激活插槽中的装备（带客户端预测） */
	UFUNCTION(BlueprintCallable, Category = "QuickBar")
	void CycleActiveSlotForward();

	/** 向后循环激活插槽中的装备（带客户端预测） */
	UFUNCTION(BlueprintCallable, Category = "QuickBar")
	void CycleActiveSlotBackward(); 

	// ========================================================================
	// 查询接口
	// ========================================================================
	
	/** 获取当前所有的插槽物品对象 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "QuickBar")
	TArray<UYcInventoryItemInstance*> GetSlots() const { return Slots; }

	/** 
	 * 获取当前激活的插槽序号
	 * 注意：如果有待确认的预测切换，返回预测的索引以保持 UI 一致性
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "QuickBar")
	int32 GetActiveSlotIndex() const;
	
	/** 获取服务器确认的激活插槽序号（不考虑预测状态） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "QuickBar")
	int32 GetConfirmedActiveSlotIndex() const { return ActiveSlotIndex; }

	/** 获取当前激活的插槽中的库存物品 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "QuickBar")
	UYcInventoryItemInstance* GetActiveSlotItem() const;

	/** 获取下一个空闲的插槽 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "QuickBar")
	int32 GetNextFreeItemSlot() const;

	/** 获取指定插槽中的物品 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "QuickBar")
	UYcInventoryItemInstance* GetSlotItem(int32 ItemIndex) const;
	
	/** 获取指定插槽中物品对应的装备实例 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "QuickBar")
	UYcEquipmentInstance* GetSlotEquipmentInstance(int32 ItemIndex) const;
	
	/** 获取当前激活插槽中物品对应的装备实例 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "QuickBar")
	UYcEquipmentInstance* GetActiveEquipmentInstance() const;
	
	/** 获取上一次激活的插槽序号 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "QuickBar")
	int32 GetLastActiveSlotIndex() const { return LastActiveSlotIndex; }
	
	/** 是否有待确认的预测切换 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "QuickBar")
	bool HasPendingSlotChange() const { return bHasPendingSlotChange; }
	
	// ========================================================================
	// 物品管理接口（仅服务器）
	// ========================================================================
	
	/**
	 * 添加指定物品至指定插槽中
	 * 会自动创建装备实例（如果物品可装备）
	 * 
	 * @param SlotIndex 要添加的插槽位置
	 * @param Item 物品实例
	 * @return 是否添加成功
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "QuickBar")
	bool AddItemToSlot(int32 SlotIndex, UYcInventoryItemInstance* Item);

	/** 
	 * 移除指定插槽中的物品
	 * 会自动销毁对应的装备实例
	 * 
	 * @param SlotIndex 要移除的插槽位置
	 * @return 被移除的物品
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "QuickBar")
	UYcInventoryItemInstance* RemoveItemFromSlot(int32 SlotIndex);

protected:
	/** 插槽数量配置 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "QuickBar")
	int32 NumSlots = 8;

	UFUNCTION()
	void OnRep_Slots();

	UFUNCTION()
	void OnRep_ActiveSlotIndex(int32 OldActiveSlotIndex);
	
private:
	// ========================================================================
	// 客户端预测
	// ========================================================================
	
	/** 执行本地预测切换 */
	void ExecuteLocalPrediction(int32 NewIndex);
	
	/** 执行本地预测取消激活 */
	void ExecuteLocalPredictionDeactivate(int32 NewIndex);
	
	/** 处理服务器确认/拒绝预测 */
	void ReconcilePrediction(int32 ServerIndex);
	
	/** 回滚预测状态到服务器状态 */
	void RollbackPrediction(int32 ServerIndex);

	// ========================================================================
	// 内部实现
	// ========================================================================
	
	/** 激活指定插槽的装备（服务器） */
	void ActivateSlotEquipment(int32 SlotIndex);
	
	/** 停用指定插槽的装备（服务器） */
	void DeactivateSlotEquipment(int32 SlotIndex);
	
	/** 创建指定插槽物品的装备实例（服务器） */
	void CreateSlotEquipment(int32 SlotIndex);
	
	/** 销毁指定插槽物品的装备实例（服务器） */
	void DestroySlotEquipment(int32 SlotIndex);

	/** 设置Slots数组指定索引的值 */
	void SetSlotItem_Internal(int32 Index, UYcInventoryItemInstance* Item);

	/** 设置激活索引 */
	void SetActiveSlotIndex_Internal(int32 NewIndex);

	/** 查找装备管理组件 */
	UYcEquipmentManagerComponent* FindEquipmentManager() const;
	
	/** 检查是否为本地控制 */
	bool IsLocallyControlled() const;
	
	/**
	 * 检查是否可以执行客户端预测
	 * 
	 * 边界情况处理：当玩家拾取物品后立即按键装备，但由于网络延迟，
	 * EquipmentInstance 可能还在传输中尚未到达客户端。
	 * 
	 * @param SlotIndex 要检查的插槽索引
	 * @return true 如果装备实例已同步到客户端，可以执行预测
	 */
	bool CanExecutePrediction(int32 SlotIndex) const;
	
	// ========================================================================
	// 网络复制属性
	// ========================================================================
	
	/** 插槽物品列表 */
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_Slots, Category = "QuickBar")
	TArray<TObjectPtr<UYcInventoryItemInstance>> Slots;
	
	/** 当前激活的插槽索引 */
	UPROPERTY(ReplicatedUsing = OnRep_ActiveSlotIndex)
	int32 ActiveSlotIndex = -1;
	
	// ========================================================================
	// 本地状态
	// ========================================================================
	
	/** 上一次激活的插槽索引, 仅服务端和主控客户端有效 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "QuickBar", meta = (AllowPrivateAccess = "true"))
	int32 LastActiveSlotIndex = -1;
	
	// ========================================================================
	// 客户端预测状态
	// ========================================================================
	
	/** 待确认的预测插槽索引 */
	int32 PendingSlotIndex = -1;
	
	/** 是否有待确认的预测切换 */
	bool bHasPendingSlotChange = false;
	
	/** 待确认的请求数量（处理快速连续切换） */
	int32 PendingRequestCount = 0;
	
	
public:
	/**
	 * 从 Actor 上查找 QuickBarComponent
	 * 支持从 Pawn、Controller、PlayerState 三种对象上查找
	 * 
	 * @param Actor 要查找的 Actor（可以是 Pawn、Controller、PlayerState 或其他）
	 * @return 找到的 QuickBarComponent，未找到返回 nullptr
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "QuickBar", meta = (DefaultToSelf = "Actor"))
	static UYcQuickBarComponent* FindQuickBarComponent(const AActor* Actor);
};

// ============================================================================
// 消息结构体（用于 GameplayMessageSubsystem）
// ============================================================================

/** QuickBar插槽发生变化时的消息 */
USTRUCT(BlueprintType)
struct FYcQuickBarSlotsChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = QuickBar)
	TObjectPtr<AActor> Owner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = QuickBar)
	TArray<TObjectPtr<UYcInventoryItemInstance>> Slots;
};

/** QuickBar当前激活插槽发生变化时的消息 */
USTRUCT(BlueprintType)
struct FYcQuickBarActiveIndexChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = QuickBar)
	TObjectPtr<AActor> Owner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = QuickBar)
	int32 ActiveIndex = -1;
};
