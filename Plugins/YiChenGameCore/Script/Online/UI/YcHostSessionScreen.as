// 主机会话创建界面
/**
 * 注意：
# 版本较新的CommonSession插件要DefaultEngine.ini中配置关闭验证否则无法加入会话
[/Script/OnlineSubsystemUtils.PartyBeaconHost]
bIsValidationStrRequired = false
*/
class UYcHostSessionScreen : UCommonActivatableWidget
{
	// Bot数量输入框
	UPROPERTY(BindWidget)
	UEditableTextBox EditableTextBox_NumBots;

	// 创建会话按钮
	UPROPERTY(BindWidget)
	UButton Button_HostSession;

	// 在线模式
	UPROPERTY()
	ECommonSessionOnlineMode OnlineMode = ECommonSessionOnlineMode::Online;

	// 地图ID
	UPROPERTY()
	FPrimaryAssetId MapID;

	// 最大玩家数
	UPROPERTY()
	int32 MaxPlayerCount = 10;

	// Bot数量
	UPROPERTY()
	int32 NumBots = 0;

	// 是否启用Bot
	UPROPERTY()
	bool bBotsEnabled = true;

	/** 作为游戏的 URL 选项传递的额外参数 */
	UPROPERTY(Category = Experience)
	TMap<FString, FString> ExtraArgs;

	// 初始化
	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		Button_HostSession.OnClicked.AddUFunction(this, n"HostSession");
		EditableTextBox_NumBots.OnTextChanged.AddUFunction(this, n"OnNumBotsTextChanged");
	}

	// Bot数量文本变化回调
	UFUNCTION()
	void OnNumBotsTextChanged(const FText&in Text)
	{
		if (Text.IsEmpty())
		{
			NumBots = 0;
			return;
		}

		NumBots = String::Conv_StringToInt(Text.ToString());
	}

	/** 创建主机会话 */
	UFUNCTION()
	void HostSession()
	{
		if (OnlineMode == ECommonSessionOnlineMode::Online)
		{
			if (!UCommonUserSubsystem::Get().TryToLoginForOnlinePlay(0))
			{
				PrintError("登录失败!");
				return;
			}
		}

		auto HostingRequest = CreateHostingRequest();

		auto SessionSubsystem = UCommonSessionSubsystem::Get();
		SessionSubsystem.K2_OnCreateSessionCompleteEvent.AddUFunction(this, n"CreateSessionCompleted");

		SessionSubsystem.HostSession(GetOwningPlayer(), HostingRequest);
	}

	// 创建主机会话请求
	// @TODO 后续专门创建一个面向用户的模式资产来定义这些数据, 就不需要硬编码了
	UCommonSession_HostSessionRequest CreateHostingRequest()
	{
		auto SessionSubsystem = UCommonSessionSubsystem::Get();
		auto Result = SessionSubsystem.CreateOnlineHostSessionRequest();
		const FString ExperienceName = "EXP_DeathMatch";
		const FString UserFacingExperienceName = "射击游戏";
		Result.OnlineMode = OnlineMode;
		// 务必设置为true, UE5.6.1如果为false, 会导致会话创建失败, 报错: LogOnline: Warning: STEAM: Unexpected GSPolicyResponse callback
		Result.bUseLobbies = true;
		Result.MapID = MapID;
		Result.ModeNameForAdvertisement = UserFacingExperienceName;
		Result.ExtraArgs = ExtraArgs;
		Result.ExtraArgs.Add("Experience", ExperienceName);
		Result.ExtraArgs.Add("NumBots", bBotsEnabled ? f"{NumBots}" : "0");
		Result.MaxPlayerCount = MaxPlayerCount;

		return Result;
	}

	// 会话创建完成回调
	UFUNCTION()
	void CreateSessionCompleted(const FOnlineResultInformation&in Result)
	{
		auto SessionSubsystem = UCommonSessionSubsystem::Get();
		SessionSubsystem.K2_OnCreateSessionCompleteEvent.Unbind(this, n"CreateSessionCompleted");
		if (Result.bWasSuccessful)
		{
			Print("会话创建成功!");
		}
		else
		{
			PrintError("会话创建失败,错误信息: " + Result.ErrorText);
		}
	}
}