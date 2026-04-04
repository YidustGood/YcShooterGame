// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "ContextAction/YcInventoryContextActionEvaluator.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayTagAssetInterface.h"
#include "YcInventoryItemInstance.h"
#include "YcInventoryManagerComponent.h"

#define LOCTEXT_NAMESPACE "YcInventoryContextActionEvaluator"

UYcInventoryContextActionEvaluator::UYcInventoryContextActionEvaluator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool UYcInventoryContextActionEvaluator::Evaluate_Implementation(const FYcContextActionEvalContext& Context, FText& OutFailReason) const
{
	OutFailReason = FText::GetEmpty();
	return true;
}

FText UYcInventoryContextActionEvaluator::ResolveFailReason(const FText& InFailReason) const
{
	if (!InFailReason.IsEmpty())
	{
		return InFailReason;
	}
	return DefaultFailReason;
}

FText UYcInventoryContextActionEvaluator::ResolveInvertedFailReason() const
{
	if (!InvertedFailReason.IsEmpty())
	{
		return InvertedFailReason;
	}
	if (!DefaultFailReason.IsEmpty())
	{
		return DefaultFailReason;
	}
	return LOCTEXT("InvertedConditionFailed", "Inverted condition failed.");
}

bool UYcEvaluator_ItemRevealed::Evaluate_Implementation(const FYcContextActionEvalContext& Context, FText& OutFailReason) const
{
	if (!Context.bItemOperable)
	{
		OutFailReason = LOCTEXT("ItemNotOperable", "Item is not operable yet.");
		return false;
	}
	OutFailReason = FText::GetEmpty();
	return true;
}

UYcEvaluator_ItemRevealed::UYcEvaluator_ItemRevealed(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EvaluatorName = LOCTEXT("Evaluator_ItemRevealed_Name", "Item Revealed");
	EvaluatorTag = FGameplayTag::RequestGameplayTag(TEXT("Yc.Inventory.ContextAction.Evaluator.ItemRevealed"), false);
	DefaultFailReason = LOCTEXT("Evaluator_ItemRevealed_DefaultFail", "Item is not operable yet.");
}

bool UYcEvaluator_PlayerAlive::Evaluate_Implementation(const FYcContextActionEvalContext& Context, FText& OutFailReason) const
{
	if (!IsValid(Context.Player))
	{
		OutFailReason = LOCTEXT("InvalidPlayer", "Invalid player.");
		return false;
	}

	if (const IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(Context.Player))
	{
		FGameplayTagContainer OwnedTags;
		TagInterface->GetOwnedGameplayTags(OwnedTags);

		const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Death.Dead"), false);
		const FGameplayTag DeathTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Death"), false);
		const FGameplayTag DyingTag = FGameplayTag::RequestGameplayTag(TEXT("Status.Death.Dying"), false);

		if ((DeadTag.IsValid() && OwnedTags.HasTag(DeadTag))
			|| (DyingTag.IsValid() && OwnedTags.HasTag(DyingTag))
			|| (DeathTag.IsValid() && OwnedTags.HasTag(DeathTag)))
		{
			OutFailReason = LOCTEXT("PlayerDead", "Player is dead.");
			return false;
		}
	}

	OutFailReason = FText::GetEmpty();
	return true;
}

UYcEvaluator_PlayerAlive::UYcEvaluator_PlayerAlive(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EvaluatorName = LOCTEXT("Evaluator_PlayerAlive_Name", "Player Alive");
	EvaluatorTag = FGameplayTag::RequestGameplayTag(TEXT("Yc.Inventory.ContextAction.Evaluator.PlayerAlive"), false);
	DefaultFailReason = LOCTEXT("Evaluator_PlayerAlive_DefaultFail", "Player is not in valid state.");
}

bool UYcEvaluator_HasStackCount::Evaluate_Implementation(const FYcContextActionEvalContext& Context, FText& OutFailReason) const
{
	const int32 RequiredCount = FMath::Max(1, RequiredStackCount);

	UYcInventoryManagerComponent* SourceInventory = Context.ItemInventory.Get();
	if (!IsValid(SourceInventory) && IsValid(Context.ItemInstance))
	{
		SourceInventory = Context.ItemInstance->GetInventoryManager();
	}

	if (!IsValid(SourceInventory) || !IsValid(Context.ItemInstance))
	{
		OutFailReason = LOCTEXT("NoInventoryOrItem", "Cannot resolve item inventory.");
		return false;
	}

	const int32 CurrentStackCount = SourceInventory->GetStackCountByItemInstance(Context.ItemInstance);
	if (CurrentStackCount < RequiredCount)
	{
		OutFailReason = FText::Format(
			LOCTEXT("NeedStackCount", "Need at least {0} stack(s)."),
			FText::AsNumber(RequiredCount));
		return false;
	}

	OutFailReason = FText::GetEmpty();
	return true;
}

