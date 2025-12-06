// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "YcAbilityTagRelationshipMapping.generated.h"

/**
 * 技能标签关系定义结构
 * 定义一个技能标签与其他技能标签之间的阻止、取消和激活依赖关系
 */
USTRUCT()
struct FYcAbilityTagRelationship
{
	GENERATED_BODY()

	/**
	 * 技能标签
	 * 此结构中其他标签容器的关系基于这个标签
	 * 一个技能可以有多个AbilityTag（每个有不同的关系配置）
	 */
	UPROPERTY(EditAnywhere, Category = Ability, meta = (Categories = "Gameplay.Action"))
	FGameplayTag AbilityTag;

	/**
	 * 当此AbilityTag激活时，会阻止的其他技能标签
	 * 即：如果一个技能有此容器中的任何标签，它将无法激活
	 */
	UPROPERTY(EditAnywhere, Category = Ability)
	FGameplayTagContainer AbilityTagsToBlock;

	/**
	 * 当此AbilityTag激活时，会取消的其他技能标签
	 * 即：如果一个技能有此容器中的任何标签，它将被取消
	 */
	UPROPERTY(EditAnywhere, Category = Ability)
	FGameplayTagContainer AbilityTagsToCancel;

	/**
	 * 当此AbilityTag激活时，需要先具有的其他技能标签
	 * 即：此容器中的标签是激活AbilityTag的前置条件
	 */
	UPROPERTY(EditAnywhere, Category = Ability)
	FGameplayTagContainer ActivationRequiredTags;

	/**
	 * 当此AbilityTag激活时，会阻止的其他技能标签
	 * 即：如果当前具有此容器中的任何标签，则AbilityTag无法激活
	 */
	UPROPERTY(EditAnywhere, Category = Ability)
	FGameplayTagContainer ActivationBlockedTags;
};

/**
 * 技能标签关系映射配置资源
 * 
 * 定义技能标签之间的相互影响关系，包括：
 *   - 阻止关系：某个技能激活时，会阻止其他技能的激活
 *   - 取消关系：某个技能激活时，会取消其他已激活的技能
 *   - 激活依赖：某个技能激活需要满足的前置条件标签
 *   - 激活阻止：某个技能激活时会被阻止的标签条件
 * 
 * 这是一个DataAsset资源，可在编辑器中配置多个FYcAbilityTagRelationship关系，
 * 由UYcAbilitySystemComponent在技能激活时查询使用，以实现复杂的技能相互制约逻辑。
 */
UCLASS()
class YCGAMECORE_API UYcAbilityTagRelationshipMapping : public UDataAsset
{
	GENERATED_BODY()

private:
	/**
	 * 技能标签关系列表
	 * 存储所有定义的技能标签之间的阻止、取消和激活依赖关系
	 * 编辑器中可通过此数组配置技能间的相互影响规则
	 */
	UPROPERTY(EditAnywhere, Category = Ability, meta=(TitleProperty="AbilityTag"))
	TArray<FYcAbilityTagRelationship> AbilityTagRelationships;

public:
	/**
	 * 查询技能标签应该阻止和取消的其他标签
	 * 遍历AbilityTagRelationships，找出与给定标签匹配的关系，收集所有应该被阻止和取消的标签
	 * @param AbilityTags 要查询的技能标签集合
	 * @param OutTagsToBlock 输出：应该被阻止的标签列表
	 * @param OutTagsToCancel 输出：应该被取消的标签列表
	 */
	void GetAbilityTagsToBlockAndCancel(const FGameplayTagContainer& AbilityTags, FGameplayTagContainer* OutTagsToBlock, FGameplayTagContainer* OutTagsToCancel) const;

	/**
	 * 查询技能激活所需的前置条件标签和阻止标签
	 * 根据技能标签获取其激活时的依赖关系，用于判断技能是否可以激活
	 * @param AbilityTags 要查询的技能标签集合
	 * @param OutActivationRequired 输出：激活所需的前置条件标签
	 * @param OutActivationBlocked 输出：激活时被阻止的标签
	 */
	void GetRequiredAndBlockedActivationTags(const FGameplayTagContainer& AbilityTags, FGameplayTagContainer* OutActivationRequired, FGameplayTagContainer* OutActivationBlocked) const;

	/**
	 * 判断指定的技能标签是否会被某个标签取消
	 * 检查是否存在一个关系，其AbilityTag为ActionTag，且AbilityTagsToCancel包含AbilityTags中的任何标签
	 * @param AbilityTags 要检查的技能标签集合
	 * @param ActionTag 动作标签
	 * @return 如果AbilityTags中的任何标签会被ActionTag取消，则返回true
	 */
	bool IsAbilityCancelledByTag(const FGameplayTagContainer& AbilityTags, const FGameplayTag& ActionTag) const;
};
