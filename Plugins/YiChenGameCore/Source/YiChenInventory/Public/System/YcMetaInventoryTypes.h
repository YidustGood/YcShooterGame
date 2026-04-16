// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DataRegistryId.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "System/YcInventorySceneTypes.h"
#include "YcItemInstanceId.h"
#include "YcMetaInventoryTypes.generated.h"

/** 整数标签值。 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcMetaTagIntValue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Value = 0;
};

/** 浮点标签值。 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcMetaTagFloatValue
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FGameplayTag Tag;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	float Value = 0.0f;
};

/** 物品扩展状态载荷。 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcMetaItemExtensionPayload
{
	GENERATED_BODY()

	/** 扩展键（模块唯一）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FName ExtensionKey = NAME_None;

	/** 扩展数据版本号。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Version = 1;

	/** 扩展载荷（建议由扩展方自行定义编码）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<uint8> PayloadBytes;
};

/** 单个库存物品记录（用于快照序列化）。 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcMetaInventoryItemRecord
{
	GENERATED_BODY()

	/** 物品实例唯一ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FYcItemInstanceId ItemInstId;

	/** 物品定义ID（DataRegistry）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FDataRegistryId ItemRegistryId;

	/** 堆叠数量。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 StackCount = 1;

	/** 整数标签堆叠（Tag -> Value）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FYcMetaTagIntValue> IntTagStacks;

	/** 浮点标签堆叠（Tag -> Value）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FYcMetaTagFloatValue> FloatTagStacks;

	/** 物品拥有标签。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FGameplayTagContainer OwnedTags;

	/** 扩展状态载荷（按 ExtensionKey 分发，未知项可透传回写）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FYcMetaItemExtensionPayload> ExtensionPayloads;
};

/** 库存级扩展载荷（由功能插件扩展库存快照）。 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcMetaInventoryExtensionPayload
{
	GENERATED_BODY()

	/** 扩展键（模块唯一）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FName ExtensionKey = NAME_None;

	/** 扩展数据版本号。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 Version = 1;

	/** 扩展结构化载荷。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FInstancedStruct Payload;
};

UENUM(BlueprintType)
enum class EYcMetaItemSourceScope : uint8
{
	/** 物品来源于玩家库存。 */
	PlayerInventory,
	/** 物品来源于仓库/容器库存。 */
	StashInventory,
	/** 未知来源（恢复时按回退规则尝试）。 */
	Unknown
};

/** 装备槽位记录。 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcMetaEquipmentSlotRecord
{
	GENERATED_BODY()

	/** 装备槽位Tag。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FGameplayTag SlotTag;

	/** 槽位绑定的物品实例ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FYcItemInstanceId ItemInstId;

	/** 物品来源范围。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	EYcMetaItemSourceScope SourceScope = EYcMetaItemSourceScope::PlayerInventory;
};

/** 快捷栏槽位记录。 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcMetaQuickBarSlotRecord
{
	GENERATED_BODY()

	/** 快捷栏槽位索引。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 SlotIndex = INDEX_NONE;

	/** 槽位绑定的物品实例ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FYcItemInstanceId ItemInstId;

	/** 物品来源范围。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	EYcMetaItemSourceScope SourceScope = EYcMetaItemSourceScope::PlayerInventory;
};

/** 玩家侧快照（背包+装备+快捷栏）。 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcMetaPlayerSnapshot
{
	GENERATED_BODY()

	/** 玩家库存物品记录。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FYcMetaInventoryItemRecord> InventoryItems;

	/** 玩家库存扩展载荷。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FYcMetaInventoryExtensionPayload> InventoryExtensions;

	/** 玩家装备槽位记录。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FYcMetaEquipmentSlotRecord> EquipmentSlots;

	/** 玩家快捷栏槽位记录。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FYcMetaQuickBarSlotRecord> QuickBarSlots;
};

/** 仓库侧快照。 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcMetaStashSnapshot
{
	GENERATED_BODY()

	/** 仓库库存物品记录。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FYcMetaInventoryItemRecord> InventoryItems;

	/** 仓库库存扩展载荷。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FYcMetaInventoryExtensionPayload> InventoryExtensions;
};

/** 账号级库存根快照。 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcMetaInventoryRootSnapshot
{
	GENERATED_BODY()

	/** 账号ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FString AccountId;

	/** 快照版本号（用于版本闸门）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 SnapshotVersion = 0;

	/** 最近保存时间（Unix时间戳，秒）。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int64 LastSavedUnixTime = 0;

	/** 玩家侧快照。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FYcMetaPlayerSnapshot Player;

	/** 仓库侧快照。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FYcMetaStashSnapshot Stash;
};
