// /**
//  * 自动重生技能
//  * 一般作为被动技能赋予玩家, 因为ASC挂载在PlayerState上的所以Pawn死亡也不影响这个技能的生命周期
//  */
// class UYcGameplayAbility_AutoRespawn : UYcGameplayAbility
// {
// 	// 重生延迟时间
// 	UPROPERTY(Category = "Respawn")
// 	float RespawnDelayDuration = 3.0f;

// 	// 重生UI Widget类
// 	UPROPERTY(Category = "Respawn")
// 	TSubclassOf<UUserWidget> RespawnWidget;

// 	UPROPERTY(Category = "Respawn")
// 	FGameplayTag RespawnWidgetTag = FGameplayTag::RequestGameplayTag(n"HUD.Slot.Reticle");

// 	// UI扩展句柄
// 	private FUIExtensionHandle Extension;

// 	// 是否正在监听重生消息
// 	private bool IsListeningForReset = false;

// 	// 需要重生的控制器
// 	private AController ControllerToReset;

// 	// 是否应该完成重生
// 	private bool ShouldFinishRestart = false;

// 	// 死亡监听器委托句柄
// 	private FGameplayMessageListenerHandle ResetListenerHandle;

// 	private FGameplayMessageListenerHandle DeathListenerHandle;

// 	// Avatar结束播放委托句柄
// 	private FGameplayMessageListenerHandle AvatarEndPlayHandle;

// 	AActor GetOwningPlayerState()
// 	{
// 		if (GetAbilitySystemComponentFromActorInfo() != nullptr)
// 		{
// 			return GetAbilitySystemComponentFromActorInfo().GetOwner();
// 		}
// 		return nullptr;
// 	}

// 	/**
// 	 * 技能被添加到ASC时调用
// 	 */
// 	UFUNCTION(BlueprintOverride)
// 	void OnAbilityAdded()
// 	{
// 		// 获取UI扩展子系统
// 		auto UIExtensionSubsystem = UUIExtensionSubsystem::Get();

// 		if (UIExtensionSubsystem != nullptr && RespawnWidget.IsValid())
// 		{
// 			// 注册UI扩展 - 在设定的HUD Slot位置上创建重生倒计时Widget, Widget那边自行处理隐藏与显示(根据倒计时时间)
// 			Extension = UIExtensionSubsystem.RegisterExtensionAsWidgetForContext(
// 				RespawnWidgetTag,
// 				RespawnWidget.Get(),
// 				GetOwningPlayerState(),
// 				-1 // Priority
// 			);
// 		}
// 	}

// 	/**
// 	 * 技能从ASC移除时调用
// 	 */
// 	UFUNCTION(BlueprintOverride)
// 	void OnAbilityRemoved()
// 	{
// 		ClearDeathListener();
// 	}

// 	/**
// 	 * 技能激活时调用
// 	 */
// 	UFUNCTION(BlueprintOverride)
// 	void OnActivateAbility()
// 	{
// 		// 检查是否已经在监听重生消息
// 		if (!IsListeningForReset)
// 		{
// 			// 开始监听重生消息
// 			ListenForResetMessage();
// 			IsListeningForReset = true;
// 		}

// 		// 检查Avatar是否已经死亡或正在死亡
// 		if (IsAvatarDeadOrDying())
// 		{
// 			// 如果已经死亡，直接触发死亡开始事件
// 			AActor Avatar = GetAvatarActorFromActorInfo();
// 			OnDeathStarted(Avatar);
// 		}
// 		else
// 		{
// 			// 否则绑定死亡监听器
// 			BindDeathListener();
// 		}
// 	}

// 	/**
// 	 * Pawn Avatar设置时调用
// 	 */
// 	UFUNCTION(BlueprintOverride)
// 	void OnPawnAvatarSet()
// 	{
// 		K2_ActivateAbility();
// 	}

// 	/**
// 	 * 技能结束时调用
// 	 */
// 	UFUNCTION(BlueprintOverride)
// 	void K2_OnEndAbility(bool bWasCancelled)
// 	{
// 		// 注销UI扩展
// 		if (Extension.IsValid())
// 		{
// 			UUIExtensionHandleFunctions::Unregister(Extension);
// 		}

// 		// 清理死亡监听器
// 		ClearDeathListener();

// 		// 调用父类结束
// 		Super::K2_OnEndAbility(bWasCancelled);
// 	}

// 	/**
// 	 * 监听重生消息
// 	 */
// 	private void ListenForResetMessage()
// 	{
// 		// 监听GameplayEvent.Reset消息
// 		ResetListenerHandle = UGameplayMessageSubsystem::Get().RegisterListener(
// 			GameplayTags::GameplayEvent_Reset,
// 			this,
// 			n"OnResetMessageReceived",
// 			FYcPlayerResetMessage(),
// 			EGameplayMessageMatch::ExactMatch);
// 	}

