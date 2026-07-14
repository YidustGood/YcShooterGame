class UYcBottomActionBar : UCommonUserWidget
{
	// 操作按钮栏，ActionBar中可以配置操作按钮类。一个包含当前可在通用用户界面的输入处理程序中执行的操作的列表框。
	UPROPERTY(BindWidget)
	USafeZone ActionBarSZ;

	UPROPERTY(BindWidget)
	UCommonBoundActionBar ActionBar;
}