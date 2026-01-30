/**
 * 基础版血条 Widget
 *
 * 功能：
 * - 显示玩家当前血量（进度条 + 数字）
 * - 自动监听血量变化并更新显示
 * - 支持平滑过渡动画
 *
 * UI 元素：
 * - HealthBar (UImage) - 血条图片（使用材质控制显示比例）
 * - HealthText (UTextBlock) - 血量数字文本
 */
class UYcHealthbarBasic : UUserWidget
{
	// ========================================
	// UI 元素绑定
	// ========================================

	/** 血条图片（需要设置材质） */
	UPROPERTY(BindWidget)
	UImage HealthBar;

	/** 血量数字文本 */
	UPROPERTY(BindWidget)
	UTextBlock HealthText;

	// ========================================
	// 配置属性
	// ========================================

	/** 是否显示百分比（true=100%，false=100） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	bool bShowPercentage = false;

	/** 是否平滑过渡（true=渐变，false=立即更新） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	bool bSmoothTransition = true;

	/** 平滑过渡速度（每秒变化的百分比） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health Bar")
	float TransitionSpeed = 2.f;

	// ========================================
	// 内部变量
	// ========================================

	/** 血条材质实例 */
	private UMaterialInstanceDynamic HealthBarMaterial;

	/** 当前显示的血量百分比（0-1） */
	private float CurrentHealthPercent = 1.0f;

	/** 目标血量百分比（0-1） */
	private float TargetHealthPercent = 1.0f;

	// ========================================
	// 生命周期
	// ========================================

	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		// 创建动态材质实例
		if (HealthBar != nullptr)
		{
			HealthBarMaterial = HealthBar.GetDynamicMaterial();
		}

		// 监听 Pawn 切换事件
		GetOwningPlayer().OnPossessedPawnChanged.AddUFunction(this, n"OnPossessedPawnChanged");

		// 初始化当前 Pawn 的血量监听
		OnPossessedPawnChanged(nullptr, GetOwningPlayer().GetControlledPawn());

		// 初始化显示
		UpdateHealthBar(1.0f);
	}

	UFUNCTION(BlueprintOverride)
	void Tick(FGeometry MyGeometry, float InDeltaTime)
	{
		// 如果启用平滑过渡，逐渐更新血条
		if (bSmoothTransition && CurrentHealthPercent != TargetHealthPercent)
		{
			// 计算本帧的变化量
			float Delta = TransitionSpeed * InDeltaTime;

			// 向目标值靠近
			if (CurrentHealthPercent < TargetHealthPercent)
			{
				CurrentHealthPercent = Math::Min(CurrentHealthPercent + Delta, TargetHealthPercent);
			}
			else
			{
				CurrentHealthPercent = Math::Max(CurrentHealthPercent - Delta, TargetHealthPercent);
			}

			// 更新材质参数
			UpdateMaterialParameter(CurrentHealthPercent);
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
		// 取消旧 Pawn 的血量监听
		auto OldHealthComp = UYcHealthComponent::FindHealthComponent(OldPawn);
		if (OldHealthComp != nullptr)
		{
			OldHealthComp.OnHealthChanged.Unbind(this, n"OnHealthChanged");
		}

		// 绑定新 Pawn 的血量监听
		auto NewHealthComp = UYcHealthComponent::FindHealthComponent(NewPawn);
		if (NewHealthComp != nullptr)
		{
			NewHealthComp.OnHealthChanged.AddUFunction(this, n"OnHealthChanged");

			// 立即更新为新 Pawn 的血量
			float HealthPercent = NewHealthComp.GetHealthNormalized();
			UpdateHealthBar(HealthPercent);
		}
	}

	// ========================================
	// 血量变化处理
	// ========================================

	/**
	 * 血量变化时的回调
	 * 更新血条和文本显示
	 */
	UFUNCTION()
	private void OnHealthChanged(UYcHealthComponent HealthComponent, float32 OldValue, float32 NewValue, AActor Instigator)
	{
		// 计算新的血量百分比
		float HealthPercent = NewValue / 100.0f;
		UpdateHealthBar(HealthPercent);
	}

	/**
	 * 更新血条显示
	 * @param HealthPercent 血量百分比（0-1）
	 */
	private void UpdateHealthBar(float HealthPercent)
	{
		// 限制范围在 0-1 之间
		float ClampHealthPercent = Math::Clamp(HealthPercent, 0.0f, 1.0f);

		// 设置目标值
		TargetHealthPercent = ClampHealthPercent;

		// 如果不使用平滑过渡，立即更新
		if (!bSmoothTransition)
		{
			CurrentHealthPercent = TargetHealthPercent;
			UpdateMaterialParameter(CurrentHealthPercent);
		}

		// 更新文本显示
		UpdateHealthText(HealthPercent);
	}

	/**
	 * 更新材质参数
	 * @param HealthPercent 血量百分比（0-1）
	 */
	private void UpdateMaterialParameter(float HealthPercent)
	{
		if (HealthBarMaterial != nullptr)
		{
			// 设置材质参数 "HealthPercent"
			// 这个参数名需要与材质中的参数名一致
			HealthBarMaterial.SetScalarParameterValue(n"HealthPercent", HealthPercent);
		}
	}

	/**
	 * 更新血量文本
	 * @param HealthPercent 血量百分比（0-1）
	 */
	private void UpdateHealthText(float HealthPercent)
	{
		if (HealthText == nullptr)
		{
			return;
		}

		if (bShowPercentage)
		{
			// 显示百分比格式：100%
			int Percent = Math::RoundToInt(HealthPercent * 100);
			HealthText.SetText(FText::FromString(f"{Percent}%"));
		}
		else
		{
			// 显示整数格式：100
			int Health = Math::RoundToInt(HealthPercent * 100);
			HealthText.SetText(FText::FromString(f"{Health}"));
		}
	}

	// ========================================
	// 公共接口
	// ========================================

	/**
	 * 手动设置血量百分比
	 * @param HealthPercent 血量百分比（0-1）
	 */
	UFUNCTION(BlueprintCallable, Category = "Health Bar")
	void SetHealthPercent(float HealthPercent)
	{
		UpdateHealthBar(HealthPercent);
	}

	/**
	 * 获取当前显示的血量百分比
	 * @return 血量百分比（0-1）
	 */
	UFUNCTION(BlueprintCallable, Category = "Health Bar")
	float GetCurrentHealthPercent() const
	{
		return CurrentHealthPercent;
	}
}
