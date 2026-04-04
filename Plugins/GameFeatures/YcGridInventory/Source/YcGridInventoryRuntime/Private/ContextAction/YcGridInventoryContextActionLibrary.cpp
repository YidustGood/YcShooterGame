// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "ContextAction/YcGridInventoryContextActionLibrary.h"

#include "ContextAction/YcInventoryContextActionPolicyAsset.h"
#include "YcInventoryItemInstance.h"
#include "YcInventoryManagerComponent.h"

#define LOCTEXT_NAMESPACE "YcGridInventoryContextActionLibrary"

bool UYcGridInventoryContextActionLibrary::IsExecutorTagAllowed(const FGameplayTag& ExecutorTag)
{
	if (!ExecutorTag.IsValid())
	{
		return false;
	}

	const FGameplayTag AllowedRootTag = FGameplayTag::RequestGameplayTag(TEXT("Yc.Inventory.ContextAction.Executor"), false);
	if (!AllowedRootTag.IsValid())
	{
		return false;
	}
	return ExecutorTag.MatchesTag(AllowedRootTag);
}

bool UYcGridInventoryContextActionLibrary::EvaluateContextActionPolicyForItem(
	AActor* Player,
	UYcInventoryManagerComponent* PlayerInventory,
	UYcInventoryItemInstance* ItemInst,
	const FGridItemContextMenuAction& ActionDef,
	const bool bItemOperable,
	FYcContextActionEvalResult& OutResult)
{
	OutResult = FYcContextActionEvalResult();

	if (!IsValid(ItemInst))
	{
		OutResult.bPassed = false;
		OutResult.FailReason = LOCTEXT("InvalidItem", "Invalid item.");
		return false;
	}

	const UYcInventoryContextActionPolicyAsset* Policy = ActionDef.Policy.LoadSynchronous();
	if (!IsValid(Policy))
	{
		OutResult.bPassed = false;
		OutResult.FailReason = LOCTEXT("MissingPolicy", "Action policy is not configured.");
		return false;
	}
	if (IsValid(Player) && !Player->HasAuthority() && !Policy->bAllowClientPreview)
	{
		OutResult.bPassed = false;
		OutResult.FailReason = LOCTEXT("ClientPreviewNotAllowed", "Client preview is disabled for this action.");
		return false;
	}

	FYcContextActionEvalContext Context;
	Context.Player = Player;
	Context.PlayerInventory = PlayerInventory;
	Context.ItemInstance = ItemInst;
	Context.ItemInventory = ItemInst->GetInventoryManager();
	Context.ActionTag = ActionDef.ActionTag;
	Context.ExecutorTag = ActionDef.ExecutorTag;
	Context.EventTag = ActionDef.EventTag;
	Context.bItemOperable = bItemOperable;

	return EvaluatePolicyInternal(Context, Policy, OutResult);
}

bool UYcGridInventoryContextActionLibrary::EvaluatePolicyInternal(
	const FYcContextActionEvalContext& Context,
	const UYcInventoryContextActionPolicyAsset* Policy,
	FYcContextActionEvalResult& OutResult)
{
	OutResult = FYcContextActionEvalResult();

	if (!IsValid(Policy))
	{
		OutResult.bPassed = false;
		OutResult.FailReason = LOCTEXT("InvalidPolicy", "Invalid action policy.");
		return false;
	}

	bool bAllPassed = true;

	for (const UYcInventoryContextActionEvaluator* Evaluator : Policy->Evaluators)
	{
		if (!IsValid(Evaluator))
		{
			continue;
		}

		FText EvaluatorFailReason = FText::GetEmpty();
		const bool bRawPassed = Evaluator->Evaluate(Context, EvaluatorFailReason);
		const bool bPassed = Evaluator->bInvertResult ? !bRawPassed : bRawPassed;
		if (bPassed)
		{
			continue;
		}

		bAllPassed = false;
		if (OutResult.FailReason.IsEmpty())
		{
			if (Evaluator->bInvertResult && bRawPassed)
			{
				OutResult.FailReason = Evaluator->ResolveInvertedFailReason();
			}
			else
			{
				OutResult.FailReason = Evaluator->ResolveFailReason(EvaluatorFailReason);
			}
			OutResult.FailedEvaluatorTag = Evaluator->EvaluatorTag;
			OutResult.FailedEvaluatorName = Evaluator->EvaluatorName;
		}

		if (Policy->bStopOnFirstFailure)
		{
			break;
		}
	}

	OutResult.bPassed = bAllPassed;
	if (bAllPassed)
	{
		OutResult.FailReason = FText::GetEmpty();
		OutResult.FailedEvaluatorTag = FGameplayTag();
		OutResult.FailedEvaluatorName = FText::GetEmpty();
	}

	return bAllPassed;
}

#undef LOCTEXT_NAMESPACE
