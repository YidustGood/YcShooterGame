// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "YcGridInventoryPlacementRuleLibrary.h"

#include "Fragments/ItemFragment_GridItem.h"
#include "YcInventoryItemInstance.h"
#include "YcInventoryLibrary.h"

namespace
{
	static void GatherStaticPlacementTags(const FDataRegistryId& ItemDefId, TArray<FGameplayTag>& OutTags)
	{
		OutTags.Reset();

		FYcInventoryItemDefinition ItemDef;
		if (!UYcInventoryLibrary::GetItemDefinition(ItemDefId, ItemDef))
		{
			return;
		}

		const auto Fragment = UYcInventoryLibrary::FindItemFragment(ItemDef, FItemFragment_GridItem::StaticStruct());
		if (!Fragment.IsValid())
		{
			return;
		}

		const FItemFragment_GridItem* GridFragment = Fragment.GetPtr<FItemFragment_GridItem>();
		if (!GridFragment)
		{
			return;
		}

		for (const FGameplayTag& Tag : GridFragment->PlacementTags)
		{
			if (Tag.IsValid() && !OutTags.Contains(Tag))
			{
				OutTags.Add(Tag);
			}
		}
	}

	static void GatherPlacementTagsForItemInstance(const UYcInventoryItemInstance* ItemInst, TArray<FGameplayTag>& OutTags)
	{
		OutTags.Reset();
		if (!IsValid(ItemInst))
		{
			return;
		}

		GatherStaticPlacementTags(ItemInst->GetItemRegistryId(), OutTags);

		TArray<FGameplayTag> OwnedTags;
		ItemInst->GetOwnedTags().GetGameplayTagArray(OwnedTags);
		for (const FGameplayTag& Tag : OwnedTags)
		{
			if (Tag.IsValid() && !OutTags.Contains(Tag))
			{
				OutTags.Add(Tag);
			}
		}
	}

	static bool IsRuleMatched(const TArray<FGameplayTag>& ItemTags, const TArray<FGameplayTag>& RuleTags, const EGridRegionTagConstraintMatchMode MatchMode)
	{
		if (RuleTags.IsEmpty())
		{
			return false;
		}

		auto IsSingleRuleMatched = [&ItemTags](const FGameplayTag& RuleTag) -> bool
		{
			if (!RuleTag.IsValid())
			{
				return false;
			}
			for (const FGameplayTag& ItemTag : ItemTags)
			{
				if (ItemTag.IsValid() && ItemTag.MatchesTag(RuleTag))
				{
					return true;
				}
			}
			return false;
		};

		if (MatchMode == EGridRegionTagConstraintMatchMode::All)
		{
			for (const FGameplayTag& RuleTag : RuleTags)
			{
				if (!IsSingleRuleMatched(RuleTag))
				{
					return false;
				}
			}
			return true;
		}

		for (const FGameplayTag& RuleTag : RuleTags)
		{
			if (IsSingleRuleMatched(RuleTag))
			{
				return true;
			}
		}
		return false;
	}

	static bool EvaluateConstraint(const TArray<FGameplayTag>& ItemTags, const FGridRegionTagConstraint& Constraint)
	{
		if (Constraint.Policy == EGridRegionTagConstraintPolicy::None || Constraint.Tags.IsEmpty())
		{
			return true;
		}

		const bool bMatched = IsRuleMatched(ItemTags, Constraint.Tags, Constraint.MatchMode);
		if (Constraint.Policy == EGridRegionTagConstraintPolicy::AllowList)
		{
			if (!bMatched)
			{
				return false;
			}
			return true;
		}

		if (Constraint.Policy == EGridRegionTagConstraintPolicy::DenyList)
		{
			if (bMatched)
			{
				return false;
			}
			return true;
		}

		return true;
	}
}

bool UYcGridInventoryPlacementRuleLibrary::PassesTagConstraintForItemDef(const FDataRegistryId& ItemDefId, const FGridRegionTagConstraint& Constraint)
{
	TArray<FGameplayTag> PlacementTags;
	GatherStaticPlacementTags(ItemDefId, PlacementTags);
	return EvaluateConstraint(PlacementTags, Constraint);
}

bool UYcGridInventoryPlacementRuleLibrary::PassesTagConstraintForItemInstance(UYcInventoryItemInstance* ItemInst, const FGridRegionTagConstraint& Constraint)
{
	TArray<FGameplayTag> PlacementTags;
	GatherPlacementTagsForItemInstance(ItemInst, PlacementTags);
	return EvaluateConstraint(PlacementTags, Constraint);
}
