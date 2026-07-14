/**
 * 呼吸回血被动能力
 *
 * 设计说明：
 * - 作为 OnSpawn + ServerOnly 的被动 GA 常驻运行
 * - 使用现有 Yc.Damage.Message 作为“最近受到伤害”的统一输入
 * - 通过 ASC LooseGameplayTag 维护 Enabled / Blocked / Active 三种状态
 * - 实际治疗仍调用 UYcHealthComponent::ApplyHealing，复用既有血量变更链路
 *
 * 当前判定口径：
 * - 依赖现有伤害消息，因此当前等价于“发生了有效伤害结算后开始 5 秒脱战等待”
 * - 若后续需要把护甲吸收、格挡等场景也视作打断回血，可再切到更底层的受击事件
 */
class UYcGameplayAbility_AutoRegen : UYcGameplayAbility
{
	default ActivationPolicy = EYcAbilityActivationPolicy::OnSpawn;
	default NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	default AbilityTags.AddTag(GameplayTags::Ability_Passive_AutoRegen);
	default ActivationOwnedTags.AddTag(GameplayTags::State_Regen_Auto_Enabled);

	/** 脱离受击后，延迟多久开始进入回血状态 */
	UPROPERTY(EditDefaultsOnly, Category = "Auto Regen", meta = (ClampMin = "0.0"))
	float RegenDelaySeconds = 5.0f;

	/** 进入回血状态后，每隔多久恢复一次生命 */
	UPROPERTY(EditDefaultsOnly, Category = "Auto Regen", meta = (ClampMin = "0.01"))
	float RegenIntervalSeconds = 2.0f;

	/** 每次回血恢复的生命值 */
	UPROPERTY(EditDefaultsOnly, Category = "Auto Regen", meta = (ClampMin = "0.0"))
	float HealPerTick = 10.0f;

