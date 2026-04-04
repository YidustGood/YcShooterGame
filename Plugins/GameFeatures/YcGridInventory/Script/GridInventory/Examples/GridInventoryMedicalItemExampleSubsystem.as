/**
 * Medical item context-action example (world subsystem):
 * - Listen context-action request messages from grid-inventory framework.
 * - When request event tag matches heal event, restore health from FItemFragment_Heal.
 * - Consume item stack on successful use.
 *
 * This is a world-level singleton service. No need to attach per-player components.
 */
class UGridInventoryMedicalItemExampleSubsystem : UScriptWorldSubsystem
{
	private FGameplayMessageListenerHandle ContextActionRequestHandle;

	UFUNCTION(BlueprintOverride)
	void Initialize()
	{
		FGameplayTag RequestTag = FGameplayTag::RequestGameplayTag(n"Yc.Inventory.Message.Grid.ContextAction.Request");
		ContextActionRequestHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			RequestTag,
			this,
			n"OnContextActionRequest",
			FGridItemContextActionRequest(),
			EGameplayMessageMatch::ExactMatch);
	}

	UFUNCTION(BlueprintOverride)
	void Deinitialize()
	{
		ContextActionRequestHandle.Unregister();
	}

	UFUNCTION()
	void OnContextActionRequest(FGameplayTag ActualTag, FGridItemContextActionRequest Request)
	{
		if (Request.Player == nullptr || Request.ItemInst == nullptr)
		{
			return;
		}

		FGameplayTag HealEventTag = FGameplayTag::RequestGameplayTag(n"Item.Action.Event.Heal");
		if (Request.EventTag != HealEventTag)
		{
			return;
		}

		FInstancedStruct HealFragmentResult = Request.ItemInst.FindItemFragment(FItemFragment_Heal);
		if (!HealFragmentResult.IsValid())
		{
			Warning("Heal context action ignored: item has no FItemFragment_Heal.");
			return;
		}
		FItemFragment_Heal HealDef = HealFragmentResult.Get(FItemFragment_Heal);
		if (HealDef.HealAmount <= 0.0f)
		{
			return;
		}

		auto HealthComp = UYcHealthComponent::FindHealthComponent(Request.Player);
		if (HealthComp == nullptr)
		{
			Warning("Heal context action failed: player has no UYcHealthComponent.");
			return;
		}

		float CurrentHealth = HealthComp.GetHealth();
		float MaxHealth = HealthComp.GetMaxHealth();
		if (CurrentHealth >= MaxHealth)
		{
			return;
		}

		AActor ItemOuterActor = Request.ItemInst.GetActorOuter();
		if (ItemOuterActor == nullptr)
		{
			Warning("Heal context action failed: item has no valid outer actor.");
			return;
		}
		auto InventoryComp = Cast<UYcInventoryManagerComponent>(YcInventory::GetInventoryManagerComponent(ItemOuterActor));
		if (InventoryComp == nullptr)
		{
			Warning("Heal context action failed: item source inventory component not found.");
			return;
		}

		float HealToApply = Math::Min(HealDef.HealAmount, MaxHealth - CurrentHealth);
		if (HealToApply <= 0.0f)
		{
			return;
		}

		if (HealDef.bConsumeOnUse)
		{
			int32 ConsumeCount = Math::Max(1, HealDef.ConsumeCount);
			if (!InventoryComp.ConsumeItemInstance(Request.ItemInst, ConsumeCount))
			{
				Warning("Heal context action failed: consume item stack failed.");
				return;
			}
		}

		if (!HealthComp.ApplyHealing(HealToApply))
		{
			Warning("Heal context action failed: apply healing failed.");
			return;
		}
	}
}
