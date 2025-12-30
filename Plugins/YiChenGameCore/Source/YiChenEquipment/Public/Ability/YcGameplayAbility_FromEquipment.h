// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcInventoryItemDefinition.h"
#include "Abilities/YcGameplayAbility.h"
#include "Fragments/YcEquipmentFragment.h"
#include "StructUtils/InstancedStruct.h"
#include "YcGameplayAbility_FromEquipment.generated.h"

class UYcInventoryItemInstance;
class UYcEquipmentInstance;
/**
 * 来自装备所提供的GA基类
 * 在游戏中所有由装备提供给玩家的技能都应该从这个类继承, 在这里我们实现了一些便捷的工具函数
 */
UCLASS()
class YICHENEQUIPMENT_API UYcGameplayAbility_FromEquipment : public UYcGameplayAbility
{
	GENERATED_BODY()
public:
	UYcGameplayAbility_FromEquipment(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	/**
	 * 获取该技能关联的装备实例对象
	 * @return 关联的EquipmentInst对象
	 */
	UFUNCTION(BlueprintCallable, Category="YcGameCore|Ability")
	UYcEquipmentInstance* GetAssociatedEquipment() const;
	
	/**
	 * 获取与本技能对象关联的库存物品实例对象
	 * @return 与本技能相关联的ItemInst对象
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Ability")
	UYcInventoryItemInstance* GetAssociatedItem() const;
	
	/**
	 * 在Ability所属的所属的ItemDef上查找指定类型的Fragment
	 * @param FragmentStructType 目标Fragment结构类型
	 * @return 查找到的Fragment结构体数据
	 * 注意：此函数会产生两次 Fragment 的深拷贝。如果频繁调用，建议缓存结果。或者使用蓝图函数库。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false)
	TInstancedStruct<FYcInventoryItemFragment> FindItemFragment(const UScriptStruct* FragmentStructType) const;
	
	/**
	 * 蓝图版本：查询装备定义中的 Fragment
	 * 从装备定义的 Fragments 数组中查找指定类型的 Fragment
	 * @param FragmentStructType 目标 Fragment 结构类型（必须是 FYcEquipmentFragment 或其子类）
	 * @return 查找到的 Fragment 结构体数据，如果不存在则返回空结构体
	 * 注意：此函数会产生两次 Fragment 的深拷贝。如果频繁调用，建议缓存结果。或者使用蓝图函数库。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure=false, Category = "Equipment")
	TInstancedStruct<FYcEquipmentFragment> FindEquipmentFragment(const UScriptStruct* FragmentStructType) const;

#if WITH_EDITOR	// 编辑器模式下
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
};