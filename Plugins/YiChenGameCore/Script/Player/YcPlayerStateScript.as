class AYcPlayerStateScript : AYcPlayerState
{
	//@TODO 这只是个实验方案，后续需要根据实际情况调整去掉
	UPROPERTY(VisibleInstanceOnly)
	private TMap<FGameplayTag, FUIExtensionHandle> ClientUIExtensionHandles;

	UFUNCTION(Client)
	void ClientCreateWidget(FGameplayTag ExtensionPointTag, TSubclassOf<UUserWidget> WidgetClass, int Priority = -1)
	{
		// 这个函数只供服务器为客户端创建UI使用
		if (!HasAuthority())
			return;

		if (WidgetClass == nullptr)
			return;

		auto UIExtensionSubsystem = UUIExtensionSubsystem::Get();
		if (ClientUIExtensionHandles.Contains(ExtensionPointTag))
		{
			FUIExtensionHandle Handle;
			ClientUIExtensionHandles.Find(ExtensionPointTag, Handle);
			UIExtensionSubsystem.UnregisterExtension(Handle);
		}
		ClientUIExtensionHandles.Add(ExtensionPointTag, UIExtensionSubsystem.RegisterExtensionAsWidgetForContext(ExtensionPointTag, WidgetClass, this, Priority));
		Log(f"服务器通过ClientRPC通知客户端创建UI, WidgetClass:{WidgetClass.Get().GetName()}");
	}
}