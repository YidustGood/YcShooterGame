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
 * 为了优化武器切换的响应速度，本组件采用客户端视觉效果预测机制，实现视觉上的无延迟装备和卸下装备，但是有以下两点需要注意
 * 1、首次装备由于需要服务端生成装备实例对象和附加的Actor并同步到客户端，所以无法预测，但是首次装备后会进行对象缓存，后续的切换就可以进行客户端预测了
 * 2、装备所会赋予玩家的GameplayAbility始终是等待服务器处理同步后才生效，所以我们才称为视觉效果预测
 * 视觉效果预测带来的一个典型的表现就是高延迟下我们依靠客户端预测立刻切换装备了一把武器，但是这把武器无法被使用
 * 它只是看起来被装备上了，但还需要服务端把武器装备逻辑处理完成把相关数据同步到客户端才算正在的完全装备上这把武器，然后才可以进行使用
 * 卸下武器时也同理，如果不做额外限制处理在一个RTT时间内虽然这把武器看起来被卸下了但是你依旧可以触发它的GameplayAbility
 * 
 * 【切换流程】：
 *                    ┌─────────────────────────────────────────────────────────┐
 *                    │                    客户端 (AutonomousProxy)              │
 *                    │  ┌─────────────────────────────────────────────────────┐ │
 *                    │  │ 1. 玩家输入切换武器                                   │ │
 *                    │  │ 2. 本地预测：立即执行 SetActiveSlotIndex_Predicted() │ │
 *                    │  │    - 更新 PendingSlotIndex (预测状态)                │ │
 *                    │  │    - 播放切换动画/音效                               │ │
 *                    │  │    - 显示新武器模型                                  │ │
 *                    │  │ 3. 发送 Server RPC                                   │ │
 *                    │  └─────────────────────────────────────────────────────┘ │
 *                    └─────────────────────────────────────────────────────────┘
 *                                              │
 *                                              ▼ Server RPC
 *                    ┌─────────────────────────────────────────────────────────┐
 *                    │                    服务器 (Authority)                    │
 *                    │  ┌─────────────────────────────────────────────────────┐ │
 *                    │  │ 4. 验证请求合法性                                    │ │
 *                    │  │    - 检查插槽是否有效                                │ │
 *                    │  │    - 检查物品是否存在                                │ │
 *                    │  │ 5. 执行实际装备逻辑                                  │ │
 *                    │  │    - UnequipItemInSlot()                            │ │
 *                    │  │    - EquipItemInSlot()                              │ │
 *                    │  │ 6. 更新 ActiveSlotIndex (权威状态)                   │ │
 *                    │  │ 7. 标记属性脏，触发复制                              │ │
 *                    │  └─────────────────────────────────────────────────────┘ │
 *                    └─────────────────────────────────────────────────────────┘
 *                                              │
 *                                              ▼ 属性复制
 *                    ┌─────────────────────────────────────────────────────────┐
 *                    │                    客户端 (AutonomousProxy)              │
 *                    │  ┌─────────────────────────────────────────────────────┐ │
 *                    │  │ 8. OnRep_ActiveSlotIndex() 触发                      │ │
 *                    │  │ 9. 比较服务器状态与预测状态                          │ │
 *                    │  │    - 一致：清除预测状态，确认成功                    │ │
 *                    │  │    - 不一致：回滚到服务器状态                        │ │
 *                    │  └─────────────────────────────────────────────────────┘ │
 *                    └─────────────────────────────────────────────────────────┘
 * 
 * 【关键状态变量】
 * - ActiveSlotIndex: 服务器权威状态，通过 ReplicatedUsing 同步
 * - PendingSlotIndex: 客户端预测状态，本地维护，不参与网络复制
 * - bHasPendingSlotChange: 标记是否有待确认的预测切换
 * 
 * 【边界情况处理】
 * 1. 快速连续切换：每次切换覆盖之前的预测状态, 跳过服务器中间状态验证, 只验证最终状态避免中途连续预测出现预期外的回滚
 * 2. 切换时物品被移除：服务器验证失败，客户端回滚
 * 3. 网络延迟/丢包：通过属性复制最终一致性保证
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
	 * 客户端调用时会立即执行本地预测，同时发送 Server RPC，仅视觉效果和UI的主控预测, 装备所附带的GA还是需要通过服务器处理完成后才能使用
	 * @param NewIndex 要激活的插槽序号
	 */
	UFUNCTION(BlueprintCallable, Category="YcGameCore")
	void SetActiveSlotIndex_WithPrediction(int32 NewIndex);
	
	/**
	 * 取消激活指定序号的插槽物品（带客户端预测）
	 * @param NewIndex 要取消激活的插槽序号
	 */
	UFUNCTION(BlueprintCallable, Category="YcGameCore")
	void DeactivateSlotIndex_WithPrediction(int32 NewIndex);

	// ========================================================================
	// Server RPC - 服务器权威执行
	// ========================================================================
	
	/**
	 * [Server RPC] 激活指定序号的插槽物品
	 * @param NewIndex 要激活的插槽序号
	 */
	UFUNCTION(Server, Reliable, Category="YcGameCore")
	void ServerSetActiveSlotIndex(int32 NewIndex);
	
	/**
	 * [Server RPC] 取消激活指定序号的插槽物品
	 * @param NewIndex 要取消激活的插槽序号
	 */
	UFUNCTION(Server, Reliable, Category="YcGameCore")
	void ServerDeactivateSlotIndex(int32 NewIndex);

	// ========================================================================
	// 兼容接口 - 保留原有 Server RPC（向后兼容）
	// ========================================================================
	
	/**
	 * [已废弃] 请使用 SetActiveSlotIndex_WithPrediction 代替
	 * 激活指定序号的插槽物品（纯服务器执行，有延迟）
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="YcGameCore", meta=(DeprecatedFunction, DeprecationMessage="请使用 SetActiveSlotIndex_WithPrediction 以获得更好的响应体验"))
	void SetActiveSlotIndex(int32 NewIndex);
	
	/**
	 * [已废弃] 请使用 DeactivateSlotIndex_WithPrediction 代替
	 * 取消激活指定序号的插槽物品（纯服务器执行，有延迟）
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="YcGameCore", meta=(DeprecatedFunction, DeprecationMessage="请使用 DeactivateSlotIndex_WithPrediction 以获得更好的响应体验"))
	void DeactivateSlotIndex(int32 NewIndex);
	
	// ========================================================================
	// 循环切换接口
	// ========================================================================
	
	/** 向前循环激活插槽中的装备（带客户端预测） */
	UFUNCTION(BlueprintCallable, Category="YiChenGameCore")
	void CycleActiveSlotForward();

	/** 向后循环激活插槽中的装备（带客户端预测） */
	UFUNCTION(BlueprintCallable, Category="YiChenGameCore")
	void CycleActiveSlotBackward(); 

	// ========================================================================
	// 查询接口
	// ========================================================================
	
	/** 获取当前所有的插槽物品对象 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	TArray<UYcInventoryItemInstance*> GetSlots() const
	{
		return Slots;
	}

	/** 
	 * 获取当前激活的插槽序号
	 * 注意：如果有待确认的预测切换，返回预测的索引以保持 UI 一致性
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	int32 GetActiveSlotIndex() const;
	
	/** 获取服务器确认的激活插槽序号（不考虑预测状态） */
	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	int32 GetConfirmedActiveSlotIndex() const { return ActiveSlotIndex; }

	/** 获取当前激活的插槽中的库存物品 */
	UFUNCTION(BlueprintCallable, BlueprintPure = false)
	UYcInventoryItemInstance* GetActiveSlotItem() const;

	/** 获取下一个空闲的插槽 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	int32 GetNextFreeItemSlot() const;

	/** 获取指定插槽中的物品 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	UYcInventoryItemInstance* GetSlotItem(int32 ItemIndex) const;
	
	// ========================================================================
	// 物品管理接口（仅服务器）
	// ========================================================================
	
	/**
	 * 添加指定物品至指定插槽中(如果目标插槽已存在物品,将会被顶替)
	 * @param SlotIndex 要添加的插槽位置
	 * @param Item 物品实例
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool AddItemToSlot(int32 SlotIndex, UYcInventoryItemInstance* Item);

	/** 移除指定插槽中的物品，返回值是被移除的物品 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UYcInventoryItemInstance* RemoveItemFromSlot(int32 SlotIndex);

private:
	// ========================================================================
	// 客户端预测相关
	// ========================================================================
	
	/**
	 * 执行本地预测切换（仅在主控客户端调用）
	 * @param NewIndex 预测的新插槽索引
	 */
	void ExecuteLocalPrediction(int32 NewIndex);
	
	/**
	 * 执行本地预测取消激活（仅在主控客户端调用）
	 * @param NewIndex 要取消激活的插槽索引
	 */
	void ExecuteLocalPredictionDeactivate(int32 NewIndex);
	
	/**
	 * 处理服务器确认/拒绝预测
	 * 在 OnRep_ActiveSlotIndex 中调用，比较服务器状态与预测状态
	 * @param ServerIndex 服务器确认的索引
	 */
	void ReconcilePrediction(int32 ServerIndex);
	
	/**
	 * 回滚预测状态到服务器状态
	 * 当预测失败时调用
	 * @param ServerIndex 服务器的权威索引
	 */
	void RollbackPrediction(int32 ServerIndex);

	// ========================================================================
	// 内部实现
	// ========================================================================
	
	/** 装备当前激活插槽中的物品 */
	void EquipItemInSlot();

	/** 取消装备当前激活插槽中的物品 */
	void UnequipItemInSlot();
	
	/**
	 * 执行本地装备预览（用于客户端预测）
	 * 只处理视觉效果，不执行实际的技能授予等服务器逻辑
	 * @param SlotIndex 要预览装备的插槽索引
	 */
	void EquipItemInSlot_Predicted(int32 SlotIndex);
	
	/**
	 * 执行本地卸载预览（用于客户端预测）
	 * 只处理视觉效果
	 */
	void UnequipItemInSlot_Predicted();

	/** 根据下标设置Slots的值,并标记为脏,触发网络同步，所有对Slots的更改都应调用这个函数进行 */
	void SetSlotsByIndex_Internal(const int32 Index, UYcInventoryItemInstance* InItem);

	/** 内部设置激活索引（服务器权威） */
	void SetActiveSlotIndex_Internal(const int32 NewActiveIndex);

	/** 从组件Owner->Pawn 查找装备管理组件对象 */
	UYcEquipmentManagerComponent* FindEquipmentManager() const;
	
	/** 检查是否为主控客户端（AutonomousProxy） */
	bool IsLocallyControlled() const;
	
