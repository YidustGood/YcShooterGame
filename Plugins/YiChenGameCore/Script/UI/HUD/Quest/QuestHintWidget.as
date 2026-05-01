class UYcQuestHintWidget : UUserWidget
{
	/** 绑定到蓝图中的文本控件，用来显示当前任务提示。 */
	UPROPERTY(BindWidget)
	UCommonTextBlock QuestHintText;

	/** 监听“显示/更新提示”的消息 Tag。通常保持默认值即可。 */
	UPROPERTY(EditAnywhere, Category = "任务提示")
	FGameplayTag UiUpdateTag;
	default UiUpdateTag = GameplayTags::Quest_UI_Update;

	/** 监听“隐藏提示”的消息 Tag。通常保持默认值即可。 */
	UPROPERTY(EditAnywhere, Category = "任务提示")
	FGameplayTag UiHideTag;
	default UiHideTag = GameplayTags::Quest_UI_Hide;

	/** 收到空文案时是否自动隐藏 Widget。 */
	UPROPERTY(EditAnywhere, Category = "任务提示")
	bool bHideWhenEmptyText = true;

	private FGameplayMessageListenerHandle UiUpdateHandle;
	private FGameplayMessageListenerHandle UiHideHandle;
	private FTimerHandle AutoHideTimerHandle;

	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		ResolveDefaultTagsIfNeeded();
		SetVisibility(ESlateVisibility::Collapsed);
		SetHintText("");

		auto MessageSubsystem = UGameplayMessageSubsystem::Get();
		UiUpdateHandle = MessageSubsystem.RegisterListener(
			UiUpdateTag,
			this,
			n"OnUiUpdated",
			FQuestHintUpdateMessage(),
			EGameplayMessageMatch::ExactMatch);

		UiHideHandle = MessageSubsystem.RegisterListener(
			UiHideTag,
			this,
			n"OnUiHide",
			FYcGameVerbMessage(),
			EGameplayMessageMatch::ExactMatch);
	}

	UFUNCTION(BlueprintOverride)
	void Destruct()
	{
		UiUpdateHandle.Unregister();
		UiHideHandle.Unregister();
		ClearAutoHideTimer();
	}

	UFUNCTION()
	private void OnUiUpdated(FGameplayTag ActualTag, FQuestHintUpdateMessage Data)
	{
		if (Data.HintText.Len() == 0)
		{
			ClearAutoHideTimer();
			if (bHideWhenEmptyText)
			{
				SetVisibility(ESlateVisibility::Collapsed);
				SetHintText("");
			}
			return;
		}

		SetVisibility(ESlateVisibility::Visible);
		SetHintText(Data.HintText);
		RefreshAutoHide(Data.DisplayDuration);
	}

	UFUNCTION()
	private void OnUiHide(FGameplayTag ActualTag, FYcGameVerbMessage Data)
	{
		ClearAutoHideTimer();
		SetVisibility(ESlateVisibility::Collapsed);
		SetHintText("");
	}

	UFUNCTION()
	private void OnAutoHideTriggered()
	{
		ClearAutoHideTimer();
		SetVisibility(ESlateVisibility::Collapsed);
		SetHintText("");
	}

	/** 根据消息里的持续时间刷新自动隐藏计时器。0 表示不主动隐藏。 */
	private void RefreshAutoHide(float DurationSeconds)
	{
		ClearAutoHideTimer();

		if (DurationSeconds <= 0.0f)
		{
			return;
		}

		AutoHideTimerHandle = System::SetTimer(this, n"OnAutoHideTriggered", Math::Max(0.01f, DurationSeconds), false);
	}

	private void ClearAutoHideTimer()
	{
		System::ClearAndInvalidateTimerHandle(AutoHideTimerHandle);
	}

	private void SetHintText(const FString& InText)
	{
		if (QuestHintText == nullptr)
		{
			return;
		}

		if (InText.Len() == 0)
		{
			QuestHintText.SetText(FText());
			return;
		}

		QuestHintText.SetText(FText::FromString(InText));
	}

	private void ResolveDefaultTagsIfNeeded()
	{
		if (!UiUpdateTag.IsValid())
		{
			UiUpdateTag = FGameplayTag::RequestGameplayTag(n"Quest.UI.Update");
		}

		if (!UiHideTag.IsValid())
		{
			UiHideTag = FGameplayTag::RequestGameplayTag(n"Quest.UI.Hide");
		}
	}
}

USTRUCT()
struct FQuestHintUpdateMessage
{
	/** 实际要显示的提示文案。空字符串可用于清空显示。 */
	UPROPERTY()
	FString HintText;

	/** 文案显示时长（秒）。0 表示不主动隐藏。 */
	UPROPERTY()
	float DisplayDuration = 0.0f;
}
