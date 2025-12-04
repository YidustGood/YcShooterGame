// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "YcInventoryItemDefinition.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "StructUtils/InstancedStruct.h"
#include "YcInventoryLibrary.generated.h"

class UYcInventoryItemInstance;
class UYcInventoryManagerComponent;
/**
 * 库存系统蓝图函数库,提供一些工具函数
 */
UCLASS()
class YICHENINVENTORY_API UYcInventoryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/* 从FYcInventoryItemDefinition->Fragments中获取特定类型的Fragment */
	UFUNCTION(BlueprintCallable)
	static TInstancedStruct<FYcInventoryItemFragment> FindItemFragment(const FYcInventoryItemDefinition& ItemDef, const UScriptStruct* FragmentStructType);
	
	/* 从Actor上获取库存管理组件 **/
	UFUNCTION(BlueprintPure, Category = "Inventory")
	static UYcInventoryManagerComponent* GetInventoryManagerComponent(const AActor* Actor);
};
