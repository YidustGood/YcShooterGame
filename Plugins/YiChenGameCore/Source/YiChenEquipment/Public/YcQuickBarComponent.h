// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Components/ControllerComponent.h"
#include "YcQuickBarComponent.generated.h"

class UYcInventoryItemInstance;
class UYcEquipmentInstance;
class UYcEquipmentManagerComponent;
/**
 * 快捷物品栏组件, 提供插槽用于关联库存物品，用于实现可以快速装备或使用并且互斥的库存物品, 例如库存中的武器、道具等, 互斥的意思是不能同时装备或使用
 */
UCLASS(ClassGroup=(YiChenGameCore), Blueprintable, meta=(BlueprintSpawnableComponent))
class YICHENEQUIPMENT_API UYcQuickBarComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	UYcQuickBarComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void BeginPlay() override;
	
	/**
	 * 激活指定序号的插槽物品
	 * @param NewIndex 要激活的插槽序号
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="YcGameCore")
	void SetActiveSlotIndex(int32 NewIndex);
	
	/**
	 * 激活指定序号的插槽物品
	 * @param NewIndex 要激活的插槽序号
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category="YcGameCore")
	void DeactivateSlotIndex(int32 NewIndex);
	
	// 向前循环激活插槽中的装备
	UFUNCTION(BlueprintCallable, Category="YiChenGameCore")
	void CycleActiveSlotForward();

	// 向后循环激活插槽中的装备
	UFUNCTION(BlueprintCallable, Category="YiChenGameCore")
	void CycleActiveSlotBackward(); 
	
	// 获取当前所有的插槽物品对象
	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	TArray<UYcInventoryItemInstance*> GetSlots() const
	{
		return Slots;
	}

	// 获取当前激活的插槽序号
	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	int32 GetActiveSlotIndex() const { return ActiveSlotIndex; }

	// 获取当前激活的插槽中的库存物品
	UFUNCTION(BlueprintCallable, BlueprintPure = false)
	UYcInventoryItemInstance* GetActiveSlotItem() const;

	// 获取下一个空闲的插槽
	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	int32 GetNextFreeItemSlot() const;

	// 获取指定插槽中的物品
	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	UYcInventoryItemInstance* GetSlotItem(int32 ItemIndex) const;
	
	/**
	 * 添加指定物品至指定插槽中(如果目标插槽已存在物品,将会被顶替)
	 * @param SlotIndex 要添加的插槽位置
	 * @param Item 物品实例
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	bool AddItemToSlot(int32 SlotIndex, UYcInventoryItemInstance* Item);

	// 移除指定插槽中的物品，返回值是被移除的物品
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
	UYcInventoryItemInstance* RemoveItemFromSlot(int32 SlotIndex);

private:
	// 装备当前激活插槽中的物品
	void EquipItemInSlot();

	// 取消装备当前激活插槽中的物品
	void UnequipItemInSlot();

	// 根据下标设置Slots的值,并标记为脏,触发网络同步，所有对Slots的更改都应调用这个函数进行
	void SetSlotsByIndex_Internal(const int32 Index, UYcInventoryItemInstance* InItem);

	void SetActiveSlotIndex_Internal(const int32 NewActiveIndex);

	// 从组件Owner->Pawn 查找装备管理组件对象
	UYcEquipmentManagerComponent* FindEquipmentManager() const;
	
protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "QuickBar", meta=(AllowPrivateAccess = "true"))
	int32 NumSlots = 8;

	UFUNCTION()
	void OnRep_Slots();

	UFUNCTION()
	void OnRep_ActiveSlotIndex(int32 InLastActiveSlotIndex);
	
private:
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_Slots, Category = "QuickBar")
	TArray<TObjectPtr<UYcInventoryItemInstance>> Slots;
	
	UPROPERTY(ReplicatedUsing=OnRep_ActiveSlotIndex)
	int32 ActiveSlotIndex = -1;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category= "QuickBar", meta=(AllowPrivateAccess = "true"))
	int32 LastActiveSlotIndex;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "QuickBar", meta=(AllowPrivateAccess = "true"))
	TObjectPtr<UYcEquipmentInstance> EquippedInst;
	
public:
	FORCEINLINE int32 GetLastActiveSlotIndex () const { return LastActiveSlotIndex; };
	FORCEINLINE UYcEquipmentInstance* GetEquippedInstance() const { return EquippedInst; };
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
