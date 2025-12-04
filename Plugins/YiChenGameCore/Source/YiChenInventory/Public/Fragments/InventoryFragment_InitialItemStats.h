// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "YcInventoryItemDefinition.h"
#include "InventoryFragment_InitialItemStats.generated.h"

/**
 * 初始物品数据状态功能片段, 通过GameplayTag和数值键值对配置物品属性, 在物品实例创建的时候初始化到ItemInst中以实现动态状态和网络复制
 * 例如：(武器备用子弹Tag,子弹数量)
 * 注意该片段静态配置然后实例化是设置到ItemInst, 由ItemInst负责动态管理和网络复制
 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FInventoryFragment_InitialItemStats : public FYcInventoryItemFragment
{
	GENERATED_BODY()
	
	UPROPERTY(EditDefaultsOnly, Category="Data")
	TMap<FGameplayTag, int32> InitialItemStats;
	
	virtual void OnInstanceCreated(UYcInventoryItemInstance* Instance) const override;

	int32 GetItemStatByTag(FGameplayTag Tag) const;
};