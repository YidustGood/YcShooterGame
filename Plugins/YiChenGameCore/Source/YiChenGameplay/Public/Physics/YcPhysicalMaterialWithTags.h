// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "YcPhysicalMaterialWithTags.generated.h"

/**
 * UYcPhysicalMaterialWithTags - 带有 GameplayTag 的物理材质
 * 
 * 继承自 UPhysicalMaterial，扩展了 GameplayTag 容器功能。
 * 允许通过 GameplayTag 来标识和区分不同的物理材质，
 * 使游戏逻辑可以基于标签进行更灵活的判断和处理。
 * 
 * 主要用途：
 * 1. 命中部位判定：通过标签区分头部、身体、四肢等部位
 *    - 例如：Tag "HitReaction.Head" 表示头部，可用于爆头伤害加成
 * 
 * 2. 材质类型识别：通过标签区分不同材质类型
 *    - 例如：Tag "Surface.Metal" 表示金属表面
 *    - 例如：Tag "Surface.Flesh" 表示肉体表面
 * 
 * 3. 特效选择：根据标签选择不同的命中特效
 *    - 金属材质播放火花特效
 *    - 肉体材质播放血液特效
 *    - 混凝土材质播放碎片特效
 * 
 * 4. 音效选择：根据标签选择不同的命中音效
 *    - 不同材质有不同的撞击声音
 * 
 * 5. 伤害修正：根据标签应用不同的伤害倍率
 *    - 护甲部位减少伤害
 *    - 弱点部位增加伤害
 * 
 * 使用方式：
 * 1. 在编辑器中创建此类的资产
 * 2. 配置 Tags 属性，添加相应的 GameplayTag
 * 3. 将物理材质分配给骨骼网格体的不同部位
 * 4. 在射线检测命中后，通过 HitResult.PhysMaterial 获取物理材质
 * 5. 转换为 UYcPhysicalMaterialWithTags 并读取 Tags 进行逻辑判断
 * 
 * 代码示例：
 * @code
 * // 从命中结果获取物理材质
 * if (const UPhysicalMaterial* PhysMat = HitResult.PhysMaterial.Get())
 * {
 *     if (const UYcPhysicalMaterialWithTags* TaggedMat = Cast<UYcPhysicalMaterialWithTags>(PhysMat))
 *     {
 *         // 检查是否命中头部
 *         if (TaggedMat->Tags.HasTag(FGameplayTag::RequestGameplayTag("HitReaction.Head")))
 *         {
 *             DamageMultiplier = 2.0f; // 爆头双倍伤害
 *         }
 *     }
 * }
 * @endcode
 */
UCLASS()
class YICHENGAMEPLAY_API UYcPhysicalMaterialWithTags : public UPhysicalMaterial
{
	GENERATED_BODY()
public:
	/**
	 * 构造函数
	 * @param ObjectInitializer - UE 对象初始化器
	 */
	UYcPhysicalMaterialWithTags(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * GameplayTag 容器
	 * 
	 * 存储与此物理材质关联的所有 GameplayTag。
	 * 游戏代码可以通过这些标签来判断材质类型、命中部位等信息。
	 * 
	 * 常用标签示例：
	 * - HitReaction.Head      : 头部
	 * - HitReaction.Body      : 身体
	 * - HitReaction.Limb      : 四肢
	 * - Surface.Metal         : 金属表面
	 * - Surface.Flesh         : 肉体表面
	 * - Surface.Concrete      : 混凝土表面
	 * - Armor.Heavy           : 重型护甲
	 * - Armor.Light           : 轻型护甲
	 * - Weakness.Critical     : 弱点/要害
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=PhysicalProperties)
	FGameplayTagContainer Tags;
};