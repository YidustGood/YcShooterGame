class AYcShooterCharacter : AYcShooterCharacterBase
{
	UPROPERTY(DefaultComponent)
	UYcShooterCombatComponent ShooterCombatComp;

	UPROPERTY(DefaultComponent)
	UYcEquipmentManagerComponent EquipmentManagerComp;

	UPROPERTY(DefaultComponent)
	UYcEquipmentSlotComponent EquipmentSlotComp;

	UPROPERTY(DefaultComponent)
	USpringArmComponent FPSpringArm;

	UPROPERTY(DefaultComponent, Attach = FPSpringArm)
	USkeletalMeshComponent FPCharacter;

	UPROPERTY(DefaultComponent, Attach = FPCharacter, AttachSocket = "head")
	UCameraComponent FPCamera;

	// 是否启用初始库存
	UPROPERTY()
	bool bEnableInitialInventory = true;

	// 初始库存物品列表
	UPROPERTY()
	TArray<FDataRegistryId> InitialInventoryItems;

	// 默认激活的快速栏槽位索引
	UPROPERTY()
	int32 DefaultActiveQuickBarSlotIndex = 0;

	// 蹲伏相机臂延迟时间, 调整这个值实现平滑蹲伏效果
	UPROPERTY()
	float CrouchCameraLagDelayTime = 0.3f;

	// 监听在第一人称手臂模型播放蒙太奇动画的GameplayEvent, 必须用成员变量+UPROPERTY持有, 否则会被Garbage Collector回收
	UPROPERTY(NotVisible)
	UAbilityAsync_WaitGameplayEvent WaitGameplayEvent_PlayFPAnim;

	default FPSpringArm.CameraLagSpeed = 36.0f; // 速度设置快一点避免产生明显镜头延迟和拉扯感
	default FPSpringArm.TargetArmLength = 0.0f;
	default FPSpringArm.bDoCollisionTest = false;
	default FPSpringArm.bUsePawnControlRotation = true;
	default FPCharacter.ComponentTags.Add(n"FPCharacter");
	default FPCamera.ComponentTags.Add(n"FPCamera");
	default FPCamera.bEnableFirstPersonFieldOfView = true;
	default FPCamera.bEnableFirstPersonScale = true;
	default FPCamera.FirstPersonScale = 0.35f;
	default FPCharacter.FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;

	private UAnimInstance FPCharacterAnimInstance;
	private UYcInventoryManagerComponent InventoryManagerComp;
	private UYcQuickBarComponent QuickBarComp;
	private FGameplayMessageListenerHandle QuickBarSlotRemovedListenerHandle;
	private FTimerHandle CrouchCameraLagTimerHandle;
	private bool bInitialInventoryApplied = false;
	private bool bExperienceReady = false;
	private bool bBoundPersistenceEvents = false;

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		Super::BeginPlay();

		QuickBarSlotRemovedListenerHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			GameplayTags::Yc_QuickBar_Message_SlotRemoved,
			this,
			n"OnQuickBarSlotRemoved",
			FYcQuickBarSlotRemovedMessage(),
			EGameplayMessageMatch::ExactMatch);
	}

	UFUNCTION(BlueprintOverride)
	void EndPlay(EEndPlayReason EndPlayReason)
	{
		WaitGameplayEvent_PlayFPAnim.Cancel();
		QuickBarSlotRemovedListenerHandle.Unregister();
	}

	UFUNCTION(BlueprintOverride)
	void OnInitStateGameplayReady(const FActorInitStateChangedParams&in Params)
	{
		Super::OnInitStateGameplayReady(Params);
		FPCharacterAnimInstance = FPCharacter.GetAnimInstance();
		YcShooterAnimComp.AnimationInstance = FPCharacterAnimInstance;
		// @TODO: 这里用延迟不够优雅, 后续可以优化
		DelayForAs(n"ListenFPCharacterPlayMontageEvent", 1);
		RefreshRuntimeInventoryHandles();
		if (InventoryManagerComp == nullptr)
		{
			if (HasAuthority())
			{
				Warning("AYcShooterCharacter::OnInitStateGameplayReady -> InventoryManagerComp is null");
			}
			return;
		}

		TryApplyInitialInventory();
	}

	UFUNCTION(BlueprintOverride)
	void Possessed(AController NewController)
	{
		RefreshRuntimeInventoryHandles(NewController);

		auto WaitForExperienceReady = AsYcGameCoreAsync::WaitForExperienceReady();
		WaitForExperienceReady.OnReady.AddUFunction(this, n"OnExperienceReady");
		WaitForExperienceReady.Activate();

		BindPersistenceEvents(NewController);
		TryApplyInitialInventory();
	}

	void BindPersistenceEvents(AController InController = nullptr)
	{
		if (bBoundPersistenceEvents || InController == nullptr)
		{
			return;
		}

		auto PersistenceComp = GetPersistenceComponent(Controller);
		if (PersistenceComp == nullptr)
		{
			return;
		}

		PersistenceComp.OnDefaultLoadoutRequested.AddUFunction(this, n"OnDefaultLoadoutRequested");
		PersistenceComp.OnPersistenceHydrated.AddUFunction(this, n"OnPersistenceHydrated");
		PersistenceComp.OnPersistenceReadyChanged.AddUFunction(this, n"OnPersistenceReadyChanged");
		bBoundPersistenceEvents = true;
	}

	UYcPlayerPersistenceComponent GetPersistenceComponent(AController InController = nullptr) const
	{
		AController EffectiveController = InController != nullptr ? InController : GetController();
		if (EffectiveController == nullptr)
		{
			return nullptr;
		}

		return Cast<UYcPlayerPersistenceComponent>(EffectiveController.GetComponentByClass(UYcPlayerPersistenceComponent));
	}

	UFUNCTION()
	void OnDefaultLoadoutRequested()
	{
		TryApplyInitialInventory();
	}

	UFUNCTION()
	void OnPersistenceHydrated(FYcProfileIdentity ProfileIdentity)
	{
		TryApplyInitialInventory();
	}

	UFUNCTION()
	void OnPersistenceReadyChanged(bool bIsReady)
	{
		if (bIsReady)
		{
			TryApplyInitialInventory();
		}
	}

	UFUNCTION(BlueprintEvent)
	void OnExperienceReady()
	{
		bExperienceReady = true;
		TryApplyInitialInventory();
		QuickBarComp.SetActiveSlotIndex_WithPrediction(DefaultActiveQuickBarSlotIndex);
	}

	void TryApplyInitialInventory()
	{
		RefreshRuntimeInventoryHandles();

		if (!HasAuthority() || InventoryManagerComp == nullptr || QuickBarComp == nullptr || bInitialInventoryApplied || !bExperienceReady)
		{
			return;
		}

		auto PersistenceComp = GetPersistenceComponent();
		if (PersistenceComp != nullptr)
		{
			auto State = PersistenceComp.GetCurrentRuntimeState();
			if (State != EYcPlayerPersistenceRuntimeState::Idle && State != EYcPlayerPersistenceRuntimeState::Ready)
			{
				return;
			}

			if (State == EYcPlayerPersistenceRuntimeState::Ready && !PersistenceComp.ShouldApplyDefaultLoadout())
			{
				if (!PersistenceComp.HasEmptyHydratedSnapshot())
				{
					return;
				}
			}
		}

		AddInitialInventory();

		if (PersistenceComp != nullptr && PersistenceComp.ShouldApplyDefaultLoadout())
		{
			PersistenceComp.NotifyDefaultLoadoutApplied();
		}
	}

	void RefreshRuntimeInventoryHandles(AController InController = nullptr)
	{
		AController EffectiveController = InController != nullptr ? InController : GetController();
		InventoryManagerComp = EffectiveController != nullptr ? YcInventory::GetInventoryManagerComponent(EffectiveController) : YcInventory::GetInventoryManagerComponent(this);
		QuickBarComp = EffectiveController != nullptr ? UYcQuickBarComponent::FindQuickBarComponent(EffectiveController) : UYcQuickBarComponent::FindQuickBarComponent(this);
	}

	void AddInitialInventory()
	{
		if (!HasAuthority() || InventoryManagerComp == nullptr || QuickBarComp == nullptr || bInitialInventoryApplied)
			return;

		auto ExistingSlots = QuickBarComp.GetSlots();
		for (auto& ItemId : InitialInventoryItems)
		{
			auto ItemInst = InventoryManagerComp.AddItem(ItemId, 1);
			if (ItemInst == nullptr)
				continue;

			auto ItemFragment = ItemInst.FindItemFragment(FInventoryFragment_Equippable);
			if (!ItemFragment.IsValid())
			{
				InventoryManagerComp.RemoveItemInstance(ItemInst);
				continue;
			}

			auto QuickBarSlotFragment = YcEquipment::FindEquipmentFragment(ItemFragment.Get(FInventoryFragment_Equippable).EquipmentDef, FEquipmentFragment_QuickBarSlot);
			if (!QuickBarSlotFragment.IsValid())
			{
				InventoryManagerComp.RemoveItemInstance(ItemInst);
				continue;
			}

			const int32 SlotIndex = QuickBarSlotFragment.Get(FEquipmentFragment_QuickBarSlot).SlotIndex;
			if (SlotIndex < 0)
			{
				InventoryManagerComp.RemoveItemInstance(ItemInst);
				continue;
			}

			if (SlotIndex < ExistingSlots.Num() && ExistingSlots[SlotIndex] != nullptr)
			{
				InventoryManagerComp.RemoveItemInstance(ItemInst);
				continue;
			}

			if (!QuickBarComp.AddItemToSlot(SlotIndex, ItemInst))
			{
				InventoryManagerComp.RemoveItemInstance(ItemInst);
				Warning(f"AddInitialInventory: AddItemToSlot failed. slot={SlotIndex}, item={ItemId}");
				continue;
			}

			ExistingSlots = QuickBarComp.GetSlots();
		}

		bInitialInventoryApplied = true;
		QuickBarComp.SetActiveSlotIndex_WithPrediction(DefaultActiveQuickBarSlotIndex);
	}

	UFUNCTION()
	void SendQuickBarSelectSlotGameplayEvent(int32 SlotIndex)
	{
		FGameplayEventData Payload;
		Payload.EventTag = GameplayTags::InputTag_Ability_QuickBar_SelectSlot;
		Payload.Instigator = this;
		Payload.Target = this;
		Payload.EventMagnitude = SlotIndex;
		AbilitySystem::SendGameplayEventToActor(this, GameplayTags::InputTag_Ability_QuickBar_SelectSlot, Payload);
	}

	UFUNCTION()
	void SwitchSmoothCrouch(bool bCrouch)
	{
		System::ClearAndInvalidateTimerHandle(CrouchCameraLagTimerHandle);
		FPSpringArm.bEnableCameraLag = !CharacterMovement.IsFalling();

		if (bCrouch)
		{
			if (!CanCrouch())
				return;
			Crouch();
			SetDelayDisableCameraLag();
			GetYcAbilitySystemComponent().AddLooseGameplayTag(GameplayTags::Character_State_Movement_Crouching);
		}
		else
		{
			UnCrouch();
			SetDelayDisableCameraLag();
			GetYcAbilitySystemComponent().RemoveLooseGameplayTag(GameplayTags::Character_State_Movement_Crouching);
		}
	}

	void SetDelayDisableCameraLag()
	{
		System::SetTimer(this, n"DisableCameraLag", CrouchCameraLagDelayTime, false);
	}

	UFUNCTION()
	void DisableCameraLag()
	{
		FPSpringArm.bEnableCameraLag = false;
	}

	// 监听在第一人称手臂模型播放蒙太奇动画的游戏事件,
	UFUNCTION()
	void ListenFPCharacterPlayMontageEvent()
	{
		WaitGameplayEvent_PlayFPAnim = AngelscriptAbilityAsync::WaitGameplayEventToActor(this, GameplayTags::GameplayEvent_Character_PlayMontageFP);
		WaitGameplayEvent_PlayFPAnim.EventReceived.AddUFunction(this, n"PlayFPCharacterMontage");
		AsYcGameCoreGameTask::ActivateAsyncAction(WaitGameplayEvent_PlayFPAnim);
	}

	UFUNCTION()
	void PlayFPCharacterMontage(FGameplayEventData Payload)
	{
		UAnimMontage MontageToPlay = Cast<UAnimMontage>(Payload.OptionalObject);
		if (MontageToPlay == nullptr)
		{
			Warning("receive GameplayEvent_Character_PlayMontageFP but MontageToPlay is null.");
			return;
		}

		float PlayRate = Payload.EventMagnitude == 0 ? 1.0f : Payload.EventMagnitude;
		FPCharacter.PlayMontage(MontageToPlay, PlayRate);
	}

	UFUNCTION()
	void OnQuickBarSlotRemoved(FGameplayTag ActualTag, FYcQuickBarSlotRemovedMessage Data)
	{
		if (Data.Owner != GetController())
		{
			return;
		}

		RefreshRuntimeInventoryHandles();
		if (QuickBarComp == nullptr)
		{
			return;
		}

		if (Data.SlotIndex == QuickBarComp.GetActiveSlotIndex())
		{
			QuickBarComp.SetActiveSlotIndex_WithPrediction(DefaultActiveQuickBarSlotIndex);
		}
	}
}
