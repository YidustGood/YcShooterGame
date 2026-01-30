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

	// ==================== 动态十字线准星 ====================

	/** 十字线中心点 */
	UPROPERTY(BindWidget)
	UImage CrosshairCenter;

	/** 十字线上边 */
	UPROPERTY(BindWidget)
	UImage CrosshairTop;

	/** 十字线下边 */
	UPROPERTY(BindWidget)
	UImage CrosshairBottom;

	/** 十字线左边 */
	UPROPERTY(BindWidget)
	UImage CrosshairLeft;

	/** 十字线右边 */
	UPROPERTY(BindWidget)
	UImage CrosshairRight;

	/** 散射值到屏幕距离的缩放系数（可在蓝图中调整） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	float SpreadToScreenScale = 10.0f;

	/** 十字线移动的平滑插值速度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	float CrosshairSmoothSpeed = 15.0f;

	/** 最小十字线间距（像素） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	float MinCrosshairGap = 5.0f;

	/** 最大十字线间距（像素） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crosshair")
	float MaxCrosshairGap = 50.0f;

	/** 当前目标间距（用于平滑插值） */
	private float CurrentTargetGap = 5.0f;

	/** 当前实际间距（平滑后的值） */
	private float CurrentActualGap = 5.0f;

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

		// 初始化十字线位置
		CurrentActualGap = MinCrosshairGap;
		CurrentTargetGap = MinCrosshairGap;
		UpdateCrosshairPositions(CurrentActualGap);
	}

	UFUNCTION(BlueprintOverride)
	void Destruct()
	{
		ADSListennerHandler.Unregister();
		EliminationListennerHandler.Unregister();
		HitCharacterListennerHandler.Unregister();
	}

	UFUNCTION(BlueprintOverride)
	void Tick(FGeometry MyGeometry, float InDeltaTime)
	{
		UpdateDynamicCrosshair(InDeltaTime);
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
			// 瞄准时隐藏十字线
			SetCrosshairVisibility(false);
		}
		else
		{
			PlayAnimationReverse(AimDownSights);
			// 腰射时显示十字线
			SetCrosshairVisibility(true);
		}
	}

	UFUNCTION()
	void OnHitCharacter(FGameplayTag ActualTag, FHitCharacterMessage Data)
	{
		if (GetOwningPlayer() != Data.Hitter)
			return;
		if (Data.HitResult.Actor == nullptr)
			return;

		// 命中队友不做命中反馈
		auto TeamSubsystem = UYcTeamSubsystem::Get();
		if (TeamSubsystem.CompareTeams(GetOwningPlayer(), Data.HitResult.Actor) == EYcTeamComparison::OnSameTeam)
		{
			return;
		}

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
		// 如果死亡的是自己那么就移除掉准星UI
		if (Data.Target == GetOwningPlayer().PlayerState)
		{
			RemoveFromParent();
			return;
		}

		if (Data.Instigator != GetOwningPlayer().PlayerState)
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

	// ==================== 动态十字线实现 ====================

	/** 更新动态十字线 */
	UFUNCTION()
	private void UpdateDynamicCrosshair(float DeltaTime)
	{
		if (WeaponInstance == nullptr)
			return;

		// 转换为 HitScan 武器实例
		auto HitScanWeapon = Cast<UYcHitScanWeaponInstance>(WeaponInstance);
		if (HitScanWeapon == nullptr)
			return;

		// 获取当前散射角度（非瞄准状态）
		float SpreadAngle = HitScanWeapon.GetCurrentSpreadAngle(false);

		// 将散射角度转换为屏幕间距
		// 散射角度越大，十字线间距越大
		CurrentTargetGap = Math::Clamp(
			SpreadAngle * SpreadToScreenScale,
			MinCrosshairGap,
			MaxCrosshairGap);

		// 平滑插值到目标间距
		CurrentActualGap = Math::FInterpTo(
			CurrentActualGap,
			CurrentTargetGap,
			DeltaTime,
			CrosshairSmoothSpeed);

		// 更新十字线位置
		UpdateCrosshairPositions(CurrentActualGap);
	}

	/** 更新十字线各部分的位置 */
	UFUNCTION()
	private void UpdateCrosshairPositions(float Gap)
	{
		if (CrosshairTop == nullptr || CrosshairBottom == nullptr ||
			CrosshairLeft == nullptr || CrosshairRight == nullptr)
			return;

		// 使用 RenderTransform 来移动位置（适用于 Overlay）
		// 上边：向上偏移
		FWidgetTransform TopTransform;
		TopTransform.Translation = FVector2D(0, -Gap);
		CrosshairTop.SetRenderTransform(TopTransform);

		// 下边：向下偏移
		FWidgetTransform BottomTransform;
		BottomTransform.Translation = FVector2D(0, Gap);
		CrosshairBottom.SetRenderTransform(BottomTransform);

		// 左边：向左偏移
		FWidgetTransform LeftTransform;
		LeftTransform.Translation = FVector2D(-Gap, 0);
		CrosshairLeft.SetRenderTransform(LeftTransform);

		// 右边：向右偏移
		FWidgetTransform RightTransform;
		RightTransform.Translation = FVector2D(Gap, 0);
		CrosshairRight.SetRenderTransform(RightTransform);
	}

	/** 设置十字线可见性 */
	UFUNCTION()
	private void SetCrosshairVisibility(bool bVisible)
	{
		ESlateVisibility TargetVisibility = bVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;

		if (CrosshairCenter != nullptr)
			CrosshairCenter.SetVisibility(TargetVisibility);
		if (CrosshairTop != nullptr)
			CrosshairTop.SetVisibility(TargetVisibility);
		if (CrosshairBottom != nullptr)
			CrosshairBottom.SetVisibility(TargetVisibility);
		if (CrosshairLeft != nullptr)
			CrosshairLeft.SetVisibility(TargetVisibility);
		if (CrosshairRight != nullptr)
			CrosshairRight.SetVisibility(TargetVisibility);
	}
}