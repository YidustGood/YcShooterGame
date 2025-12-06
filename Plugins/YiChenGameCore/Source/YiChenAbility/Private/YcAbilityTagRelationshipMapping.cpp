// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcAbilityTagRelationshipMapping.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAbilityTagRelationshipMapping)

/**
 * 获取技能标签对应的阻止和取消标签
 * 
 * @param AbilityTags        技能标签容器
 * @param OutTagsToBlock     输出阻止标签容器
 * @param OutTagsToCancel    输出取消标签容器
 */
void UYcAbilityTagRelationshipMapping::GetAbilityTagsToBlockAndCancel(const FGameplayTagContainer& AbilityTags, FGameplayTagContainer* OutTagsToBlock, FGameplayTagContainer* OutTagsToCancel) const
{
	// 遍历所有技能标签关系，找出与给定标签匹配的关系，收集应该阻止和取消的标签
	for (int32 i = 0; i < AbilityTagRelationships.Num(); i++)
	{
		const FYcAbilityTagRelationship& Tags = AbilityTagRelationships[i];
		if (AbilityTags.HasTag(Tags.AbilityTag))
		{
			if (OutTagsToBlock)
			{
				OutTagsToBlock->AppendTags(Tags.AbilityTagsToBlock);
			}
			if (OutTagsToCancel)
			{
				OutTagsToCancel->AppendTags(Tags.AbilityTagsToCancel);
			}
		}
	}
}

/**
 * 获取技能标签对应的激活依赖和阻止条件
 * 
 * @param AbilityTags           技能标签容器
 * @param OutActivationRequired 输出激活依赖标签容器
 * @param OutActivationBlocked  输出激活阻止标签容器
 */
void UYcAbilityTagRelationshipMapping::GetRequiredAndBlockedActivationTags(const FGameplayTagContainer& AbilityTags, FGameplayTagContainer* OutActivationRequired,
																		   FGameplayTagContainer* OutActivationBlocked) const
{
	// 遍历所有技能标签关系，找出给定技能标签的激活依赖和阻止条件
	for (int32 i = 0; i < AbilityTagRelationships.Num(); i++)
	{
		const FYcAbilityTagRelationship& Tags = AbilityTagRelationships[i];
		if (AbilityTags.HasTag(Tags.AbilityTag))
		{
			if (OutActivationRequired)
			{
				OutActivationRequired->AppendTags(Tags.ActivationRequiredTags);
			}
			if (OutActivationBlocked)
			{
				OutActivationBlocked->AppendTags(Tags.ActivationBlockedTags);
			}
		}
	}
}

/**
 * 检查ActionTag是否定义了取消AbilityTags中任何标签的关系
 * 
 * @param AbilityTags 技能标签容器
 * @param ActionTag   动作标签
 * @return 是否取消
 */
bool UYcAbilityTagRelationshipMapping::IsAbilityCancelledByTag(const FGameplayTagContainer& AbilityTags, const FGameplayTag& ActionTag) const
{
	// 检查ActionTag是否定义了取消AbilityTags中任何标签的关系
	for (int32 i = 0; i < AbilityTagRelationships.Num(); i++)
	{
		const FYcAbilityTagRelationship& Tags = AbilityTagRelationships[i];

		if (Tags.AbilityTag == ActionTag && Tags.AbilityTagsToCancel.HasAny(AbilityTags))
		{
			return true;
		}
	}

	return false;
}