protected:
	/** 插槽数量配置 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "QuickBar", meta=(AllowPrivateAccess = "true"))
	int32 NumSlots = 8;

	UFUNCTION()
	void OnRep_Slots();

	UFUNCTION()
	void OnRep_ActiveSlotIndex(int32 InLastActiveSlotIndex);
	
private:
	// ========================================================================
	// 网络复制属性（服务器权威状态）
	// ========================================================================
	
	/** 插槽物品列表 - 服务器权威，复制到所有客户端 */
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_Slots, Category = "QuickBar")
	TArray<TObjectPtr<UYcInventoryItemInstance>> Slots;
	
	/** 当前激活的插槽索引 - 服务器权威，复制到所有客户端 */
	UPROPERTY(ReplicatedUsing=OnRep_ActiveSlotIndex)
	int32 ActiveSlotIndex = -1;
	
	// ========================================================================
	// 本地状态（不参与网络复制）
	// ========================================================================
	
	/** 上一次激活的插槽索引（用于 UI 动画等） */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category= "QuickBar", meta=(AllowPrivateAccess = "true"))
	int32 LastActiveSlotIndex;
	
	/** 当前装备的实例对象 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "QuickBar", meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UYcEquipmentInstance> EquippedInst;
	
	// ========================================================================
	// 客户端预测状态（仅主控客户端使用，不参与网络复制）
	// ========================================================================
	
	/** 
	 * 待确认的预测插槽索引
	 * 当客户端发起切换请求但尚未收到服务器确认时，存储预测的目标索引
	 */
	UPROPERTY()
	int32 PendingSlotIndex = -1;
	
	/** 是否有待确认的预测切换 */
	UPROPERTY()
	bool bHasPendingSlotChange = false;
	
	/**
	 * 待确认的请求数量
	 * 用于处理快速连续切换的情况：
	 * - 每次发送 Server RPC 时 +1
	 * - 每次收到服务器确认（OnRep_ActiveSlotIndex）时 -1
	 * - 只有当计数为 0 时，才真正清除预测状态
	 */
	UPROPERTY()
	int32 PendingRequestCount = 0;
	
	/** 
	 * 预测装备的实例对象（用于客户端预测显示）
	 * 当预测失败需要回滚时，需要隐藏这个预测显示的装备
	 */
	UPROPERTY()
	TObjectPtr<UYcEquipmentInstance> PredictedEquippedInst;
	
public:
	FORCEINLINE int32 GetLastActiveSlotIndex () const { return LastActiveSlotIndex; };
	FORCEINLINE UYcEquipmentInstance* GetEquippedInstance() const { return EquippedInst; };
	
	/** 是否有待确认的预测切换 */
	FORCEINLINE bool HasPendingSlotChange() const { return bHasPendingSlotChange; }
};

/**
 * 用于GameplayMessageRouter插件中的UGameplayMessageSubsystem子系统 消息广播中的消息内容结构体
 * 这是QuickBar插槽发生变化时通知的消息结构体
 */
USTRUCT(BlueprintType)
struct FYcQuickBarSlotsChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	TObjectPtr<AActor> Owner;

	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TArray<TObjectPtr<UYcInventoryItemInstance>> Slots;
};

/**
 * 这是QuickBar当前激活插槽发生变化时通知的消息结构体
 */
USTRUCT(BlueprintType)
struct FYcQuickBarActiveIndexChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	TObjectPtr<AActor> Owner;

	UPROPERTY(BlueprintReadOnly, Category=Inventory)
	int32 ActiveIndex = 0;
};
