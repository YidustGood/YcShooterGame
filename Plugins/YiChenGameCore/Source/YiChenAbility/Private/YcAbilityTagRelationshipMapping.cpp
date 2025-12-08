// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcAbilityTagRelationshipMapping.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAbilityTagRelationshipMapping)

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