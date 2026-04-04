// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Components/PawnComponent.h"
#include "YcEquipmentSlotComponent.generated.h"

class UYcInventoryItemInstance;
class UYcInventoryManagerComponent;
class UYcEquipmentManagerComponent;

/**
 * ============================================================================
 * 装备栏槽位结构体
 * ============================================================================
 */
USTRUCT(BlueprintType)
struct FYcEquipmentSlot
{
	GENERATED_BODY()

	/** 槽位标签（如 EquipmentSlot.Armor, EquipmentSlot.Helmet） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FGameplayTag SlotTag;
	
	/** 当前槽位中的物品实例 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UYcInventoryItemInstance> ItemInstance = nullptr;
};

/**
 * ============================================================================
 * 装备栏槽位组件 (Equipment Slot Component)
 * ============================================================================
 * 
 * 功能说明：
 * - 管理角色的装备栏槽位（护甲、头盔、武器等）
 * - 装备时物品从 Inventory 移出，卸下时回归 Inventory
 * - 槽位互斥：同一槽位只能装备一件装备
 * - 与 EquipmentManager 协作：本组件管物品存储，EquipmentManager 管装备状态
 * 
 * ============================================================================
 * 与 QuickBarComponent 的区别
 * ============================================================================
 * 
 * QuickBarComponent:
 * - 互斥激活（同一时间只能激活一个武器）
 * - 物品保留在 Inventory 中
 * - 用于快速切换的武器/道具栏
 * 
 * EquipmentSlotComponent:
 * - 独立槽位（护甲、头盔可同时装备）
 * - 物品从 Inventory 移出/回归
 * - 用于永久装备栏
 * 
 * ============================================================================
 */
UCLASS(ClassGroup=(YiChenGameCore), Blueprintable, meta=(BlueprintSpawnableComponent))
class YICHENEQUIPMENT_API UYcEquipmentSlotComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UYcEquipmentSlotComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;

	// ========================================================================
	// 装备接口（仅服务器）
	// ========================================================================

	/**
	 * 将物品装备到对应槽位
	 * - 从 Inventory 移出物品
	 * - 通知 EquipmentManager 创建装备实例并装备
	 * 
	 * @param ItemInstance 要装备的物品实例
	 * @return 是否成功装备
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	bool EquipItem(UYcInventoryItemInstance* ItemInstance);

	/**
	 * 卸下指定槽位的装备
	 * - 通知 EquipmentManager 卸下装备
	 * - 将物品回归 Inventory
	 * 
	 * @param SlotTag 要卸下的槽位标签
	 * @return 被卸下的物品实例，失败返回 nullptr
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	UYcInventoryItemInstance* UnequipSlot(FGameplayTag SlotTag);

	/** 客户端请求装备物品到对应装备槽 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Equipment")
	void ServerEquipItem(UYcInventoryItemInstance* ItemInstance);

	/** 客户端请求卸下指定装备槽 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Equipment")
	void ServerUnequipSlot(FGameplayTag SlotTag);

	/**
	 * 卸下所有装备
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	void UnequipAll();

	// ========================================================================
	// 查询接口
	// ========================================================================

	/** 检查指定槽位是否已被占用 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	bool IsSlotOccupied(FGameplayTag SlotTag) const;

	/** 获取指定槽位中的物品 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	UYcInventoryItemInstance* GetItemInSlot(FGameplayTag SlotTag) const;

	/** 获取所有已占用的槽位标签 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	TArray<FGameplayTag> GetOccupiedSlots() const;

	/** 获取所有槽位配置 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	const TArray<FYcEquipmentSlot>& GetSlots() const { return Slots; }

	/** 获取槽位配置 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	bool GetSlotConfig(FGameplayTag SlotTag, FYcEquipmentSlot& OutSlotConfig) const;

	// ========================================================================
	// 静态查找函数
	// ========================================================================

	/**
	 * 从 Actor 上查找 EquipmentSlotComponent
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment", meta = (DefaultToSelf = "Actor"))
	static UYcEquipmentSlotComponent* FindEquipmentSlotComponent(const AActor* Actor);

protected:
	/** 槽位配置列表 */
	UPROPERTY(EditDefaultsOnly, ReplicatedUsing = OnRep_Slots, Category = "QuickBar")
	TArray<FYcEquipmentSlot> Slots;
	
	UFUNCTION()
	void OnRep_Slots();

private:
	// ========================================================================
	// 内部实现
	// ========================================================================

	/** 查找槽位索引 */
	int32 FindSlotIndex(FGameplayTag SlotTag) const;

	/** 设置槽位物品 */
	void SetSlotItem_Internal(int32 SlotIndex, UYcInventoryItemInstance* Item);

	/** 获取关联的 InventoryManager */
	UYcInventoryManagerComponent* GetInventoryManager() const;

	/** 获取关联的 EquipmentManager */
	UYcEquipmentManagerComponent* GetEquipmentManager() const;

	/** 获取物品应装备到的槽位标签 */
	FGameplayTag GetEquipmentSlotTag(UYcInventoryItemInstance* ItemInstance) const;
};

// ============================================================================
// 消息结构体（用于 GameplayMessageSubsystem）
// ============================================================================

/** 装备栏槽位发生变化时的消息 */
USTRUCT(BlueprintType)
struct FYcEquipmentSlotChangedMessage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<AActor> Owner = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	FGameplayTag SlotTag;

	UPROPERTY(BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UYcInventoryItemInstance> ItemInstance = nullptr;
};
