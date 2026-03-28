// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "UObject/Interface.h"
#include "GameplayTagContainer.h"
#include "YcAbilitySourceInterface.generated.h"

class UObject;
class UPhysicalMaterial;
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
	 * 计算基于物理材质的伤害乘数
	 * 根据击中的物理材质计算伤害乘数，用于实现爆头加成、四肢减伤等效果
	 * @param PhysicalMaterial 击中的物理材质
	 * @param SourceTags 源对象的标签集合
	 * @param TargetTags 目标对象的标签集合
	 * @return 应用于基础伤害的物理材质乘数（如爆头2.0x、四肢0.8x）
	 */
	virtual float GetPhysicalMaterialMultiplier(const UPhysicalMaterial* PhysicalMaterial, const FGameplayTagContainer* SourceTags = nullptr,
												const FGameplayTagContainer* TargetTags = nullptr) const = 0;

	/**
	 * 从物理材质映射到命中部位标签
	 * 用于多部位护甲系统，根据击中的物理材质确定伤害应该作用于哪个部位
	 * @param PhysicalMaterial 击中的物理材质
	 * @return 命中部位标签（如 HitZone.Head, HitZone.Body），无法映射时返回空标签
	 */
	virtual FGameplayTag GetHitZoneFromPhysicalMaterial(const UPhysicalMaterial* PhysicalMaterial) const { return FGameplayTag(); }
};
