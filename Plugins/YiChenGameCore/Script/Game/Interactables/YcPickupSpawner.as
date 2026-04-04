/**
 * 可拾取Actor生成器
 * 在场景中生成配置的PickupActor，支持定时刷新和生成次数限制
 */
class AYcPickupSpawner : AActor
{
	/** 根场景组件 */
	UPROPERTY(DefaultComponent, RootComponent)
	USceneComponent SceneRoot;

	/** 底座静态网格体组件 */
	UPROPERTY(DefaultComponent, Attach = SceneRoot)
	UStaticMeshComponent MeshComponent;

	/** 要生成的Actor类 */
	UPROPERTY(Category = "Pickup Spawner")
	TSubclassOf<AYcPickupActor> PickupActorClass;

	/** 生成位置偏移（相对于生成器位置） */
	UPROPERTY(Category = "Pickup Spawner")
	FVector SpawnOffset = FVector(0.f, 0.f, 100.f);

	/** 是否为生成的Actor启用悬浮旋转效果 */
	UPROPERTY(Category = "Pickup Spawner|Animation")
	bool bEnableFloatingRotation = true;

	/** 悬浮速度（每秒完成的周期数，1.0 表示 1 秒完成一个上下循环） */
	UPROPERTY(Category = "Pickup Spawner|Animation", Meta = (EditCondition = "bEnableFloatingRotation"))
	float FloatingSpeed = 0.75f;

	/** 悬浮幅度（上下移动的距离） */
	UPROPERTY(Category = "Pickup Spawner|Animation", Meta = (EditCondition = "bEnableFloatingRotation"))
	float FloatingAmplitude = 5.f;

	/** 旋转速度（度/秒） */
	UPROPERTY(Category = "Pickup Spawner|Animation", Meta = (EditCondition = "bEnableFloatingRotation"))
	float RotationSpeed = 50.f;

	/** 是否启用自动重新生成 */
	UPROPERTY(Category = "Pickup Spawner")
	bool bEnableRespawn = true;

	/** 重新生成延迟时间（秒） */
	UPROPERTY(Category = "Pickup Spawner", Meta = (EditCondition = "bEnableRespawn"))
	float RespawnDelay = 5.0f;

	/** 最大生成次数（0表示无限制） */
	UPROPERTY(Category = "Pickup Spawner")
	int32 MaxSpawnCount = 0;

	/** 当前已生成次数 */
	UPROPERTY(VisibleInstanceOnly)
	int32 CurrentSpawnCount = 0;

	/** 当前生成的Actor实例 */
	AYcPickupActor SpawnedActor;

	/** 生成Actor的初始位置（用于悬浮动画） */
	FVector SpawnedActorInitialLocation;

	/** 悬浮动画的时间累积 */
	float FloatingTime = 0.f;