	/** 自动回血最多恢复到最大生命值的百分比 */
	UPROPERTY(EditDefaultsOnly, Category = "Auto Regen", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float MaxRecoverPercent = 0.7f;

	/** 能力激活时，若角色已低于回血封顶线，是否安排一次初始脱战检查 */
	UPROPERTY(EditDefaultsOnly, Category = "Auto Regen")
	bool bScheduleInitialRegenCheck = true;

	private FGameplayMessageListenerHandle DamageMessageListenerHandle;
	private FTimerHandle RegenDelayTimerHandle;
	private FTimerHandle RegenTickTimerHandle;
	private bool bDamageMessageListenerRegistered = false;

	UFUNCTION(BlueprintOverride)
	void ActivateAbility()
	{
		if (!HasAuthority())
		{
			EndAbility();
			return;
		}

		RegisterDamageMessageListener();

		if (bScheduleInitialRegenCheck && ShouldScheduleInitialRegenCheck())
		{
			ScheduleRegenDelay(false);
		}
	}

	UFUNCTION(BlueprintOverride)
	void OnEndAbility(bool bWasCancelled)
	{
		UnregisterDamageMessageListener();
		ClearAllRegenTimers();
		ClearRegenStateTags();
	}

	/** 收到有效伤害消息后，重置脱战回血的等待窗口 */
	UFUNCTION()
	private void OnDamageMessage(FGameplayTag ActualTag, FYcGameVerbMessage Data)
	{
		if (!IsRelevantDamageMessage(Data))
		{
			return;
		}

		ScheduleRegenDelay(true);
	}

	/** 5 秒脱战等待结束后，尝试正式进入回血状态 */
	UFUNCTION()
	private void TryStartRegen()
	{
		System::ClearAndInvalidateTimerHandle(RegenDelayTimerHandle);
		SetRegenBlockedTag(false);

		if (!CanStartOrContinueRegen())
		{
			StopActiveRegen();
			return;
		}

		SetRegenActiveTag(true);

		const float SafeInterval = Math::Max(0.01f, RegenIntervalSeconds);
		System::ClearAndInvalidateTimerHandle(RegenTickTimerHandle);
		RegenTickTimerHandle = System::SetTimer(this, n"HandleRegenTick", SafeInterval, true);
	}

	/** 回血状态中的单次治疗 Tick */
	UFUNCTION()
	private void HandleRegenTick()
	{
		if (!CanStartOrContinueRegen())
		{
			StopActiveRegen();
			return;
		}

		auto HealthComp = ResolveHealthComponent();
		if (HealthComp == nullptr)
		{
			StopActiveRegen();
			return;
		}

		const float CurrentHealth = HealthComp.GetHealth();
		const float RegenCapHealth = GetRegenCapHealth(HealthComp);
		const float MissingToCap = RegenCapHealth - CurrentHealth;
		const float HealAmount = Math::Min(HealPerTick, MissingToCap);

		if (HealAmount <= 0.0f)
		{
			StopActiveRegen();
			return;
		}

		HealthComp.ApplyHealing(HealAmount);

		if (!CanStartOrContinueRegen())
		{
			StopActiveRegen();
		}
	}

	/** 安排一次新的脱战回血启动检查，并按需标记 Blocked 状态 */
	private void ScheduleRegenDelay(bool bMarkAsBlocked)
	{
		ClearAllRegenTimers();
		SetRegenActiveTag(false);

		if (bMarkAsBlocked)
		{
			SetRegenBlockedTag(true);
		}

		const float SafeDelay = Math::Max(0.0f, RegenDelaySeconds);
		if (SafeDelay <= 0.0f)
		{
			TryStartRegen();
			return;
		}

		RegenDelayTimerHandle = System::SetTimer(this, n"TryStartRegen", SafeDelay, false);
	}

	/** 停止当前回血状态，不修改 Blocked 标记 */
	private void StopActiveRegen()
	{
		System::ClearAndInvalidateTimerHandle(RegenTickTimerHandle);
		SetRegenActiveTag(false);
	}

	/** 清理自动回血相关的所有计时器 */
	private void ClearAllRegenTimers()
	{
		System::ClearAndInvalidateTimerHandle(RegenDelayTimerHandle);
		System::ClearAndInvalidateTimerHandle(RegenTickTimerHandle);
	}

	/** 清理能力内部维护的状态标签 */
	private void ClearRegenStateTags()
	{
		SetRegenBlockedTag(false);
		SetRegenActiveTag(false);
	}

	/** 当前是否满足进入或继续自动回血的条件 */
	private bool CanStartOrContinueRegen() const
	{
		auto ASC = GetYcAbilitySystemComponentFromActorInfo();
		auto HealthComp = ResolveHealthComponent();
		if (ASC == nullptr || HealthComp == nullptr)
		{
			return false;
		}

		if (HealPerTick <= 0.0f)
		{
			return false;
		}

		if (HealthComp.IsDeadOrDying() ||
			ASC.HasMatchingGameplayTag(GameplayTags::Ability_NoAutoRegen) ||
			ASC.HasMatchingGameplayTag(GameplayTags::State_Regen_Auto_Blocked))
		{
			return false;
		}

		const float RegenCapHealth = GetRegenCapHealth(HealthComp);
		return HealthComp.GetHealth() < RegenCapHealth;
	}

	/** 能力激活时，是否值得安排一次初始回血检查 */
	private bool ShouldScheduleInitialRegenCheck() const
	{
		auto HealthComp = ResolveHealthComponent();
		if (HealthComp == nullptr)
		{
			return false;
		}

		if (HealthComp.IsDeadOrDying())
		{
			return false;
		}

		return HealthComp.GetHealth() < GetRegenCapHealth(HealthComp);
	}

	/** 当前消息是否属于本能力对应的角色 */
	private bool IsRelevantDamageMessage(const FYcGameVerbMessage&in Data) const
	{
		if (Data.Magnitude <= 0.0)
		{
			return false;
		}

		UObject MessageTarget = Data.Target;
		if (MessageTarget == nullptr)
		{
			return false;
		}

		AActor AvatarActor = GetAvatarActorFromActorInfo();
		AActor OwningActor = GetOwningActorFromActorInfo();
		return MessageTarget == AvatarActor || MessageTarget == OwningActor;
	}

	/** 按最大生命值与百分比计算呼吸回血封顶线 */
	private float GetRegenCapHealth(UYcHealthComponent HealthComp) const
	{
		if (HealthComp == nullptr)
		{
			return 0.0f;
		}

		const float ClampedPercent = Math::Clamp(MaxRecoverPercent, 0.0f, 1.0f);
		return HealthComp.GetMaxHealth() * ClampedPercent;
	}

	/** 优先从 Avatar 上解析 HealthComponent，必要时回退到 Owner */
	private UYcHealthComponent ResolveHealthComponent() const
	{
		AActor AvatarActor = GetAvatarActorFromActorInfo();
		auto HealthComp = UYcHealthComponent::FindHealthComponent(AvatarActor);
		if (HealthComp != nullptr)
		{
			return HealthComp;
		}

		AActor OwningActor = GetOwningActorFromActorInfo();
		return UYcHealthComponent::FindHealthComponent(OwningActor);
	}

	/** 注册伤害消息监听，作为脱战回血的打断来源 */
	private void RegisterDamageMessageListener()
	{
		if (bDamageMessageListenerRegistered)
		{
			return;
		}

		DamageMessageListenerHandle = UGameplayMessageSubsystem::Get().RegisterListener(
			GameplayTags::Yc_Damage_Message,
			this,
			n"OnDamageMessage",
			FYcGameVerbMessage(),
			EGameplayMessageMatch::ExactMatch);

		bDamageMessageListenerRegistered = true;
	}

	/** 注销消息监听，避免能力结束后残留回调 */
	private void UnregisterDamageMessageListener()
	{
		if (!bDamageMessageListenerRegistered)
		{
			return;
		}

		DamageMessageListenerHandle.Unregister();
		bDamageMessageListenerRegistered = false;
	}

	/** 统一设置 Blocked 状态 */
	private void SetRegenBlockedTag(bool bEnabled)
	{
		SetLooseStateTag(GameplayTags::State_Regen_Auto_Blocked, bEnabled);
	}

	/** 统一设置 Active 状态 */
	private void SetRegenActiveTag(bool bEnabled)
	{
		SetLooseStateTag(GameplayTags::State_Regen_Auto_Active, bEnabled);
	}

	/** 以幂等方式维护 ASC 上的 LooseGameplayTag */
	private void SetLooseStateTag(FGameplayTag Tag, bool bEnabled)
	{
		auto ASC = GetYcAbilitySystemComponentFromActorInfo();
		if (ASC == nullptr || !Tag.IsValid())
		{
			return;
		}

		ASC.RemoveLooseGameplayTag(Tag, 9999);

		if (bEnabled)
		{
			ASC.AddLooseGameplayTag(Tag, 1);
		}
	}
}
