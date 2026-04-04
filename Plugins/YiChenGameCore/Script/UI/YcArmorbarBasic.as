/**
 * 基础版护甲量条 Widget
 *
 * 功能：
 * - 显示玩家当前护甲量（进度条 + 数字）
 * - 自动监听护甲量变化并更新显示
 * - 支持平滑过渡动画
 *
 * UI 元素：
 * - ArmorBar (UImage) - 护甲量条图片（使用材质控制显示比例）
 * - ArmorText (UTextBlock) - 护甲量数字文本
 */
class UYcArmorbarBasic : UUserWidget
{
	// ========================================
	// UI 元素绑定
	// ========================================

	/** 护甲量条图片（需要设置材质） */
	UPROPERTY(BindWidget)
	UImage ArmorBar;

	/** 护甲量数字文本 */
	UPROPERTY(BindWidget)
	UTextBlock ArmorText;

	// ========================================
	// 配置属性
	// ========================================

	/** 是否显示百分比（true=100%，false=100） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor Bar")
	bool bShowPercentage = false;

	/** 是否平滑过渡（true=渐变，false=立即更新） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor Bar")
	bool bSmoothTransition = true;

	/** 平滑过渡速度（每秒变化的百分比） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor Bar")
	float TransitionSpeed = 2.f;

	// ========================================
	// 内部变量
	// ========================================

	/** 护甲量条材质实例 */
	private UMaterialInstanceDynamic ArmorBarMaterial;

	/** 当前显示的护甲量百分比（0-1） */
	private float CurrentArmorPercent = 1.0f;

	/** 目标护甲量百分比（0-1） */
	private float TargetArmorPercent = 1.0f;

	// ========================================
	// 生命周期
	// ========================================

	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		// 创建动态材质实例
		if (ArmorBar != nullptr)
		{
			ArmorBarMaterial = ArmorBar.GetDynamicMaterial();
		}

		// 监听 Pawn 切换事件
		// GetOwningPlayer().OnPossessedPawnChanged.AddUFunction(this, n"OnPossessedPawnChanged");

		// 初始化当前 Pawn 的护甲量监听
		// OnPossessedPawnChanged(nullptr, GetOwningPlayer().GetControlledPawn());

		// 初始化显示
		UpdateArmorBar(0.0f);

