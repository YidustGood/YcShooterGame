USTRUCT()
struct FYcQuestRewardItemEntry
{
	// Reward item definition id in DataRegistry.
	UPROPERTY(EditAnywhere, Category = "Quest Reward")
	FDataRegistryId ItemRegistryId;

	// Item stack count, minimum is 1.
	UPROPERTY(EditAnywhere, Category = "Quest Reward")
	int32 StackCount = 1;
}

namespace YcQuestRewardUtils
{
	void ResolveHumanPlayerPawns(TArray<AActor> &out OutPlayers, bool bIncludeBots = false)
	{
		OutPlayers.Empty();

		auto GameState = Gameplay::GetGameState();
		if (GameState == nullptr)
		{
			return;
		}

		for (auto& PlayerStatePtr : GameState.PlayerArray)
		{
			auto PlayerState = PlayerStatePtr.Get();
			if (PlayerState == nullptr)
			{
				continue;
			}

			if (!bIncludeBots && PlayerState.IsABot())
			{
				continue;
			}

			auto Pawn = Cast<APawn>(PlayerState.GetPawn());
			if (Pawn == nullptr)
			{
				continue;
			}

			OutPlayers.Add(Pawn);
		}
	}

	int32 GrantRewardsToActors(const TArray<AActor>& InTargets, const TArray<FYcQuestRewardItemEntry>& InRewardEntries, bool bEnableDebugLog = true)
	{
		if (InTargets.Num() == 0 || InRewardEntries.Num() == 0)
		{
			return 0;
		}

		int32 SuccessCount = 0;
		for (auto TargetActor : InTargets)
		{
			if (TargetActor == nullptr)
			{
				continue;
			}

			auto InventoryManager = YcInventory::GetInventoryManagerComponent(TargetActor);
			if (InventoryManager == nullptr)
			{
				if (bEnableDebugLog)
				{
					Warning(f"[QuestReward] Grant failed. Target={TargetActor.GetName()} Reason=Inventory manager not found.");
				}
				continue;
			}

			for (auto RewardEntry : InRewardEntries)
			{
				if (!RewardEntry.ItemRegistryId.IsValid())
				{
					continue;
				}

				const int32 StackCount = Math::Max(1, RewardEntry.StackCount);
				FString FailureReason;
				const bool bGranted = TryGrantRewardToInventory(InventoryManager, RewardEntry.ItemRegistryId, StackCount, FailureReason);
				if (bGranted)
				{
					++SuccessCount;
					if (bEnableDebugLog)
					{
						Log(f"[QuestReward] Grant success. Target={TargetActor.GetName()} Item={RewardEntry.ItemRegistryId.ToString()} Count={StackCount}");
					}
				}
				else if (bEnableDebugLog)
				{
					const FString EffectiveReason = FailureReason.IsEmpty() ? "Unknown reason." : FailureReason;
					Warning(f"[QuestReward] Grant failed. Target={TargetActor.GetName()} Item={RewardEntry.ItemRegistryId.ToString()} Count={StackCount} Reason={EffectiveReason}");
				}
			}
		}

		return SuccessCount;
	}

	bool TryGrantRewardToInventory(UYcInventoryManagerComponent InventoryManager, const FDataRegistryId&in ItemRegistryId, int32 StackCount, FString&out OutFailureReason)
	{
		OutFailureReason = "";

		if (InventoryManager == nullptr || !ItemRegistryId.IsValid() || StackCount <= 0)
		{
			OutFailureReason = "Invalid inventory target or reward item config.";
			return false;
		}

		auto GridInventoryManager = Cast<UGridInventoryManagerComponent>(InventoryManager);
		if (GridInventoryManager != nullptr)
		{
			FGameplayTag RegionId;
			int32 PocketIndex = -1;
			FIntPoint Tile = FIntPoint();
			bool bRotated = false;
			if (!GridInventoryManager.FindFirstFitPlacement(ItemRegistryId, RegionId, PocketIndex, Tile, bRotated))
			{
				OutFailureReason = "Grid inventory has no free space for this item.";
				return false;
			}

			const bool bAdded = GridInventoryManager.TryAddGridItemByDefinition(ItemRegistryId, StackCount, Tile, bRotated, RegionId, PocketIndex);
			if (!bAdded)
			{
				OutFailureReason = "Grid inventory placement failed while adding item.";
			}
			return bAdded;
		}

		const bool bAdded = InventoryManager.AddItem(ItemRegistryId, StackCount) != nullptr;
		if (!bAdded)
		{
			OutFailureReason = "Inventory manager rejected the reward item.";
		}
		return bAdded;
	}
}
