enum EYcAmmoSupplyPromptState
{
	Available,
	Cooldown,
	Exhausted
}

class AYcAmmoSupplyActor : AInteractableActorBase
{
	default InteractableComp.Option.InteractionAbilityToGrant = UYcGameplayAbility_QuickBarWeaponAmmoSupply;
	default InteractableComp.Option.Text = FText::FromString("补充弹药");
	default InteractableComp.Option.SubText = FText::FromString("补满快捷栏武器子弹");

	UPROPERTY(EditAnywhere, Category = "Supply")
	bool bDestroyOnSupplySuccess = false;

	UPROPERTY(EditAnywhere, Category = "Supply", meta = (ClampMin = "0"))
	int32 MaxSupplyCount = 0;

	UPROPERTY(EditAnywhere, Category = "Supply", meta = (ClampMin = "0.0"))
	float CooldownSeconds = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Supply", meta = (ClampMin = "0.0"))
	float DestroyDelay = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Supply|UI")
	FText PromptText_Available = FText::FromString("补充弹药");

	UPROPERTY(EditAnywhere, Category = "Supply|UI")
	FText PromptSubText_Available = FText::FromString("补满快捷栏武器子弹");

	UPROPERTY(EditAnywhere, Category = "Supply|UI")
	FText PromptText_Exhausted = FText::FromString("已达到最大补给次数");

	UPROPERTY(EditAnywhere, Category = "Supply|UI")
	FText PromptSubText_Exhausted = FText::FromString("这个弹药箱已经空了");

	UPROPERTY(EditAnywhere, Category = "Supply|UI")
	FString CooldownPromptPrefix = "弹药箱冷却中";

	UPROPERTY(EditAnywhere, Category = "Supply|UI")
	FText PromptSubText_Cooldown = FText::FromString("请稍后再来");

	private int32 CurrentSupplyCount = 0;
	private int32 RemainingCooldownSeconds = 0;
	private EYcAmmoSupplyPromptState PromptState = EYcAmmoSupplyPromptState::Available;

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		Super::BeginPlay();
		ApplyPromptStateLocal(EYcAmmoSupplyPromptState::Available, 0);
	}

	UFUNCTION(BlueprintCallable)
	void HandleAmmoSupplyInteracted(AActor Interactor)
	{
		if (!HasAuthority())
		{
			return;
		}

		if (bDestroyOnSupplySuccess)
		{
			SetLifeSpan(Math::Max(0.0f, DestroyDelay));
			return;
		}

		CurrentSupplyCount++;
		if (HasReachedMaxSupplyCount())
		{
			System::ClearTimer(this, "HandleCooldownTick");
			RemainingCooldownSeconds = 0;
			MulticastApplyPromptState(EYcAmmoSupplyPromptState::Exhausted, 0);
			return;
		}

		if (CooldownSeconds > 0.0f)
		{
			StartCooldown();
			return;
		}

		MulticastApplyPromptState(EYcAmmoSupplyPromptState::Available, 0);
	}

	bool HasReachedMaxSupplyCount() const
	{
		return MaxSupplyCount > 0 && CurrentSupplyCount >= MaxSupplyCount;
	}

	void StartCooldown()
	{
		RemainingCooldownSeconds = Math::Max(1, Math::CeilToInt(CooldownSeconds));
		MulticastApplyPromptState(EYcAmmoSupplyPromptState::Cooldown, RemainingCooldownSeconds);
		System::ClearTimer(this, "HandleCooldownTick");
		System::SetTimer(this, n"HandleCooldownTick", 1.0f, true);
	}

	UFUNCTION()
	void HandleCooldownTick()
	{
		if (!HasAuthority())
		{
			System::ClearTimer(this, "HandleCooldownTick");
			return;
		}

		RemainingCooldownSeconds = Math::Max(0, RemainingCooldownSeconds - 1);
		if (RemainingCooldownSeconds <= 0)
		{
			System::ClearTimer(this, "HandleCooldownTick");
			MulticastApplyPromptState(EYcAmmoSupplyPromptState::Available, 0);
			return;
		}

		MulticastApplyPromptState(EYcAmmoSupplyPromptState::Cooldown, RemainingCooldownSeconds);
	}

	UFUNCTION(NetMulticast)
	private void MulticastApplyPromptState(EYcAmmoSupplyPromptState NewPromptState, int32 CooldownRemainingSeconds)
	{
		ApplyPromptStateLocal(NewPromptState, CooldownRemainingSeconds);
	}

	private void ApplyPromptStateLocal(EYcAmmoSupplyPromptState NewPromptState, int32 CooldownRemainingSeconds)
	{
		PromptState = NewPromptState;
		RemainingCooldownSeconds = CooldownRemainingSeconds;

		FYcInteractionOption& NewOption = InteractableComp.GetInteractionOption();
		NewOption.InteractionAbilityToGrant = UYcGameplayAbility_QuickBarWeaponAmmoSupply;
		NewOption.Text = PromptText_Available;
		NewOption.SubText = PromptSubText_Available;
		NewOption.DisabledDisplayPolicy = EYcInteractionDisabledDisplayPolicy::Show;
		NewOption.DisabledText = FText();
		NewOption.DisabledSubText = FText();
		NewOption.bCanInteract = true;

		if (PromptState == EYcAmmoSupplyPromptState::Cooldown)
		{
			NewOption.bCanInteract = false;
			NewOption.DisabledText = FText::FromString(f"{CooldownPromptPrefix}{RemainingCooldownSeconds}秒");
			NewOption.DisabledSubText = PromptSubText_Cooldown;
		}
		else if (PromptState == EYcAmmoSupplyPromptState::Exhausted)
		{
			NewOption.bCanInteract = false;
			NewOption.DisabledText = PromptText_Exhausted;
			NewOption.DisabledSubText = PromptSubText_Exhausted;
		}

		InteractableComp.SetInteractionOption(NewOption);
	}
}