UYcEvaluator_HasStackCount::UYcEvaluator_HasStackCount(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EvaluatorName = LOCTEXT("Evaluator_HasStackCount_Name", "Has Stack Count");
	EvaluatorTag = FGameplayTag::RequestGameplayTag(TEXT("Yc.Inventory.ContextAction.Evaluator.HasStackCount"), false);
	DefaultFailReason = LOCTEXT("Evaluator_HasStackCount_DefaultFail", "Not enough stack count.");
}

bool UYcEvaluator_InAllowedInventoryScope::Evaluate_Implementation(const FYcContextActionEvalContext& Context, FText& OutFailReason) const
{
	if (!IsValid(Context.PlayerInventory))
	{
		OutFailReason = LOCTEXT("NoPlayerInventory", "Player inventory is invalid.");
		return false;
	}

	UYcInventoryManagerComponent* SourceInventory = Context.ItemInventory.Get();
	if (!IsValid(SourceInventory) && IsValid(Context.ItemInstance))
	{
		SourceInventory = Context.ItemInstance->GetInventoryManager();
	}
	if (!IsValid(SourceInventory))
	{
		OutFailReason = LOCTEXT("NoSourceInventory", "Item source inventory is invalid.");
		return false;
	}

	const bool bInPlayerInventory = SourceInventory == Context.PlayerInventory;
	if (bInPlayerInventory && !bAllowPlayerInventory)
	{
		OutFailReason = LOCTEXT("DisallowPlayerInventory", "This action is disabled in player inventory.");
		return false;
	}

	if (!bInPlayerInventory && !bAllowContainerInventory)
	{
		OutFailReason = LOCTEXT("DisallowContainerInventory", "This action is disabled in container inventory.");
		return false;
	}

	OutFailReason = FText::GetEmpty();
	return true;
}

UYcEvaluator_InAllowedInventoryScope::UYcEvaluator_InAllowedInventoryScope(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EvaluatorName = LOCTEXT("Evaluator_InAllowedInventoryScope_Name", "Allowed Inventory Scope");
	EvaluatorTag = FGameplayTag::RequestGameplayTag(TEXT("Yc.Inventory.ContextAction.Evaluator.InAllowedInventoryScope"), false);
	DefaultFailReason = LOCTEXT("Evaluator_InAllowedInventoryScope_DefaultFail", "Action is not allowed in current inventory scope.");
}

namespace
{
	static bool CompareAttributeValue(
		const float LeftValue,
		const EYcContextAttributeComparisonOp Op,
		const float RightValue,
		const float Tolerance)
	{
		switch (Op)
		{
		case EYcContextAttributeComparisonOp::Greater:
			return LeftValue > RightValue;
		case EYcContextAttributeComparisonOp::GreaterOrEqual:
			return LeftValue >= RightValue;
		case EYcContextAttributeComparisonOp::Less:
			return LeftValue < RightValue;
		case EYcContextAttributeComparisonOp::LessOrEqual:
			return LeftValue <= RightValue;
		case EYcContextAttributeComparisonOp::Equal:
			return FMath::IsNearlyEqual(LeftValue, RightValue, Tolerance);
		case EYcContextAttributeComparisonOp::NotEqual:
			return !FMath::IsNearlyEqual(LeftValue, RightValue, Tolerance);
		default:
			return false;
		}
	}

	static FString ComparisonOpToString(const EYcContextAttributeComparisonOp Op)
	{
		switch (Op)
		{
		case EYcContextAttributeComparisonOp::Greater:
			return TEXT(">");
		case EYcContextAttributeComparisonOp::GreaterOrEqual:
			return TEXT(">=");
		case EYcContextAttributeComparisonOp::Less:
			return TEXT("<");
		case EYcContextAttributeComparisonOp::LessOrEqual:
			return TEXT("<=");
		case EYcContextAttributeComparisonOp::Equal:
			return TEXT("==");
		case EYcContextAttributeComparisonOp::NotEqual:
			return TEXT("!=");
		default:
			return TEXT("?");
		}
	}
}

UYcEvaluator_GASAttributeCompare::UYcEvaluator_GASAttributeCompare(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EvaluatorName = LOCTEXT("Evaluator_GASAttributeCompare_Name", "GAS Attribute Compare");
	EvaluatorTag = FGameplayTag::RequestGameplayTag(TEXT("Yc.Inventory.ContextAction.Evaluator.GASAttributeCompare"), false);
	DefaultFailReason = LOCTEXT("Evaluator_GASAttributeCompare_DefaultFail", "GAS attribute requirement not met.");
}

