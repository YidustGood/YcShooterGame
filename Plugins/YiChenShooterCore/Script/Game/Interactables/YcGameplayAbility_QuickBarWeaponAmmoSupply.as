class UYcGameplayAbility_QuickBarWeaponAmmoSupply : UYcGameplayAbility
{
	default NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	UPROPERTY(EditAnywhere, Category = "Supply")
	bool bDestroySourceActorOnSuccess = false;

	UPROPERTY(EditAnywhere, Category = "Supply", meta = (ClampMin = "0.0"))
	float SourceActorDestroyDelay = 0.1f;

	UFUNCTION(BlueprintOverride)
	bool CanActivateAbility(FGameplayAbilityActorInfo InActorInfo, FGameplayAbilitySpecHandle Handle,
							FGameplayTagContainer& RelevantTags) const
	{
		AActor AvatarActor = InActorInfo.AvatarActor;
		if (AvatarActor == nullptr)
		{
			return false;
		}

		auto QuickBarComponent = UYcQuickBarComponent::FindQuickBarComponent(AvatarActor);
		if (QuickBarComponent == nullptr)
		{
			return false;
		}

		auto SlotItems = QuickBarComponent.GetSlots();
		for (auto ItemInst : SlotItems)
		{
			if (ShouldSupplyItem(ItemInst))
			{
				return true;
			}
		}

		return false;
	}

	UFUNCTION(BlueprintOverride)
	void ActivateAbility()
	{
		if (!HasAuthority())
		{
			EndAbility();
			return;
		}

		auto AvatarActor = GetAvatarActorFromActorInfo();
		auto QuickBarComponent = UYcQuickBarComponent::FindQuickBarComponent(AvatarActor);
		if (QuickBarComponent == nullptr)
		{
			Warning("QuickBarWeaponAmmoSupply: QuickBarComponent is null");
			EndAbility();
			return;
		}

		bool bSuppliedAnyItem = false;
		auto SlotItems = QuickBarComponent.GetSlots();
		for (auto ItemInst : SlotItems)
		{
			bSuppliedAnyItem = SupplyItemAmmoToConfiguredMax(ItemInst) || bSuppliedAnyItem;
		}

		if (bSuppliedAnyItem && bDestroySourceActorOnSuccess)
		{
			DestroyCurrentSourceActor();
		}

		if (bSuppliedAnyItem)
		{
			NotifySourceActorSupplyUsed(AvatarActor);
		}

		EndAbility();
	}

	bool ShouldSupplyItem(UYcInventoryItemInstance ItemInst) const
	{
		if (ItemInst == nullptr)
		{
			return false;
		}

		int32 TargetMagazineSize = 0;
		int32 TargetMagazineAmmo = 0;
		int32 TargetSpareAmmo = 0;
		if (!ResolveTargetAmmoValues(ItemInst, TargetMagazineSize, TargetMagazineAmmo, TargetSpareAmmo))
		{
			return false;
		}

		return ItemInst.GetStatTagStackCount(GameplayTags::Weapon_Stat_MagazineSize) != TargetMagazineSize || ItemInst.GetStatTagStackCount(GameplayTags::Weapon_Stat_MagazineAmmo) != TargetMagazineAmmo || ItemInst.GetStatTagStackCount(GameplayTags::Weapon_Stat_SpareAmmo) != TargetSpareAmmo;
	}

	bool SupplyItemAmmoToConfiguredMax(UYcInventoryItemInstance ItemInst)
	{
		if (ItemInst == nullptr)
		{
			return false;
		}

		int32 TargetMagazineSize = 0;
		int32 TargetMagazineAmmo = 0;
		int32 TargetSpareAmmo = 0;
		if (!ResolveTargetAmmoValues(ItemInst, TargetMagazineSize, TargetMagazineAmmo, TargetSpareAmmo))
		{
			return false;
		}

		bool bChanged = false;
		bChanged = SetItemIntStat(ItemInst, GameplayTags::Weapon_Stat_MagazineSize, TargetMagazineSize) || bChanged;
		bChanged = SetItemIntStat(ItemInst, GameplayTags::Weapon_Stat_MagazineAmmo, TargetMagazineAmmo) || bChanged;
		bChanged = SetItemIntStat(ItemInst, GameplayTags::Weapon_Stat_SpareAmmo, TargetSpareAmmo) || bChanged;
		return bChanged;
	}

	bool ResolveTargetAmmoValues(UYcInventoryItemInstance ItemInst, int32&out OutMagazineSize, int32&out OutMagazineAmmo, int32&out OutSpareAmmo) const
	{
		auto InitialStatsFragment = ItemInst.FindItemFragment(FInventoryFragment_InitialItemStats);
		if (!InitialStatsFragment.IsValid())
		{
			return false;
		}

		auto InitialStats = InitialStatsFragment.Get(FInventoryFragment_InitialItemStats).InitialItemStats;

		int32 InitialMagazineSize = 0;
		if (!InitialStats.Find(GameplayTags::Weapon_Stat_MagazineSize, InitialMagazineSize) || InitialMagazineSize <= 0)
		{
			return false;
		}

		int32 InitialSpareAmmo = 0;
		InitialStats.Find(GameplayTags::Weapon_Stat_SpareAmmo, InitialSpareAmmo);

		OutMagazineSize = InitialMagazineSize;
		OutMagazineAmmo = InitialMagazineSize;
		OutSpareAmmo = InitialSpareAmmo;
		return true;
	}

	bool SetItemIntStat(UYcInventoryItemInstance ItemInst, FGameplayTag StatTag, int32 TargetValue)
	{
		const int32 CurrentValue = ItemInst.GetStatTagStackCount(StatTag);
		if (CurrentValue == TargetValue)
		{
			return false;
		}

		if (CurrentValue < TargetValue)
		{
			ItemInst.AddStatTagStack(StatTag, TargetValue - CurrentValue);
		}
		else
		{
			ItemInst.RemoveStatTagStack(StatTag, CurrentValue - TargetValue);
		}

		return true;
	}

	void DestroyCurrentSourceActor()
	{
		if (CurrentSourceObject == nullptr)
		{
			return;
		}

		AActor SourceActor = Cast<AActor>(CurrentSourceObject.GetOuter());
		if (SourceActor == nullptr)
		{
			SourceActor = Cast<AActor>(CurrentSourceObject);
		}

		if (SourceActor != nullptr)
		{
			SourceActor.SetLifeSpan(Math::Max(0.0f, SourceActorDestroyDelay));
		}
	}

	void NotifySourceActorSupplyUsed(AActor Interactor)
	{
		if (CurrentSourceObject == nullptr)
		{
			return;
		}

		auto AmmoSupplyActor = Cast<AYcAmmoSupplyActor>(CurrentSourceObject.GetOuter());
		if (AmmoSupplyActor == nullptr)
		{
			AmmoSupplyActor = Cast<AYcAmmoSupplyActor>(CurrentSourceObject);
		}

		if (AmmoSupplyActor != nullptr)
		{
			AmmoSupplyActor.HandleAmmoSupplyInteracted(Interactor);
		}
	}
}
