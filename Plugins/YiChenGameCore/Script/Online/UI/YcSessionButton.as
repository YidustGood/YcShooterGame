// 会话按钮控件
class UYcSessionButton : UCommonButtonBase
{
	// 游戏模式文本
	UPROPERTY(BindWidget)
	UCommonTextBlock GameModeText;

	// 地图名称文本
	UPROPERTY(BindWidget)
	UCommonTextBlock MapNameText;

	// 延迟文本
	UPROPERTY(BindWidget)
	UCommonTextBlock PingText;

	// 当前玩家数文本
	UPROPERTY(BindWidget)
	UCommonTextBlock PlayersText;

	// 最大玩家数文本
	UPROPERTY(BindWidget)
	UCommonTextBlock MaxPlayersText;

	/** 设置会话文本 */
	void SetSessionTexts(FText GameMode, FText MapName, FText Ping, FText Players, FText MaxPlayers)
	{
		GameModeText.SetText(GameMode);
		MapNameText.SetText(MapName);
		PingText.SetText(Ping);
		PlayersText.SetText(Players);
		MaxPlayersText.SetText(MaxPlayers);
	}
}