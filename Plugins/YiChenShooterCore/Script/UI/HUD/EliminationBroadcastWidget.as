/** 对局击杀消息广播Widget, 用于包含UEliminationBroadcastEntryWidget */
class UEliminationBroadcastWidget : UUserWidget
{
	/** 用于包含ToastListWidget */
	UPROPERTY(BindWidget)
	UOverlay FeedVisibilityOvr;

	UPROPERTY(BindWidget)
	UListView ToastListWidget;

	// @TODO: Splitscreen: Filter to player HUD belongs to

	FGameplayMessageListenerHandle KillFeedListennerHandler;

	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		// 因为会用到widget对象池,所以清理避免残留
		ToastListWidget.ClearListItems();

		UpdateDisplayVisibility();
		ListenEliminationmMessage();
		ToastListWidget.BP_OnEntryInitialized.AddUFunction(this, n"OnEntryInitializedDelegate");
	}

	UFUNCTION(BlueprintOverride)
	void Destruct()
	{
		KillFeedListennerHandler.Unregister();
	}

	private void UpdateDisplayVisibility()
	{
		if (ToastListWidget.GetNumItems() > 0)
		{
			FeedVisibilityOvr.SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			FeedVisibilityOvr.SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	void ListenEliminationmMessage()
	{
		auto GameplayMessage = UGameplayMessageSubsystem::Get();
		KillFeedListennerHandler = GameplayMessage.RegisterListener(
			FGameplayTag::RequestGameplayTag(n"Yc.AddNotification.KillFeed"),
			this,
			n"OnReceivedKillFeed",
			FEliminationFeedMessage(),
			EGameplayMessageMatch::ExactMatch);
	}

	UFUNCTION()
	private void OnReceivedKillFeed(FGameplayTag ActualTag, FEliminationFeedMessage Data)
	{
		if (ToastListWidget.EntryWidgetClass == nullptr)
		{
			Warning("UEliminationBroadcastWidget::OnReceivedKillFeed: 击杀反馈条目UI类无效, 请检查!");
			return;
		}

		auto ElimFeedEntry = NewObject(this, UElimFeedEntry);
		ElimFeedEntry.EliminationFeedMessage = Data;
		ToastListWidget.AddItem(ElimFeedEntry);
		UpdateDisplayVisibility();
	}

	UFUNCTION(NotBlueprintCallable)
	protected void OnEntryInitializedDelegate(UObject Item, UUserWidget Widget)
	{
		auto ElimEntryWidget = Cast<UEliminationBroadcastEntryWidget>(Widget);
		if (ElimEntryWidget == nullptr)
			return;

		// 绑定过期委托以移除击杀信息
		ElimEntryWidget.OnToastExpiredEvent.AddUFunction(this, n"RemoveToast");
	}

	UFUNCTION()
	void RemoveToast(UObject ItemObject)
	{
		ToastListWidget.RemoveItem(ItemObject);
		UpdateDisplayVisibility();
	}
}