	/** 重新生成的定时器句柄 */
	FTimerHandle RespawnTimerHandle;

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
		// 只在服务器端生成
		if (HasAuthority())
		{
			SpawnPickupActor();
		}
	}

	UFUNCTION(BlueprintOverride)
	void Tick(float DeltaSeconds)
	{
		// 如果启用了悬浮旋转且有生成的Actor，则更新动画
		if (bEnableFloatingRotation && SpawnedActor != nullptr)
		{
			UpdateFloatingRotation(DeltaSeconds);
		}
	}

	UFUNCTION(BlueprintOverride)
	void EndPlay(EEndPlayReason Reason)
	{
		// 清理定时器
		System::ClearAndInvalidateTimerHandle(RespawnTimerHandle);

		// 清理生成的Actor
		if (SpawnedActor != nullptr)
		{
			SpawnedActor.DestroyActor();
			SpawnedActor = nullptr;
		}
	}

	/** 生成PickupActor */
	UFUNCTION()
	void SpawnPickupActor()
	{
		// 检查是否达到最大生成次数
		if (MaxSpawnCount > 0 && CurrentSpawnCount >= MaxSpawnCount)
		{
			return;
		}

		// 检查类是否有效
		if (PickupActorClass == nullptr)
		{
			Print(f"YcPickupSpawner: PickupActorClass is not set!", 5.0f, FLinearColor::Red);
			return;
		}

		// 计算生成位置（应用偏移）
		FVector SpawnLocation = ActorLocation + ActorTransform.TransformVector(SpawnOffset);
		FRotator SpawnRotation = ActorRotation;

		// 生成Actor
		SpawnedActor = Cast<AYcPickupActor>(SpawnActor(PickupActorClass, SpawnLocation, SpawnRotation));

		if (SpawnedActor != nullptr)
		{
			CurrentSpawnCount++;

			// 记录初始位置用于悬浮动画
			SpawnedActorInitialLocation = SpawnLocation;
			FloatingTime = 0.f;

			// 绑定拾取事件
			if (SpawnedActor.YcPickupableComp != nullptr)
			{
				SpawnedActor.YcPickupableComp.OnPickedUp.AddUFunction(this, n"OnPickupActorPickedUp");
			}
		}
	}

	/** 当生成的Actor被拾取时调用 */
	UFUNCTION()
	void OnPickupActorPickedUp(AActor InInstigator)
	{
		// 清空引用
		if (SpawnedActor != nullptr)
		{
			// 解绑事件
			if (SpawnedActor.YcPickupableComp != nullptr)
			{
				SpawnedActor.YcPickupableComp.OnPickedUp.Unbind(this, n"OnPickupActorPickedUp");
			}
			SpawnedActor = nullptr;
		}

		// 如果启用了重新生成，则启动定时器
		if (bEnableRespawn)
		{
			// 检查是否还能继续生成
			if (MaxSpawnCount == 0 || CurrentSpawnCount < MaxSpawnCount)
			{
				RespawnTimerHandle = System::SetTimer(this, n"SpawnPickupActor", RespawnDelay, false);
			}
		}
	}

	/** 手动触发重新生成 */
	UFUNCTION(BlueprintCallable, Category = "Pickup Spawner")
	void ForceRespawn()
	{
		if (!HasAuthority())
		{
			return;
		}

		// 清除现有定时器
		System::ClearAndInvalidateTimerHandle(RespawnTimerHandle);

		// 销毁现有Actor
		if (SpawnedActor != nullptr)
		{
			SpawnedActor.DestroyActor();
			SpawnedActor = nullptr;
		}

		// 立即生成新的
		SpawnPickupActor();
	}

	/** 重置生成计数器 */
	UFUNCTION(BlueprintCallable, Category = "Pickup Spawner")
	void ResetSpawnCount()
	{
		CurrentSpawnCount = 0;
	}

	/** 获取剩余可生成次数 */
	UFUNCTION(BlueprintPure, Category = "Pickup Spawner")
	int32 GetRemainingSpawnCount() const
	{
		if (MaxSpawnCount == 0)
		{
			return -1; // 无限制
		}
		return Math::Max(0, MaxSpawnCount - CurrentSpawnCount);
	}

	/** 更新悬浮旋转动画 */
	void UpdateFloatingRotation(float DeltaSeconds)
	{
		if (SpawnedActor == nullptr)
		{
			return;
		}

		// 累积时间
		FloatingTime += DeltaSeconds;

		// 计算悬浮偏移（使用正弦波）
		// FloatingSpeed 表示每秒完成的周期数，需要乘以 2π 转换为角频率
		float AngularFrequency = FloatingSpeed * 2.0f * PI;
		float FloatingOffset = Math::Sin(FloatingTime * AngularFrequency) * FloatingAmplitude;
		FVector NewLocation = SpawnedActorInitialLocation + FVector(0.f, 0.f, FloatingOffset);

		// 计算旋转
		FRotator CurrentRotation = SpawnedActor.ActorRotation;
		FRotator NewRotation = CurrentRotation + FRotator(0.f, RotationSpeed * DeltaSeconds, 0.f);

		// 应用变换
		SpawnedActor.SetActorLocationAndRotation(NewLocation, NewRotation);
	}
}
