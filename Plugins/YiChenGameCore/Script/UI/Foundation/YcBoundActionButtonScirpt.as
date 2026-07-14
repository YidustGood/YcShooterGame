class UYcBoundActionButtonScirpt : UYcBoundActionButton
{
	// 这个按钮被设计成“操作栏”按钮。它不可被聚焦，也不应被导航至，
	// 其作用仅仅是向用户展示游戏手柄上可执行的操作，但同时，如果家里有人需要的话，也可以通过点击它来使用这些功能。

	UFUNCTION(BlueprintOverride)
	void PreConstruct(bool IsDesignTime)
	{
		UpdateSyleOnInputMethod(UCommonInputSubsystem::Get(GetOwningPlayer()).GetCurrentInputType());
	}

	UFUNCTION(BlueprintOverride)
	void Construct()
	{
		UCommonInputSubsystem::Get(GetOwningPlayer()).OnInputMethodChanged.AddUFunction(this, n"UpdateSyleOnInputMethod");
	}

	UFUNCTION(BlueprintOverride)
	void Destruct()
	{
		UCommonInputSubsystem::Get(GetOwningPlayer()).OnInputMethodChanged.AddUFunction(this, n"UpdateSyleOnInputMethod");
	}

	UFUNCTION()
	void UpdateSyleOnInputMethod(ECommonInputType bNewInputType)
	{
		// @TODO 为Gamepad设置盒型绘制
	}
}