// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "DataRegistryId.h"
#include "Engine/DataTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "YcPickupable.h"
#include "YcGridItemContainerLibrary.generated.h"

class UGridInventoryManagerComponent;

/** 关卡物资容器的物品刷新策略。 */
UENUM(BlueprintType)
enum class EYcContainerItemSpawnStrategy : uint8
{
	/** 兼容旧版本：直接读取 Pickupable.StaticInventory.Templates。 */
	LegacyStaticInventory,

	/** 从可复用的 Loot Pool 中随机抽取物品并放入容器。 */
	RandomPool,
};

/** Loot Pool 中的单个候选条目。 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FYcContainerLootPoolEntry
{
	GENERATED_BODY()

public:
	/** 物品定义 ID。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (RegistryType = "InventoryItem"))
	FDataRegistryId ItemRegistryId;

	/** 抽中该条目时生成的堆叠数。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "1"))
	int32 StackCount = 1;
};

/** 可复用的容器随机掉落池定义。 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FYcContainerLootPoolRow : public FTableRowBase
{
	GENERATED_BODY()

public:
	/** 候选物品列表。随机刷新时从这里有放回抽样。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<FYcContainerLootPoolEntry> Candidates;

	/** 默认抽取次数。Actor 可通过 Override 覆盖。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = "0"))
	int32 SpawnCount = 0;
};

/**
 * 关卡物资容器相关工具函数。
 * 负责按照策略把配置中的物品装入网格库存。
 */
UCLASS()
class YCGRIDINVENTORYRUNTIME_API UYcGridItemContainerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 从 DataRegistry 读取 Loot Pool 定义。 */
	UFUNCTION(BlueprintPure, Category = "GridInventory|Container")
	static bool GetLootPoolRow(const FDataRegistryId& LootPoolId, FYcContainerLootPoolRow& OutLootPoolRow);

	/**
	 * 按策略向容器中填充初始物品。
	 * @return 成功放入容器的物品条目数量。
	 */
	UFUNCTION(BlueprintCallable, Category = "GridInventory|Container")
	static int32 PopulateContainerInventory(
		UGridInventoryManagerComponent* InventoryManager,
		const FYcInventoryPickup& LegacyPickupInventory,
		EYcContainerItemSpawnStrategy SpawnStrategy,
		const FDataRegistryId& LootPoolId,
		int32 SpawnCountOverride = 0);
};
