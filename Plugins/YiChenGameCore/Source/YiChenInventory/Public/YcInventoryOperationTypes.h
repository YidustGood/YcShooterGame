// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "GameplayTagContainer.h"
#include "YcInventoryOperationTypes.generated.h"

class AActor;
class UYcInventoryManagerComponent;
class UYcInventoryItemInstance;

/** 客户端预测态快照。 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcInventoryProjectedState
{
	GENERATED_BODY()

	/** 各库存网格的预测版本号（Key 通常为库存组件名）。 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TMap<FName, int32> InventoryGridRevision;

	/** 预测态装备槽占用。 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TMap<FGameplayTag, TObjectPtr<UYcInventoryItemInstance>> EquipmentSlots;

	/** 预测态快捷栏槽位占用。 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TMap<int32, TObjectPtr<UYcInventoryItemInstance>> QuickBarSlots;
};

/** 库存操作事件类型（提交/确认/拒绝）。 */
UENUM(BlueprintType)
enum class EYcInventoryOperationEvent : uint8
{
	/** 操作已进入本地 Pending 队列。 */
	Submitted,
	/** 操作已被服务端确认。 */
	Acked,
	/** 操作已被服务端拒绝。 */
	Nacked
};

/** 服务端处理操作后的结果。 */
UENUM(BlueprintType)
enum class EYcInventoryOperationProcessResult : uint8
{
	/** 处理成功，操作生效。 */
	Succeeded,
	/** 处理失败，操作未生效。 */
	Failed,
	/** 延迟处理（异步流程稍后回执）。 */
	Deferred
};

/** 操作结果增量（供 UI 与日志消费）。 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcInventoryOperationDelta
{
	GENERATED_BODY()

	/** 人类可读的结果摘要。 */
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	FString Summary;

	/** 受影响的物品实例 ID 列表。 */
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	TArray<FName> AffectedItemIds;

	/** 受影响的槽位列表（如装备槽 TagName）。 */
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	TArray<FName> AffectedSlots;
};

/**
 * 统一库存操作载体。
 * 用于承载 Inventory / Equipment / QuickBar 等系统的统一请求。
 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcInventoryOperation
{
	GENERATED_BODY()

	/** 客户端本地操作 ID（<=0 时由提交端分配）。 */
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	int64 OpId = 0;

	/** 操作类型（例如 Inventory.SwapGrid / Equipment.Equip）。 */
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	FName OpType = NAME_None;

	/** 客户端提交时看到的权威版本号。 */
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	int32 BaseRevision = -1;

	/** 客户端时间戳（秒）。 */
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	double ClientTimestamp = 0.0;

	/** 请求发起者（通常是 Pawn / Controller）。 */
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	TObjectPtr<AActor> RequestActor = nullptr;

	/** 请求发起方库存组件（用于服务端还原执行上下文）。 */
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	TObjectPtr<UYcInventoryManagerComponent> RequestInventory = nullptr;

	/** 源库存组件。 */
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	TObjectPtr<UYcInventoryManagerComponent> SourceInventory = nullptr;

	/** 目标库存组件。 */
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	TObjectPtr<UYcInventoryManagerComponent> TargetInventory = nullptr;

	/** 目标物品实例。 */
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	TObjectPtr<UYcInventoryItemInstance> ItemInstance = nullptr;

	/** 数量参数（拆分/转移/消耗等）。 */
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	int32 StackCount = 1;

	/** 网格坐标参数（格子类操作）。 */
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	FIntPoint GridTile = FIntPoint::ZeroValue;

	/** 网格旋转参数。 */
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	bool bRotated = false;

	/** 槽位 Tag 参数（装备类操作）。 */
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	FGameplayTag SlotTag;

	/** 槽位索引参数（快捷栏类操作）。 */
	UPROPERTY(BlueprintReadWrite, Category = Inventory)
	int32 SlotIndex = INDEX_NONE;
};

/** 操作状态变化消息（用于 UI 订阅）。 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcInventoryOperationStateMessage
{
	GENERATED_BODY()

	/** Router 所属 Owner。 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	TObjectPtr<AActor> Owner = nullptr;

	/** 当前事件类型。 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	EYcInventoryOperationEvent Event = EYcInventoryOperationEvent::Submitted;

	/** 触发本次状态变化的操作。 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	FYcInventoryOperation Operation;

	/** 最新权威版本号。 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	int32 AuthoritativeRevision = 0;

	/** 当前预测版本号。 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	int32 ProjectedRevision = 0;

	/** 当前 Pending 数量。 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	int32 PendingCount = 0;

	/** 补充描述文本（失败原因/摘要等）。 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	FString Detail;

	/** 本次操作对应增量。 */
	UPROPERTY(BlueprintReadOnly, Category = Inventory)
	FYcInventoryOperationDelta Delta;
};

/** 前置校验委托：返回是否通过，失败时写入原因。 */
DECLARE_DELEGATE_RetVal_TwoParams(bool, FYcInventoryOperationValidateDelegate, const FYcInventoryOperation&, FString&);
/** 执行委托：返回是否成功，失败时写入原因。 */
DECLARE_DELEGATE_RetVal_TwoParams(bool, FYcInventoryOperationExecuteDelegate, const FYcInventoryOperation&, FString&);
/** 增量构建委托：根据执行结果输出 Delta。 */
DECLARE_DELEGATE_ThreeParams(FYcInventoryOperationBuildDeltaDelegate, const FYcInventoryOperation&, bool /*bSuccess*/, FYcInventoryOperationDelta& /*OutDelta*/);
/** 预测态投影委托：将操作结果投影到 Router 预测态。 */
DECLARE_DELEGATE_TwoParams(FYcInventoryOperationProjectStateDelegate, const FYcInventoryOperation&, FYcInventoryProjectedState& /*InOutProjectedState*/);

/** Router 中注册的操作处理器配置。 */
struct YICHENINVENTORY_API FYcInventoryOperationHandler
{
	/** 校验逻辑。 */
	FYcInventoryOperationValidateDelegate Validate;
	/** 执行逻辑。 */
	FYcInventoryOperationExecuteDelegate Execute;
	/** 增量构建逻辑。 */
	FYcInventoryOperationBuildDeltaDelegate BuildDelta;
	/** 预测态投影逻辑。 */
	FYcInventoryOperationProjectStateDelegate ProjectState;
	/** 前缀匹配时优先级（越大越优先）。 */
	int32 Priority = 0;
	/** 是否按前缀匹配 OpType。 */
	bool bPrefixMatch = false;
	/** 是否启用此处理器。 */
	bool bEnabled = true;
};
