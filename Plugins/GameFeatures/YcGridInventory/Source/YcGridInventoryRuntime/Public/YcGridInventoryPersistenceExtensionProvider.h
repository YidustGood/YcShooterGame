// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"
#include "System/YcInventoryPersistenceExtensionProvider.h"
#include "YcItemInstanceId.h"
#include "YcGridInventoryPersistenceExtensionProvider.generated.h"

class UGridInventoryManagerComponent;
class UYcInventoryManagerComponent;
class UYcInventoryItemInstance;

/** 单个网格物品的落位记录。 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FYcGridInventoryPlacementRecord
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FYcItemInstanceId ItemInstId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FIntPoint GridTile = FIntPoint::ZeroValue;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	bool bRotated = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	FGameplayTag GridRegionId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	int32 GridPocketIndex = -1;
};

/** GridInventory 持久化扩展载荷。 */
USTRUCT(BlueprintType)
struct YCGRIDINVENTORYRUNTIME_API FYcGridInventoryExtensionPayload
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
	TArray<FYcGridInventoryPlacementRecord> Placements;
};

/** GridInventory 的持久化扩展 Provider。 */
UCLASS()
class YCGRIDINVENTORYRUNTIME_API UYcGridInventoryPersistenceExtensionProvider final : public UYcInventoryPersistenceExtensionProvider
{
	GENERATED_BODY()

public:
	virtual FName GetExtensionKey() const override;
	virtual int32 GetExtensionVersion() const override;
	virtual bool CanHandleInventory(const UYcInventoryManagerComponent* Inventory) const override;
	virtual bool BuildInventoryExtensionPayload(const UYcInventoryManagerComponent* Inventory, FInstancedStruct& OutPayload, FString& OutReason) const override;
	virtual bool ApplyInventoryExtensionPayload(UYcInventoryManagerComponent* Inventory, const FInstancedStruct& Payload, const TMap<FYcItemInstanceId, TObjectPtr<UYcInventoryItemInstance>>& ItemMap, FString& OutReason) const override;

private:
	static constexpr int32 CurrentVersion = 1;
	static const FName ExtensionKey;
	static UGridInventoryManagerComponent* AsGrid(UYcInventoryManagerComponent* Inventory);
	static const UGridInventoryManagerComponent* AsGrid(const UYcInventoryManagerComponent* Inventory);
};
