// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "StructUtils/InstancedStruct.h"
#include "YcInventoryItemDefinition.generated.h"

class UYcInventoryItemInstance;

/**
 * 为FYcInventoryItemDefinition提供扩展信息/功能的结构体,也可以理解为为物品提供单一能力,创建子结构体实现不同的功能,并在ItemDef中进行组合
 * 会在FYcInventoryItemDefinition中定义一个数组进行存放，并且全局只存在一份实例
 */
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcInventoryItemFragment
{
	GENERATED_BODY()
	
	virtual ~FYcInventoryItemFragment() = default;
	
	// 物品实例创建时的回调
	virtual void OnInstanceCreated(UYcInventoryItemInstance* Instance) const;
};

//@TODO 后续看是否考虑优化这个基本物品定义,例如不要额外的物品名称描述等,单独做成一个Fragment,这样不是所有的Item都需要为此付出内存代价
USTRUCT(BlueprintType)
struct YICHENINVENTORY_API FYcInventoryItemDefinition : public FTableRowBase
{
	GENERATED_BODY()
	
	/** 物品唯一ID */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Item")
	FName ItemId;
	
	/** 物品显示名称 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Item")
	FText DisplayName;
	
	/** 物品的本地化描述 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Description")
	FText Description = FText::GetEmpty();

	/** 物品的实例对象类 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category= "Item")
	TSubclassOf<UYcInventoryItemInstance> ItemInstanceType;
	
	/** 物品片段,可以通过添加自定义片段实现扩展物品信息/功能 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ExcludeBaseStruct))
	TArray<TInstancedStruct<FYcInventoryItemFragment>> Fragments;
};