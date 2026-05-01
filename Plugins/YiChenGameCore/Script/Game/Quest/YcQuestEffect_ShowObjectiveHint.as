class UYcQuestEffect_ShowObjectiveHint : UYcQuestEffect
{
	/** UI 更新消息 Tag。通常保持默认的 Quest.UI.Update。 */
	UPROPERTY(EditDefaultsOnly, Category = "任务提示|消息")
	FGameplayTag UiUpdateTag;
	default UiUpdateTag = GameplayTags::Quest_UI_Update;

	/** UI 隐藏消息 Tag。通常保持默认的 Quest.UI.Hide。 */
	UPROPERTY(EditDefaultsOnly, Category = "任务提示|消息")
	FGameplayTag UiHideTag;
	default UiHideTag = GameplayTags::Quest_UI_Hide;

	/** 激活步骤时显示的文案。留空时使用 Objective 自己的 DisplayText。 */
	UPROPERTY(EditDefaultsOnly, Category = "任务提示|激活")
	FString OverrideHintText;

	/** 激活步骤时的显示时长。0 表示不主动隐藏。 */
	UPROPERTY(EditDefaultsOnly, Category = "任务提示|激活")
	float ActivatedDisplayDuration = 0.0f;

	/** 步骤完成时是否立即隐藏当前提示。 */
	UPROPERTY(EditDefaultsOnly, Category = "任务提示|完成")
	bool bHideOnCompleted = false;

	/** 步骤完成时是否额外广播一条“完成提示”。 */
	UPROPERTY(EditDefaultsOnly, Category = "任务提示|完成")
	bool bBroadcastOnCompleted = false;

	/** 完成提示文案。留空时复用激活文案或 Objective 的 DisplayText。 */
	UPROPERTY(EditDefaultsOnly, Category = "任务提示|完成")
	FString CompletedHintText;

	/** 完成提示的显示时长。0 表示不主动隐藏。 */
	UPROPERTY(EditDefaultsOnly, Category = "任务提示|完成")
	float CompletedDisplayDuration = 0.0f;

	/** 步骤失败时是否立即隐藏当前提示。 */
	UPROPERTY(EditDefaultsOnly, Category = "任务提示|失败")
	bool bHideOnFailed = false;

	/** 步骤失败时是否额外广播一条“失败提示”。 */
	UPROPERTY(EditDefaultsOnly, Category = "任务提示|失败")
	bool bBroadcastOnFailed = false;

	/** 失败提示文案。留空时复用激活文案或 Objective 的 DisplayText。 */
	UPROPERTY(EditDefaultsOnly, Category = "任务提示|失败")
	FString FailedHintText;

	/** 失败提示的显示时长。0 表示不主动隐藏。 */
	UPROPERTY(EditDefaultsOnly, Category = "任务提示|失败")
	float FailedDisplayDuration = 0.0f;

	UFUNCTION(BlueprintOverride)
	void ExecuteEffect(UYcQuestSubsystem QuestSubsystem, const FYcQuestInstanceKey&in InstanceKey, EYcQuestEffectTrigger Trigger, FName ObjectiveId, const FYcQuestPublicProgress&in Progress)
	{
		if (Trigger == EYcQuestEffectTrigger::ObjectiveActivated)
		{
			BroadcastUpdate(ResolveHintText(Progress), ActivatedDisplayDuration);
			return;
		}

		if (Trigger == EYcQuestEffectTrigger::ObjectiveCompleted)
		{
			if (bBroadcastOnCompleted)
			{
				BroadcastUpdate(CompletedHintText.IsEmpty() ? ResolveHintText(Progress) : CompletedHintText, CompletedDisplayDuration);
			}

			if (bHideOnCompleted)
			{
				BroadcastHide();
			}
			return;
		}

		if (Trigger == EYcQuestEffectTrigger::ObjectiveFailed)
		{
			if (bBroadcastOnFailed)
			{
				BroadcastUpdate(FailedHintText.IsEmpty() ? ResolveHintText(Progress) : FailedHintText, FailedDisplayDuration);
			}

			if (bHideOnFailed)
			{
				BroadcastHide();
			}
		}
	}

	private void BroadcastUpdate(const FString& InText, float DisplayDuration) const
	{
		if (!UiUpdateTag.IsValid())
		{
			return;
		}

		FQuestHintUpdateMessage Message;
		Message.HintText = InText;
		Message.DisplayDuration = Math::Max(0.0f, DisplayDuration);
		UGameplayMessageSubsystem::Get().BroadcastMessage(UiUpdateTag, Message);
	}

	private void BroadcastHide() const
	{
		if (!UiHideTag.IsValid())
		{
			return;
		}

		FYcGameVerbMessage Message;
		Message.Verb = UiHideTag;
		Message.Magnitude = 0.0;
		UGameplayMessageSubsystem::Get().BroadcastMessage(UiHideTag, Message);
	}

	private FString ResolveHintText(const FYcQuestPublicProgress&in Progress) const
	{
		return OverrideHintText.IsEmpty() ? Progress.DisplayText : OverrideHintText;
	}
}
