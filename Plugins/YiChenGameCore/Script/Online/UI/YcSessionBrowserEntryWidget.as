// 会话浏览器条目控件
class UYcSessionBrowserEntryWidget : UCommonTabListWidgetBase
{
	// 会话按钮
	UPROPERTY(BindWidget)
	UYcSessionButton SessionButton;

	// 会话搜索结果
	UCommonSession_SearchResult SessionEntry;

	// 加入进度提示控件
	UCommonActivatableWidget JoiningProgressWidget;

	// 会话所使用的ExperienceId, 可以利用这个创建主资产ID提前加载资产
	FString ExperienceId;

	// 初始化
	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		SessionButton.OnButtonBaseClicked.AddUFunction(this, n"OnSessionButtonClicked");
	}

	// 设置会话条目数据
	// UMG蓝图子类实现UserObjectListEntry后在OnListItemObjectSet实现中转发调用到这个函数上处理会话数据
	UFUNCTION()
	void OnSessionEntrySet(UObject Item)
	{
		SessionEntry = Cast<UCommonSession_SearchResult>(Item);

		bool bFoundValue;
		SessionEntry.GetStringSetting(n"GAMEMODE", ExperienceId, bFoundValue);
		int32 PingMs = SessionEntry.GetPingInMs();
		int32 Players = SessionEntry.GetNumOpenPublicConnections();
		int32 MaxPlayers = SessionEntry.GetMaxPublicConnections();

		SessionButton.SetSessionTexts(FText::FromString("战斗模式"), FText::FromString("体验地图"), FText::FromString(f"{PingMs}"), FText::FromString(f"{Players}"), FText::FromString(f"{MaxPlayers}"));
	}

	// 会话按钮点击回调
	UFUNCTION()
	void OnSessionButtonClicked(UCommonButtonBase Button)
	{
		Print("加入会话");
		JoinSession();
	}

	// 加入会话
	UFUNCTION()
	void JoinSession()
	{
		auto Push = NewObject(this, UAsyncAction_PushContentToLayerForPlayer);
		Push.AfterPush.AddUFunction(this, n"OnPushJoiningProgressWidget");
		Push.Activate();

		// 会话寻找
		bool bFoundValue;
		SessionEntry.GetStringSetting(n"GAMEMODE", ExperienceId, bFoundValue);

		auto SessionSubsystem = UCommonSessionSubsystem::Get();

		SessionSubsystem.K2_OnJoinSessionCompleteEvent.AddUFunction(this, n"OnJoinSessionComplete");
		auto AA = GetOwningPlayer();
		SessionSubsystem.JoinSession(GetOwningPlayer(), SessionEntry);
	}

	// 加入进度控件推送完成回调
	UFUNCTION()
	void OnPushJoiningProgressWidget(UCommonActivatableWidget UserWidget)
	{
		JoiningProgressWidget = UserWidget;
	}

	// 加入会话完成回调
	UFUNCTION()
	void OnJoinSessionComplete(const FOnlineResultInformation&in Result)
	{
		auto SessionSubsystem = UCommonSessionSubsystem::Get();
		SessionSubsystem.K2_OnJoinSessionCompleteEvent.Unbind(this, n"OnJoinSessionComplete");

		// 移除加载UI
		if (JoiningProgressWidget != nullptr)
		{
			JoiningProgressWidget.RemoveFromParent();
		}

		if (Result.bWasSuccessful)
		{
			Log("JoinSessionComplete: 成功，正在连接到服务器...");
		}
		else
		{
			Log("JoinSessionComplete: 失败 - " + Result.ErrorText);
		}
	}
}