// 	/**
// 	 * 收到重生消息时的回调
// 	 */
// 	UFUNCTION()
// 	private void OnResetMessageReceived(FGameplayTag Tag, const FYcPlayerResetMessage& Payload)
// 	{
// 		// 检查消息是否是发给当前Avatar的
// 		AActor OwningActor = GetOwningActorFromActorInfo();
// 		if (Payload.OwnerPlayerState == OwningActor)
// 		{
// 			// 如果有权限，执行重生
// 			if (HasAuthority())
// 			{
// 				// 保存控制器引用
// 				ControllerToReset = GetControllerFromActorInfo();

// 				// 清理死亡监听器
// 				ClearDeathListener();

// 				// 标记应该完成重生
// 				ShouldFinishRestart = false;

// 				// 执行重生流程
// 				PerformRespawn();
// 			}
// 		}
// 	}

// 	/**
// 	 * 绑定Avatar结束播放事件
// 	 */
// 	private void BindAvatarEndPlayListener()
// 	{
// 		AActor Avatar = GetAvatarActorFromActorInfo();
// 		if (Avatar != nullptr)
// 		{
// 			AvatarEndPlayHandle = Avatar.OnEndPlay.AddUFunction(this, n"OnAvatarEndPlay");
// 		}
// 	}

// 	/**
// 	 * Avatar结束播放时的回调
// 	 */
// 	UFUNCTION()
// 	private void OnAvatarEndPlay(AActor Actor, EEndPlayReason EndPlayReason)
// 	{
// 		// Avatar被销毁时，如果有权限则执行重生
// 		if (K2_HasAuthority())
// 		{
// 			ControllerToReset = GetControllerFromActorInfo();
// 			ClearDeathListener();
// 			ShouldFinishRestart = false;
// 			PerformRespawn();
// 		}
// 	}

// 	/**
// 	 * 绑定死亡监听器
// 	 */
// 	private void BindDeathListener()
// 	{
// 		ClearDeathListener();

// 		AActor Avatar = GetAvatarActorFromActorInfo();
// 		if (Avatar != nullptr)
// 		{
// 			// 假设Avatar有健康组件
// 			UYcHealthComponent HealthComp = Cast<UYcHealthComponent>(
// 				Avatar.GetComponentByClass(UYcHealthComponent::StaticClass()));

// 			if (HealthComp != nullptr)
// 			{
// 				DeathListenerHandle = HealthComp.OnDeathStarted.AddUFunction(this, n"OnDeathStarted");
// 			}
// 		}

// 		// 同时绑定Avatar结束播放监听
// 		BindAvatarEndPlayListener();
// 	}

// 	/**
// 	 * 清理死亡监听器
// 	 */
// 	private void ClearDeathListener()
// 	{
// 		AActor Avatar = GetAvatarActorFromActorInfo();
// 		if (Avatar != nullptr)
// 		{
// 			UYcHealthComponent HealthComp = Cast<UYcHealthComponent>(
// 				Avatar.GetComponentByClass(UYcHealthComponent::StaticClass()));

// 			if (HealthComp != nullptr && DeathListenerHandle.IsValid())
// 			{
// 				HealthComp.OnDeathStarted.Remove(DeathListenerHandle);
// 				DeathListenerHandle.Reset();
// 			}

// 			// 清理Avatar结束播放监听
// 			if (AvatarEndPlayHandle.IsValid())
// 			{
// 				Avatar.OnEndPlay.Remove(AvatarEndPlayHandle);
// 				AvatarEndPlayHandle.Reset();
// 			}
// 		}
// 	}

// 	/**
// 	 * 死亡开始时的回调
// 	 */
// 	UFUNCTION()
// 	private void OnDeathStarted(AActor DyingActor)
// 	{
// 		// 清理死亡监听器
// 		ClearDeathListener();

// 		// 如果有权限，准备重生
// 		if (K2_HasAuthority())
// 		{
// 			// 保存控制器引用
// 			ControllerToReset = GetControllerFromActorInfo();

// 			if (ControllerToReset != nullptr)
// 			{
// 				// 标记应该完成重生
// 				ShouldFinishRestart = true;

// 				// 广播重生时间消息
// 				BroadcastRespawnDuration();

// 				// 延迟后执行重生
// 				System::SetTimer(this, n"OnRespawnDelayComplete", RespawnDelayDuration, false);
// 			}
// 		}
// 	}

// 	/**
// 	 * 广播重生持续时间消息
// 	 */
// 	private void BroadcastRespawnDuration()
// 	{
// 		UGameplayMessageSubsystem MessageSubsystem = UGameplayMessageSubsystem::Get(this);
// 		if (MessageSubsystem != nullptr)
// 		{
// 			FYcInteractionDurationMessage Message;
// 			Message.Instigator = GetOwningPlayerState();
// 			Message.Duration = RespawnDelayDuration;