		// 监听护甲量变化消息
		ListenArmorMessage();
	}

	void ListenArmorMessage()
	{
		UGameplayMessageSubsystem::Get().RegisterListener(
			GameplayTags::Message_Armor_Changed,
			this,
			n"OnArmorChanged",
			FYcArmorMessage(),
			EGameplayMessageMatch::ExactMatch);

		UGameplayMessageSubsystem::Get().RegisterListener(
			GameplayTags::Message_Armor_Equipped,
			this,
			n"OnArmorEquipped",
			FYcArmorMessage(),
			EGameplayMessageMatch::ExactMatch);

		UGameplayMessageSubsystem::Get().RegisterListener(
			GameplayTags::Message_Armor_Unequipped,
			this,
			n"OnArmorUnequipped",
			FYcArmorMessage(),
			EGameplayMessageMatch::ExactMatch);
	}

	UFUNCTION()
	private void OnArmorEquipped(FGameplayTag ActualTag, FYcArmorMessage Data)
	{
		// 只关心自己的护甲量变化
		if (GetOwningPlayer().GetControlledPawn() != Data.TargetActor)
			return;

		// 计算新的护甲量百分比
		float ArmorPercent = Data.NewValue / Data.MaxArmor;
		UpdateArmorBar(ArmorPercent);
	}

	UFUNCTION()
	private void OnArmorUnequipped(FGameplayTag ActualTag, FYcArmorMessage Data)
	{
		// 只关心自己的护甲量变化
		if (GetOwningPlayer().GetControlledPawn() != Data.TargetActor)
			return;

		UpdateArmorBar(0);
	}

	UFUNCTION(BlueprintOverride)
	void Tick(FGeometry MyGeometry, float InDeltaTime)
	{
		// 如果启用平滑过渡，逐渐更新血条
		if (bSmoothTransition && CurrentArmorPercent != TargetArmorPercent)
		{
			// 计算本帧的变化量
			float Delta = TransitionSpeed * InDeltaTime;

			// 向目标值靠近
			if (CurrentArmorPercent < TargetArmorPercent)
			{
				CurrentArmorPercent = Math::Min(CurrentArmorPercent + Delta, TargetArmorPercent);
			}
			else
			{
				CurrentArmorPercent = Math::Max(CurrentArmorPercent - Delta, TargetArmorPercent);
			}

			// 更新材质参数
			UpdateMaterialParameter(CurrentArmorPercent);
		}
	}

	// ========================================
	// Pawn 切换处理
	// ========================================

	/**
	 * Pawn 切换时的回调
	 * 取消旧 Pawn 的监听，绑定新 Pawn 的监听
	 */
	UFUNCTION()
	private void OnPossessedPawnChanged(APawn OldPawn, APawn NewPawn)
	{
		//@TODO
	}

	// ========================================
	// 护甲量变化处理
	// ========================================

	/**
	 * 护甲量变化时的回调
	 * 更新护甲量条和文本显示
	 */
	UFUNCTION()
	private void OnArmorChanged(FGameplayTag ActualTag, FYcArmorMessage Data)
	{
		// 只关心自己的护甲量变化
		if (GetOwningPlayer().GetControlledPawn() != Data.TargetActor)
			return;

		// @TODO 需要以护甲最大耐久为基准计算护甲量百分比,当前实现是基于护甲量的
		// 计算新的护甲量百分比
		float ArmorPercent = Data.NewValue / 100.0f;
		UpdateArmorBar(ArmorPercent);
	}

	/**
	 * 更新护甲量条显示
	 * @param ArmorPercent 护甲量百分比（0-1）
	 */
	private void UpdateArmorBar(float ArmorPercent)
	{
		// 限制范围在 0-1 之间
		float ClampArmorPercent = Math::Clamp(ArmorPercent, 0.0f, 1.0f);

		// 设置目标值
		TargetArmorPercent = ClampArmorPercent;

		// 如果不使用平滑过渡，立即更新
		if (!bSmoothTransition)
		{
			CurrentArmorPercent = TargetArmorPercent;
			UpdateMaterialParameter(CurrentArmorPercent);
		}

		// 更新文本显示
		UpdateArmorText(ArmorPercent);
	}

	/**
	 * 更新材质参数
	 * @param ArmorPercent 护甲量百分比（0-1）
	 */
	private void UpdateMaterialParameter(float ArmorPercent)
	{
		if (ArmorBarMaterial != nullptr)
		{
			// 设置材质参数 "HealthPercent"
			// 这个参数名需要与材质中的参数名一致
			ArmorBarMaterial.SetScalarParameterValue(n"HealthPercent", ArmorPercent);
		}
	}

	/**
	 * 更新护甲量文本
	 * @param ArmorPercent 护甲量百分比（0-1）
	 */
	private void UpdateArmorText(float ArmorPercent)
	{
		if (ArmorText == nullptr)
		{
			return;
		}

		if (bShowPercentage)
		{
			// 显示百分比格式：100%
			int Percent = Math::RoundToInt(ArmorPercent * 100);
			ArmorText.SetText(FText::FromString(f"{Percent}%"));
		}
		else
		{
			// 显示整数格式：100
			int ArmorInt = Math::RoundToInt(ArmorPercent * 100);
			ArmorText.SetText(FText::FromString(f"{ArmorInt}"));
		}
	}

	// ========================================
	// 公共接口
	// ========================================

	/**
	 * 手动设置护甲量百分比
	 * @param ArmorPercent 护甲量百分比（0-1）
	 */
	UFUNCTION(BlueprintCallable, Category = "Armor Bar")
	void SetArmorPercent(float ArmorPercent)
	{
		UpdateArmorBar(ArmorPercent);
	}

	/**
	 * 获取当前显示的护甲量百分比
	 * @return 护甲量百分比（0-1）
	 */
	UFUNCTION(BlueprintCallable, Category = "Armor Bar")
	float GetCurrentArmorPercent() const
	{
		return CurrentArmorPercent;
	}
}
