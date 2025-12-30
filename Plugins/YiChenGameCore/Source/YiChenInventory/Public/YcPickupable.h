// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "DataRegistryId.h"
#include "UObject/Interface.h"
#include "YcPickupable.generated.h"

struct FYcInventoryItemDefinition;
class UYcInventoryItemInstance;

/** 
 * 拾取物的物品模板结构体
 * 包含物品的个数和物品定义数据, 库存组件通过这些数据可以添加物品
 */
USTRUCT(BlueprintType)
struct FYcPickupTemplate
{
	GENERATED_BODY()

public:
	/** 这个拾取物的堆叠个数 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	int32 StackCount = 1;
	
	/** 
	 * 物品在DataRegistry中的ID
	 * 通过DataRegistry统一管理物品定义，支持多数据源聚合查询
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	FDataRegistryId ItemRegistryId;
};

/** 拾取物的物品实例对象 */
USTRUCT(BlueprintType)
struct FYcPickupInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TObjectPtr<UYcInventoryItemInstance> Item = nullptr;
};

/** 
 * 拾取物配置结构体
 * 可以配置多个FYcPickupTemplate、FYcPickupInstance
 * 通过配置多个物品数据可实现一个可拾取物品拾取后为玩家库存添加多个物品
 */
USTRUCT(BlueprintType)
struct FYcInventoryPickup
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FYcPickupTemplate> Templates;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly)
	TArray<FYcPickupInstance> Instances;
};

UINTERFACE(MinimalAPI, BlueprintType, meta = (CannotImplementInterfaceInBlueprint))
class UYcPickupable : public UInterface
{
	GENERATED_BODY()
};

/**
 * 可拾取接口, 实现该接口提供获得拾取物配置的功能, 然后就可以通过拾取物配置往库存中添加物品
 */
class YICHENINVENTORY_API IYcPickupable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	virtual FYcInventoryPickup GetPickupInventory() const = 0;
};