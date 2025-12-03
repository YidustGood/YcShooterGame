// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcInventoryItemDefinition.h"
#include "YiChenGameCore/Public/YcReplicableObject.h"
#include "YcInventoryItemInstance.generated.h"

/**
 * 根据ItemDefinition创建的Item的实例对象,支持网络复制,由InventoryManagerComponent持有(InventoryItemList,使用FastArray进行网络同步)
 * 基于FastArray提供了高效的网络同步TagsStack,可以自由的以GameplayTag为单位存储StackCount
 */
UCLASS()
class YICHENINVENTORY_API UYcInventoryItemInstance : public UYcReplicableObject
{
	GENERATED_BODY()
public:
	UYcInventoryItemInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual bool IsSupportedForNetworking() const override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	/**
	 * 在Instance所属的ItemDef上查找指定类型的Fragment
	 * @param FragmentStructType 目标Fragment结构类型
	 * @return 查找到的Fragment结构体数据
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	TInstancedStruct<FYcInventoryItemFragment> FindItemFragment(const UScriptStruct* FragmentStructType) const;
	
private:
	/* 设置Instance所属的ItemDef, 通常是在通过ItemDef创建出Instance后立刻进行设置 **/
	void SetItemDef(const FYcInventoryItemDefinition& InItemDef);

	/* 这个ItemInstance所属的物品定义类型 **/
	UPROPERTY(Replicated, BlueprintReadWrite, VisibleInstanceOnly, meta = (ExcludeBaseStruct, AllowPrivateAccess = "true"))
	FYcInventoryItemDefinition ItemDef;
};
