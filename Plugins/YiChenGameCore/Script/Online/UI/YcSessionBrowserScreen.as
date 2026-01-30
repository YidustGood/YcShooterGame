// 会话浏览器界面
class UYcSessionBrowserScreen : UCommonActivatableWidget
{
	// 加载进度图标
	UPROPERTY(BindWidget)
	UImage ProgressSpinner;

	// 会话列表视图
	UPROPERTY(BindWidget)
	UListView ListView;

	// 刷新按钮
	UPROPERTY(BindWidget)
	UButton RefreshButton;

	// 会话搜索请求
	UPROPERTY()
	UCommonSession_SearchSessionRequest SearchSessionRequest;

	// 界面激活时
	UFUNCTION(BlueprintOverride)
	void OnActivated()
	{
		StartSearch();
		RefreshButton.OnClicked.AddUFunction(this, n"StartSearch");
	}

	// 界面停用时
	UFUNCTION(BlueprintOverride)
	void OnDeactivated()
	{
		RefreshButton.OnClicked.UnbindObject(this);
	}

	// 开始搜索游戏会话
	UFUNCTION()
	void StartSearch()
	{
		Print("开始搜索游戏会话");
		ListView.ClearListItems();
		ProgressSpinner.SetVisibility(ESlateVisibility::Visible);

		auto SessionSubsystem = UCommonSessionSubsystem::Get();
		SearchSessionRequest = SessionSubsystem.CreateOnlineSearchSessionRequest();

		SearchSessionRequest.K2_OnSearchFinished.AddUFunction(this, n"OnSearchFinished");
		SessionSubsystem.FindSessions(GetOwningPlayer(), SearchSessionRequest);
	}

	// 搜索游戏会话完成回调
	UFUNCTION()
	void OnSearchFinished(bool bSucceeded, FText ErrorMessage)
	{
		ProgressSpinner.SetVisibility(ESlateVisibility::Collapsed);
		if (!bSucceeded)
		{
			PrintError("搜索游戏会话失败:" + ErrorMessage.ToString());
			return;
		}

		TArray<UObject> Items;
		for (int i = 0; i < SearchSessionRequest.Results.Num(); i++)
		{
			Items.Add(SearchSessionRequest.Results[i].Get());
		}
		ListView.SetListItems(Items);

		if (SearchSessionRequest.Results.Num() <= 0)
		{
			Print("搜索游戏会话成功，但没有找到任何会话");
			return;
		}
		Print("搜索游戏会话成功，找到 " + SearchSessionRequest.Results.Num() + " 个会话");
	}
}