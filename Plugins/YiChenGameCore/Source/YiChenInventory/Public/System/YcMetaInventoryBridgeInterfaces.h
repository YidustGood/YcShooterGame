// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"
#include "YcMetaInventoryBridgeInterfaces.generated.h"

class UYcInventoryItemInstance;

/**
 * 装备槽桥接接口：用于 MetaInventory 在不依赖 YiChenEquipment 的前提下访问装备槽数据。
 */
UINTERFACE(BlueprintType)
class YICHENINVENTORY_API UYcMetaInventoryEquipmentBridge : public UInterface
{
	GENERATED_BODY()
};

class YICHENINVENTORY_API IYcMetaInventoryEquipmentBridge
{
	GENERATED_BODY()

public:
	/** 获取所有已占用槽位标签。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory|MetaBridge")
	void GetMetaOccupiedSlots(TArray<FGameplayTag>& OutSlots) const;

	/** 获取指定槽位中的物品。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory|MetaBridge")
	UYcInventoryItemInstance* GetMetaItemInSlot(FGameplayTag SlotTag) const;

	/** 卸下指定槽位。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory|MetaBridge")
	UYcInventoryItemInstance* MetaUnequipSlot(FGameplayTag SlotTag);

	/** 装备物品。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory|MetaBridge")
	bool MetaEquipItem(UYcInventoryItemInstance* ItemInstance);

	/** 通知槽位状态已更新（用于触发客户端UI刷新）。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory|MetaBridge")
	void MetaNotifySlotsUpdated();
};

/**
 * 快捷栏桥接接口：用于 MetaInventory 在不依赖 YiChenEquipment 的前提下访问快捷栏数据。
 */
UINTERFACE(BlueprintType)
class YICHENINVENTORY_API UYcMetaInventoryQuickBarBridge : public UInterface
{
	GENERATED_BODY()
};

class YICHENINVENTORY_API IYcMetaInventoryQuickBarBridge
{
	GENERATED_BODY()

public:
	/** 获取快捷栏槽位数组。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory|MetaBridge")
	void GetMetaQuickBarSlots(TArray<UYcInventoryItemInstance*>& OutSlots) const;

	/** 从指定槽位移除物品。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory|MetaBridge")
	UYcInventoryItemInstance* MetaRemoveQuickBarSlot(int32 SlotIndex);

	/** 向指定槽位添加物品。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory|MetaBridge")
	bool MetaAddQuickBarSlot(int32 SlotIndex, UYcInventoryItemInstance* Item);

	/** 通知槽位状态已更新（用于触发客户端UI刷新）。 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory|MetaBridge")
	void MetaNotifyQuickBarSlotsUpdated();
};