// 			MessageSubsystem.K2_BroadcastMessage(
// 				FGameplayTag::RequestGameplayTag(n"Character.Respawn.DurationChanged"),
// 				Message);
// 		}
// 	}

// 	/**
// 	 * 重生延迟完成后的回调
// 	 */
// 	UFUNCTION()
// 	private void OnRespawnDelayComplete()
// 	{
// 		// 检查是否应该完成重生
// 		if (ShouldFinishRestart)
// 		{
// 			ShouldFinishRestart = false;
// 			PerformRespawn();
// 		}
// 	}

// 	/**
// 	 * 执行重生流程
// 	 */
// 	private void PerformRespawn()
// 	{
// 		if (ControllerToReset != nullptr)
// 		{
// 			// 获取玩家状态的ASC
// 			APlayerState PlayerState = Cast<APlayerState>(ControllerToReset.PlayerState);
// 			if (PlayerState != nullptr)
// 			{
// 				UAbilitySystemComponent ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PlayerState);

// 				// 结束所有死亡相关技能
// 				EndDeathAbilities(ASC);
// 			}

// 			// 短暂延迟后请求重生（避免死亡取消太接近重生导致Pawn初始化问题）
// 			System::SetTimer(this, n"RequestRespawn", 0.1f, false);
// 		}
// 	}

// 	/**
// 	 * 请求重生
// 	 */
// 	UFUNCTION()
// 	private void RequestRespawn()
// 	{
// 		// 获取游戏模式
// 		AYcGameMode GameMode = Cast<AYcGameMode>(Gameplay::GetGameMode(this));
// 		if (GameMode != nullptr && ControllerToReset != nullptr)
// 		{
// 			// 请求玩家在下一帧重生
// 			GameMode.RequestPlayerRestartNextFrame(ControllerToReset, true);

// 			// 获取游戏状态
// 			AYcGameState GameState = Cast<AYcGameState>(Gameplay::GetGameState(this));
// 			if (GameState != nullptr)
// 			{
// 				// 广播重生完成消息到客户端
// 				FYcGameVerbMessage Message;
// 				Message.Verb = FGameplayTag::RequestGameplayTag(n"Ability.Respawn.Completed.Message");
// 				Message.Instigator = GetOwningPlayerState();

// 				GameState.MulticastMessageToClients(Message);

// 				// 如果可以执行装饰性事件，也在本地广播
// 				if (CanExecuteCosmeticEvents())
// 				{
// 					BroadcastRespawnCompletedMessage(Message);
// 				}
// 			}
// 		}
// 	}

// 	/**
// 	 * 广播重生完成消息
// 	 */
// 	private void BroadcastRespawnCompletedMessage(const FYcGameVerbMessage& Message)
// 	{
// 		UGameplayMessageSubsystem MessageSubsystem = UGameplayMessageSubsystem::Get(this);
// 		if (MessageSubsystem != nullptr)
// 		{
// 			MessageSubsystem.K2_BroadcastMessage(
// 				FGameplayTag::RequestGameplayTag(n"Ability.Respawn.Completed.Message"),
// 				Message);
// 		}
// 	}

// 	/**
// 	 * 结束死亡相关技能
// 	 */
// 	private void EndDeathAbilities(UAbilitySystemComponent ASC)
// 	{
// 		if (ASC != nullptr)
// 		{
// 			// 取消所有带有Death标签的技能
// 			FGameplayTagContainer DeathTags;
// 			DeathTags.AddTag(FGameplayTag::RequestGameplayTag(n"Ability.Type.Death"));

// 			ASC.CancelAbilities(DeathTags);
// 		}
// 	}

// 	/**
// 	 * 检查Avatar是否死亡或正在死亡
// 	 */
// 	private bool IsAvatarDeadOrDying()
// 	{
// 		AActor Avatar = GetAvatarActorFromActorInfo();
// 		if (Avatar != nullptr)
// 		{
// 			UYcHealthComponent HealthComp = Cast<UYcHealthComponent>(
// 				Avatar.GetComponentByClass(UYcHealthComponent::StaticClass()));

// 			if (HealthComp != nullptr)
// 			{
// 				return HealthComp.IsDeadOrDying();
// 			}
// 		}

// 		return false;
// 	}

// 	/**
// 	 * 检查是否可以执行装饰性事件
// 	 */
// 	private bool CanExecuteCosmeticEvents()
// 	{
// 		// 在客户端或单机模式下可以执行装饰性事件
// 		return !K2_HasAuthority() || GetWorld().IsNetMode(NM_Standalone);
// 	}
// }