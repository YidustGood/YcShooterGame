class AYcShooterCharacterBase : AYcCharacter
{
	UPROPERTY(DefaultComponent)
	UYcShooterAnimComponent YcShooterAnimComp;

	UPROPERTY()
	bool bAiming;

	UPROPERTY()
	bool bRunning;

	UPROPERTY()
	bool bLowered;

	// 缓存上一帧的移动状态，避免在Tick中重复添加/移除标签
	private bool bWasMovingForward = true;

	UFUNCTION(BlueprintOverride)
	void BeginPlay()
	{
	}

	UFUNCTION(BlueprintOverride)
	void Tick(float DeltaSeconds)
	{
		if (IsValid(YcAbilitySystemComponent))
		{
			UpdateMovementStateTags();
		}
	}

	// 更新移动状态标签，让能力系统根据标签自动管理能力状态
	void UpdateMovementStateTags()
	{
		float VelocitySize = CharacterMovement.Velocity.Size2D();
		float MoveDir = YcShooterAnimComp.GetMoveDirection();

		// 判断是否在向前移动（前方或斜前方，角度在±70度内）
		bool bIsMovingForward = VelocitySize >= 10.0f && Math::Abs(MoveDir) <= 70.0f;

		// 只在状态发生变化时才添加/移除标签，避免Tick中重复操作导致计数累积
		if (bIsMovingForward != bWasMovingForward)
		{
			if (!bIsMovingForward)
			{
				// 状态切换：从向前移动 -> 非向前移动
				YcAbilitySystemComponent.AddLooseGameplayTag(GameplayTags::Character_State_Movement_NotForward);
			}
			else
			{
				// 状态切换：从非向前移动 -> 向前移动, 移除999是避免标签count残留问题
				YcAbilitySystemComponent.RemoveLooseGameplayTag(GameplayTags::Character_State_Movement_NotForward, 999);
			}
			bWasMovingForward = bIsMovingForward;
		}
	}
}