// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "YcAbilitySourceInterface.generated.h"

class UObject;
class UPhysicalMaterial;
struct FGameplayTagContainer;

/**
 * 技能效果计算源接口
 * 为技能效果提供源对象信息，用于计算距离衰减和物理材质衰减等效果修正
 */
UINTERFACE()
class UYcAbilitySourceInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * 技能效果计算源接口实现
 * 定义技能效果源对象需要实现的接口，用于提供衰减计算相关的数据
 */
class YICHENABILITY_API IYcAbilitySourceInterface
{
	GENERATED_IINTERFACE_BODY()
	
	/**
	 * 计算基于距离的效果衰减乘数
	 * 根据源到目标的距离计算效果衰减，用于模拟远距离伤害递减等效果
	 * @param Distance 源到目标的距离（如枪弹飞行距离）
	 * @param SourceTags 源对象的标签集合
	 * @param TargetTags 目标对象的标签集合
	 * @return 应用于基础属性值的距离衰减乘数
	 */
	virtual float GetDistanceAttenuation(float Distance, const FGameplayTagContainer* SourceTags = nullptr, const FGameplayTagContainer* TargetTags = nullptr) const = 0;

	/**
	 * 计算基于物理材质的效果衰减乘数
	 * 根据击中的物理材质计算效果衰减，用于模拟不同材质的防护效果
	 * @param PhysicalMaterial 击中的物理材质
	 * @param SourceTags 源对象的标签集合
	 * @param TargetTags 目标对象的标签集合
	 * @return 应用于基础属性值的物理材质衰减乘数
	 */
	virtual float GetPhysicalMaterialAttenuation(const UPhysicalMaterial* PhysicalMaterial, const FGameplayTagContainer* SourceTags = nullptr,
												 const FGameplayTagContainer* TargetTags = nullptr) const = 0;
};
