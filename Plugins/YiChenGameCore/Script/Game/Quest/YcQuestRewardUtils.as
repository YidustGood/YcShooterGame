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
					Warning(f"[QuestReward] Inventory manager not found. Target={TargetActor.GetName()}");
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
				auto CreatedItem = InventoryManager.AddItem(RewardEntry.ItemRegistryId, StackCount);
				if (CreatedItem != nullptr)
				{
					++SuccessCount;
					if (bEnableDebugLog)
					{
						Log(f"[QuestReward] Grant success. Target={TargetActor.GetName()} Item={RewardEntry.ItemRegistryId.ToString()} Count={StackCount}");
					}
				}
				else if (bEnableDebugLog)
				{
					Warning(f"[QuestReward] Grant failed. Target={TargetActor.GetName()} Item={RewardEntry.ItemRegistryId.ToString()} Count={StackCount}");
				}
			}
		}

		return SuccessCount;
	}
}
