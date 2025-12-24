// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "System/YcGameSystemStatics.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameSystemStatics)

void UYcGameSystemStatics::PlayNextGame(const UObject* WorldContextObject, bool bSeamlessTravel)
{
	// 从上下文对象获取当前有效的世界
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (World == nullptr)
	{
		return;
	}

	const FWorldContext& WorldContext = GEngine->GetWorldContextFromWorldChecked(World);
	FURL LastURL = WorldContext.LastURL;

#if WITH_EDITOR
	// 在 PIE 模式下进行关卡切换时，需要移除关卡名中的 PIE 前缀
	LastURL.Map = UWorld::StripPIEPrefixFromPackageName(LastURL.Map, WorldContext.World()->StreamingLevelsPrefix);
#endif

	if (bSeamlessTravel)
	{
		// 添加无缝旅行选项以保持客户端连接；如果引擎关闭了无缝旅行会自动回退到普通 ServerTravel
		LastURL.AddOption(TEXT("SeamlessTravel"));
	}

	FString URL = LastURL.ToString();
	// 若不移除 Host/Port 信息，ServerTravel 会失败，这里构造相对 URL
	URL.RemoveFromStart(LastURL.GetHostPortString());
	
	const bool bAbsolute = false; // we want to use TRAVEL_Relative
	const bool bShouldSkipGameNotify = false;
	World->ServerTravel(URL, bAbsolute, bShouldSkipGameNotify);
}