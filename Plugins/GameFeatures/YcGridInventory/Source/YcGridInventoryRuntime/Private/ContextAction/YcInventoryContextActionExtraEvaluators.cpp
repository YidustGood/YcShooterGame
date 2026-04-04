// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "ContextAction/YcInventoryContextActionExtraEvaluators.h"

#include "YcInventoryItemInstance.h"

#define LOCTEXT_NAMESPACE "YcInventoryContextActionExtraEvaluators"

namespace
{
	static bool EvaluateTagMatchMode(
		const FGameplayTagContainer& OwnedTags,
		const EYcContextTagMatchMode MatchMode,
		const FGameplayTag& RequiredTag,
		const FGameplayTagContainer& RequiredTags,
		const FGameplayTagQuery& RequiredTagQuery,
		FText& OutFailReason)
	{
		switch (MatchMode)
		{
		case EYcContextTagMatchMode::HasTag:
			if (!RequiredTag.IsValid())
			{
				OutFailReason = LOCTEXT("TagMode_HasTagInvalid", "RequiredTag is not configured.");
				return false;
			}
			return OwnedTags.HasTag(RequiredTag);

		case EYcContextTagMatchMode::NotHasTag:
			if (!RequiredTag.IsValid())
			{
				OutFailReason = LOCTEXT("TagMode_NotHasTagInvalid", "RequiredTag is not configured.");
				return false;
			}
			return !OwnedTags.HasTag(RequiredTag);

		case EYcContextTagMatchMode::HasAll:
			return OwnedTags.HasAll(RequiredTags);

		case EYcContextTagMatchMode::HasAny:
			return OwnedTags.HasAny(RequiredTags);

		case EYcContextTagMatchMode::NotHasAll:
			return !OwnedTags.HasAll(RequiredTags);

		case EYcContextTagMatchMode::NotHasAny:
			return !OwnedTags.HasAny(RequiredTags);

		case EYcContextTagMatchMode::MatchTagQuery:
			if (RequiredTagQuery.IsEmpty())
			{
				OutFailReason = LOCTEXT("TagMode_QueryEmpty", "RequiredTagQuery is empty.");
				return false;
			}
			return RequiredTagQuery.Matches(OwnedTags);

		case EYcContextTagMatchMode::NotMatchTagQuery:
			if (RequiredTagQuery.IsEmpty())
			{
				OutFailReason = LOCTEXT("TagMode_NotQueryEmpty", "RequiredTagQuery is empty.");
				return false;
			}
			return !RequiredTagQuery.Matches(OwnedTags);

		default:
			OutFailReason = LOCTEXT("TagMode_Invalid", "Invalid tag match mode.");
			return false;
		}
	}

	static bool EvaluateChildWithInvert(
		const UYcInventoryContextActionEvaluator* Child,
		const FYcContextActionEvalContext& Context,
		FText& OutFailReason)
	{
		FText ChildReason = FText::GetEmpty();
		const bool bRawPass = Child->Evaluate(Context, ChildReason);
		const bool bPass = Child->bInvertResult ? !bRawPass : bRawPass;
		if (!bPass)
		{
			if (Child->bInvertResult && bRawPass)
			{
				OutFailReason = Child->ResolveInvertedFailReason();
			}
			else
			{
				OutFailReason = Child->ResolveFailReason(ChildReason);
			}
		}
		return bPass;
	}
}

UYcEvaluator_ItemOwnedTagQuery::UYcEvaluator_ItemOwnedTagQuery(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EvaluatorName = LOCTEXT("Evaluator_ItemOwnedTagQuery_Name", "Item Owned Tag Query");
	EvaluatorTag = FGameplayTag::RequestGameplayTag(TEXT("Yc.Inventory.ContextAction.Evaluator.ItemOwnedTagQuery"), false);
	DefaultFailReason = LOCTEXT("Evaluator_ItemOwnedTagQuery_DefaultFail", "Item tag requirement not met.");
}

bool UYcEvaluator_ItemOwnedTagQuery::Evaluate_Implementation(const FYcContextActionEvalContext& Context, FText& OutFailReason) const
{
	if (!IsValid(Context.ItemInstance))
	{
		OutFailReason = LOCTEXT("ItemTag_InvalidItem", "Invalid item.");
		return false;
	}

	FGameplayTagContainer OwnedTags;
	Context.ItemInstance->GetOwnedGameplayTags(OwnedTags);

	const bool bPass = EvaluateTagMatchMode(
		OwnedTags,
		MatchMode,
		RequiredTag,
		RequiredTags,
		RequiredTagQuery,
		OutFailReason);

	if (!bPass && OutFailReason.IsEmpty())
	{
		OutFailReason = DefaultFailReason;
	}
	return bPass;
}

UYcEvaluator_Composite::UYcEvaluator_Composite(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EvaluatorName = LOCTEXT("Evaluator_Composite_Name", "Composite Evaluator");
	EvaluatorTag = FGameplayTag::RequestGameplayTag(TEXT("Yc.Inventory.ContextAction.Evaluator.Composite"), false);
	DefaultFailReason = LOCTEXT("Evaluator_Composite_DefaultFail", "Composite condition failed.");
}

bool UYcEvaluator_Composite::Evaluate_Implementation(const FYcContextActionEvalContext& Context, FText& OutFailReason) const
{
	if (Children.Num() <= 0)
	{
		if (bPassWhenNoChildren)
		{
			OutFailReason = FText::GetEmpty();
			return true;
		}
		OutFailReason = EmptyChildrenFailReason.IsEmpty() ? DefaultFailReason : EmptyChildrenFailReason;
		return false;
	}

	if (LogicOp == EYcContextCompositeLogicOp::All)
	{
		bool bAllPass = true;
		for (const UYcInventoryContextActionEvaluator* Child : Children)
		{
			if (!IsValid(Child) || Child == this)
			{
				continue;
			}

			FText ChildFailReason = FText::GetEmpty();
			const bool bChildPass = EvaluateChildWithInvert(Child, Context, ChildFailReason);
			if (!bChildPass)
			{
				bAllPass = false;
				OutFailReason = ChildFailReason.IsEmpty() ? DefaultFailReason : ChildFailReason;
				if (bStopOnFirstResult)
				{
					return false;
				}
			}
		}

		if (bAllPass)
		{
			OutFailReason = FText::GetEmpty();
			return true;
		}
		if (OutFailReason.IsEmpty())
		{
			OutFailReason = DefaultFailReason;
		}
		return false;
	}

	// LogicOp == Any
	bool bAnyPass = false;
	FText FirstFailReason = FText::GetEmpty();
	for (const UYcInventoryContextActionEvaluator* Child : Children)
	{
		if (!IsValid(Child) || Child == this)
		{
			continue;
		}

		FText ChildFailReason = FText::GetEmpty();
		const bool bChildPass = EvaluateChildWithInvert(Child, Context, ChildFailReason);
		if (bChildPass)
		{
			bAnyPass = true;
			OutFailReason = FText::GetEmpty();
			if (bStopOnFirstResult)
			{
				return true;
			}
		}
		else if (FirstFailReason.IsEmpty())
		{
			FirstFailReason = ChildFailReason;
		}
	}

	if (bAnyPass)
	{
		OutFailReason = FText::GetEmpty();
		return true;
	}
	OutFailReason = FirstFailReason.IsEmpty() ? DefaultFailReason : FirstFailReason;
	return false;
}

#undef LOCTEXT_NAMESPACE
