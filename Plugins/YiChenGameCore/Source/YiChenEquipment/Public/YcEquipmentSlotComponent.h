// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Components/PawnComponent.h"
#include "YcEquipmentSlotComponent.generated.h"

class UYcInventoryItemInstance;
class UYcInventoryManagerComponent;
class UYcEquipmentManagerComponent;
struct FYcInventoryOperation;
struct FYcInventoryOperationDelta;
struct FYcInventoryProjectedState;

/**
 * 装备栏槽位数据。
 */
USTRUCT(BlueprintType)
struct FYcEquipmentSlot
{
	GENERATED_BODY()

	/** 槽位标签，例如 `EquipmentSlot.Armor`。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FGameplayTag SlotTag;

	/** 当前槽位中持有的物品实例。 */
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Equipment")
	TObjectPtr<UYcInventoryItemInstance> ItemInstance = nullptr;
};

/**
 * 装备栏组件：负责槽位占用与装备/卸下流程。
 */
UCLASS(ClassGroup=(YiChenGameCore), Blueprintable, meta=(BlueprintSpawnableComponent))
class YICHENEQUIPMENT_API UYcEquipmentSlotComponent : public UPawnComponent
{
	GENERATED_BODY()

public:
	UYcEquipmentSlotComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 装备接口（仅服务器）

	/**
	 * 将物品装备到对应槽位。
	 * @param ItemInstance 要装备的物品实例。
	 * @return 是否装备成功。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	bool EquipItem(UYcInventoryItemInstance* ItemInstance);

	/**
	 * 卸下指定槽位的装备。
	 * @param SlotTag 槽位标签。
	 * @return 被卸下的物品，失败返回 `nullptr`。
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	UYcInventoryItemInstance* UnequipSlot(FGameplayTag SlotTag);

	/** 客户端请求装备物品到对应槽位。 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Equipment")
	void ServerEquipItem(UYcInventoryItemInstance* ItemInstance);

	/** 客户端请求卸下指定槽位。 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Equipment")
	void ServerUnequipSlot(FGameplayTag SlotTag);

	/** 卸下所有装备。 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Equipment")
	void UnequipAll();

	// 查询接口

	/** 检查指定槽位是否已被占用。 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	bool IsSlotOccupied(FGameplayTag SlotTag) const;

	/** 获取指定槽位中的物品。 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	UYcInventoryItemInstance* GetItemInSlot(FGameplayTag SlotTag) const;

	/** 获取当前所有已占用槽位。 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	TArray<FGameplayTag> GetOccupiedSlots() const;

	/** 获取槽位数组。 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	const TArray<FYcEquipmentSlot>& GetSlots() const { return Slots; }

	/** 获取指定槽位配置。 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment")
	bool GetSlotConfig(FGameplayTag SlotTag, FYcEquipmentSlot& OutSlotConfig) const;

	// 静态查找

	/** 从 Actor 上查找 `UYcEquipmentSlotComponent`。 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment", meta = (DefaultToSelf = "Actor"))
	static UYcEquipmentSlotComponent* FindEquipmentSlotComponent(const AActor* Actor);

protected:
	/** 槽位配置列表。 */
	UPROPERTY(EditDefaultsOnly, ReplicatedUsing = OnRep_Slots, Category = "QuickBar")
	TArray<FYcEquipmentSlot> Slots;

	UFUNCTION()
	void OnRep_Slots();

private:
	/** 根据槽位标签查找数组索引。 */
	int32 FindSlotIndex(FGameplayTag SlotTag) const;

	/** 设置指定索引槽位中的物品。 */
	void SetSlotItem_Internal(int32 SlotIndex, UYcInventoryItemInstance* Item);

	/** 获取同一拥有者上的库存组件。 */
	UYcInventoryManagerComponent* GetInventoryManager() const;

	/** 获取同一拥有者上的装备管理组件。 */
	UYcEquipmentManagerComponent* GetEquipmentManager() const;

	/** 获取物品应装备到的槽位标签。 */
	FGameplayTag GetEquipmentSlotTag(UYcInventoryItemInstance* ItemInstance) const;

	/** 向 Router 注册装备操作处理器。 */
	void RegisterInventoryOperationHandlers();

	/** 从 Router 反注册装备操作处理器。 */
	void UnregisterInventoryOperationHandlers();

	/** `Equipment.Equip` 的服务端校验逻辑。 */
	bool ValidateEquipOperation(const FYcInventoryOperation& Operation, FString& OutReason) const;

	/** `Equipment.Equip` 的服务端执行逻辑。 */
	bool ExecuteEquipOperation(const FYcInventoryOperation& Operation, FString& OutReason);

	/** `Equipment.Equip` 的增量结果构建逻辑。 */
	void BuildEquipOperationDelta(const FYcInventoryOperation& Operation, bool bSuccess, FYcInventoryOperationDelta& OutDelta) const;
	/** `Equipment.Equip` 的预测态投影逻辑。 */
	void ProjectEquipOperationState(const FYcInventoryOperation& Operation, FYcInventoryProjectedState& InOutProjectedState) const;

	/** `Equipment.Unequip` 的服务端校验逻辑。 */
	bool ValidateUnequipOperation(const FYcInventoryOperation& Operation, FString& OutReason) const;

	/** `Equipment.Unequip` 的服务端执行逻辑。 */
	bool ExecuteUnequipOperation(const FYcInventoryOperation& Operation, FString& OutReason);

	/** `Equipment.Unequip` 的增量结果构建逻辑。 */
	void BuildUnequipOperationDelta(const FYcInventoryOperation& Operation, bool bSuccess, FYcInventoryOperationDelta& OutDelta) const;
	/** `Equipment.Unequip` 的预测态投影逻辑。 */
	void ProjectUnequipOperationState(const FYcInventoryOperation& Operation, FYcInventoryProjectedState& InOutProjectedState) const;
	/** 是否已经成功向 Router 注册操作处理器。 */
	bool bOperationHandlersRegistered = false;
};

/**
 * 装备栏槽位变化消息（用于 GameplayMessageSubsystem）。
 */
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
