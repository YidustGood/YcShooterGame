struct FUIVisibilityMessage
{
	bool bVisibility;
	APlayerController Controller;
}

USTRUCT()
struct FHitCharacterMessage
{
	UPROPERTY()
	AController Hitter;
	UPROPERTY()
	FHitResult HitResult;
}

/** 武器准星Widget */
class UWeaponReticleWidget : UCommonUserWidget
{
	// 准星Anim
	UPROPERTY(NotEditable, Transient, meta = (BindWidgetAnim), Category = "Animation")
	UWidgetAnimation AimDownSights;

	/** 命中玩家标记图片 */
	UPROPERTY(BindWidget)
	UImage HitMarker;

	UPROPERTY(NotEditable, Transient, meta = (BindWidgetAnim), Category = "Animation")
	UWidgetAnimation HitMarkerFade;

	/** 消灭玩家标记图片 */
	UPROPERTY(BindWidget)
	UImage EliminationMarker;

	UPROPERTY(NotEditable, Transient, meta = (BindWidgetAnim), Category = "Animation")
	UWidgetAnimation EliminationMarkerFade;

	/** 消灭目标音效 */
	UPROPERTY()
	USoundBase EliminationSound;

	/** 命中音效 */
	UPROPERTY()
	USoundBase HitMarkerSound;

	// 运行时数据
	UYcWeaponInstance WeaponInstance;
	UYcInventoryItemInstance InventoryInstance;
	FGameplayMessageListenerHandle ADSListennerHandler;
	FGameplayMessageListenerHandle EliminationListennerHandler;
	FGameplayMessageListenerHandle HitCharacterListennerHandler;

	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		auto GameplayMessage = UGameplayMessageSubsystem::Get();
		// 监听ADS状态改变消息, 以同步更新准星UI
		ADSListennerHandler = GameplayMessage.RegisterListener(
			GameplayTags::Message_Weapon_ADS_StateChanged,
			this,
			n"OnADSStateChanged",
			FYcWeaponADSMessage(),
			EGameplayMessageMatch::ExactMatch);

		// 监听消灭玩家消息
		EliminationListennerHandler = GameplayMessage.RegisterListener(
			FGameplayTag::RequestGameplayTag(n"Yc.Elimination.Message"),
			this,
			n"OnEliminate",
			FYcGameVerbMessage(),
			EGameplayMessageMatch::ExactMatch);

		// 监听命中玩家消息
		HitCharacterListennerHandler = GameplayMessage.RegisterListener(
			GameplayTags::GameplayCue_Character_DamageTaken,
			this,
			n"OnHitCharacter",
			FHitCharacterMessage(),
			EGameplayMessageMatch::ExactMatch);

		// 默认设置两个Marker图标渲染不透明度为0隐藏掉
		HitMarker.RenderOpacity = 0;
		EliminationMarker.RenderOpacity = 0;
	}

	UFUNCTION(BlueprintOverride)
	void Destruct()
	{
		ADSListennerHandler.Unregister();
		EliminationListennerHandler.Unregister();
		HitCharacterListennerHandler.Unregister();
	}

	// 处理瞄准时准心动画
	UFUNCTION()
	private void OnADSStateChanged(FGameplayTag ActualTag, FYcWeaponADSMessage Data)
	{
		// 只响应自己的
		if (GetOwningPlayer() != Data.PlayerController)
			return;

		// ADS状态禁用准心
		if (IsAnimationPlayingForward(AimDownSights))
		{
			StopAnimation(AimDownSights);
		}

		if (Data.bIsAiming)
		{
			PlayAnimationForward(AimDownSights);
		}
		else
		{
			PlayAnimationReverse(AimDownSights);
		}
	}

	UFUNCTION()
	void OnHitCharacter(FGameplayTag ActualTag, FHitCharacterMessage Data)
	{
		if (GetOwningPlayer() != Data.Hitter)
			return;
		if (Data.HitResult.Actor == nullptr)
			return;
		auto HealthComponent = Data.HitResult.Actor.GetComponentByClass(UYcHealthComponent);
		if (HealthComponent == nullptr)
			return;

		PlayAnimation(HitMarkerFade);
		if (HitMarkerSound != nullptr)
		{
			Gameplay::PlaySound2D(HitMarkerSound);
		}
	}

	// 当我们消灭了一个玩家时在准心上做出特殊反馈动画
	UFUNCTION()
	private void OnEliminate(FGameplayTag ActualTag, FYcGameVerbMessage Data)
	{
		if (Data.Instigator != GetOwningPlayer())
			return;

		PlayAnimation(EliminationMarkerFade);

		auto PS = Cast<AYcPlayerState>(Data.Instigator);
		bool bFlag1 = GetOwningPlayer() == PS.GetYcPlayerController();
		bool bFlag2 = Data.Target != PS;
		if (bFlag1 && bFlag2)
		{
			if (EliminationSound != nullptr)
			{
				Gameplay::PlaySound2D(EliminationSound);
			}
			if (HitMarkerSound != nullptr)
			{
				Gameplay::PlaySound2D(HitMarkerSound, 1.5);
			}
		}
	}

	// 创建时在创建的地方调用初始化我们
	UFUNCTION(BlueprintCallable)
	void InitializeFromWeapon(UYcWeaponInstance InWeapon)
	{
		WeaponInstance = InWeapon;
		InventoryInstance = nullptr;
		if (WeaponInstance != nullptr)
		{
			InventoryInstance = Cast<UYcInventoryItemInstance>(WeaponInstance.Instigator);
		}
		OnWeaponInitialized();
	}

	UFUNCTION(BlueprintEvent)
	void OnWeaponInitialized()
	{
	}
}