// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "Fragments/YcEquipmentFragment.h"
#include "YcEquipmentVisualLibrary.generated.h"

class UYcEquipmentInstance;
class UYcEquipmentVisualData;

/**
 * 装备视觉资产蓝图函数库
 * 
 * 提供获取UYcEquipmentVisualData在蓝图中的便捷操作接口
 */
UCLASS()
class YICHENEQUIPMENT_API UYcEquipmentVisualLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/**
	 * 从装备实例获取已加载的视觉数据（组合函数）
	 * @param Equipment 装备实例
	 * @return 视觉数据对象，未找到或未加载返回 nullptr
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Equipment|Visual",
		meta = (DisplayName = "Get Equipment Visual Data"))
	static UYcEquipmentVisualData* GetEquipmentVisualData(UYcEquipmentInstance* Equipment);
};