bool UYcEvaluator_GASAttributeCompare::Evaluate_Implementation(const FYcContextActionEvalContext& Context, FText& OutFailReason) const
{
	if (!IsValid(Context.Player))
	{
		OutFailReason = LOCTEXT("GASAttr_InvalidPlayer", "Invalid player.");
		return false;
	}
	if (!Attribute.IsValid())
	{
		OutFailReason = LOCTEXT("GASAttr_InvalidAttribute", "Attribute is not configured.");
		return false;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Context.Player);
	if (!IsValid(ASC))
	{
		OutFailReason = LOCTEXT("GASAttr_NoASC", "Player has no AbilitySystemComponent.");
		return false;
	}

	float LeftValue = ASC->GetNumericAttribute(Attribute);
	float RightValue = ThresholdValue;

	if (bUseNormalizedComparison)
	{
		if (!MaxAttribute.IsValid())
		{
			OutFailReason = LOCTEXT("GASAttr_InvalidMaxAttribute", "MaxAttribute is required for normalized comparison.");
			return false;
		}

		const float MaxValue = ASC->GetNumericAttribute(MaxAttribute);
		if (MaxValue <= UE_SMALL_NUMBER)
		{
			OutFailReason = LOCTEXT("GASAttr_InvalidMaxValue", "MaxAttribute value must be greater than zero.");
			return false;
		}

		LeftValue = LeftValue / MaxValue;
		RightValue = FMath::Clamp(ThresholdValue, 0.0f, 1.0f);
	}

	const bool bPass = CompareAttributeValue(LeftValue, ComparisonOp, RightValue, FMath::Max(0.0f, Tolerance));
	if (!bPass)
	{
		OutFailReason = FText::Format(
			LOCTEXT("GASAttr_CompareFailed", "Attribute value {0} must be {1} {2}."),
			FText::AsNumber(LeftValue),
			FText::FromString(ComparisonOpToString(ComparisonOp)),
			FText::AsNumber(RightValue));
		return false;
	}

	OutFailReason = FText::GetEmpty();
	return true;
}

UYcEvaluator_ASCGameplayTagQuery::UYcEvaluator_ASCGameplayTagQuery(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	EvaluatorName = LOCTEXT("Evaluator_ASCGameplayTagQuery_Name", "ASC Gameplay Tag Query");
	EvaluatorTag = FGameplayTag::RequestGameplayTag(TEXT("Yc.Inventory.ContextAction.Evaluator.ASCGameplayTagQuery"), false);
	DefaultFailReason = LOCTEXT("Evaluator_ASCGameplayTagQuery_DefaultFail", "ASC tag requirement not met.");
}

bool UYcEvaluator_ASCGameplayTagQuery::Evaluate_Implementation(const FYcContextActionEvalContext& Context, FText& OutFailReason) const
{
	if (!IsValid(Context.Player))
	{
		OutFailReason = LOCTEXT("ASCTag_InvalidPlayer", "Invalid player.");
		return false;
	}

	UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Context.Player);
	if (!IsValid(ASC))
	{
		OutFailReason = LOCTEXT("ASCTag_NoASC", "Player has no AbilitySystemComponent.");
		return false;
	}
	FGameplayTagContainer OwnedTags;
	ASC->GetOwnedGameplayTags(OwnedTags);

	bool bPass = false;
	switch (MatchMode)
	{
	case EYcContextTagMatchMode::HasTag:
		if (!RequiredTag.IsValid())
		{
			OutFailReason = LOCTEXT("ASCTag_HasTagInvalid", "RequiredTag is not configured.");
			return false;
		}
		bPass = ASC->HasMatchingGameplayTag(RequiredTag);
		break;

	case EYcContextTagMatchMode::NotHasTag:
		if (!RequiredTag.IsValid())
		{
			OutFailReason = LOCTEXT("ASCTag_NotHasTagInvalid", "RequiredTag is not configured.");
			return false;
		}
		bPass = !ASC->HasMatchingGameplayTag(RequiredTag);
		break;

	case EYcContextTagMatchMode::HasAll:
		bPass = ASC->HasAllMatchingGameplayTags(RequiredTags);
		break;

	case EYcContextTagMatchMode::HasAny:
		bPass = ASC->HasAnyMatchingGameplayTags(RequiredTags);
		break;

	case EYcContextTagMatchMode::NotHasAll:
		bPass = !ASC->HasAllMatchingGameplayTags(RequiredTags);
		break;

	case EYcContextTagMatchMode::NotHasAny:
		bPass = !ASC->HasAnyMatchingGameplayTags(RequiredTags);
		break;

	case EYcContextTagMatchMode::MatchTagQuery:
		if (RequiredTagQuery.IsEmpty())
		{
			OutFailReason = LOCTEXT("ASCTag_QueryEmpty", "RequiredTagQuery is empty.");
			return false;
		}
		bPass = RequiredTagQuery.Matches(OwnedTags);
		break;

	case EYcContextTagMatchMode::NotMatchTagQuery:
		if (RequiredTagQuery.IsEmpty())
		{
			OutFailReason = LOCTEXT("ASCTag_NotQueryEmpty", "RequiredTagQuery is empty.");
			return false;
		}
		bPass = !RequiredTagQuery.Matches(OwnedTags);
		break;

	default:
		bPass = false;
		break;
	}

	if (!bPass)
	{
		OutFailReason = DefaultFailReason;
		return false;
	}

	OutFailReason = FText::GetEmpty();
	return true;
}

#undef LOCTEXT_NAMESPACE
