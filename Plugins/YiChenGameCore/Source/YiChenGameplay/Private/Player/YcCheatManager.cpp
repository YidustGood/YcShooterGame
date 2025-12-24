// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Player/YcCheatManager.h"

#include "Development/YcGameDeveloperSettings.h"
#include "Engine/Console.h"
#include "Player/YcPlayerController.h"
#include "System/YcGameSystemStatics.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcCheatManager)

DEFINE_LOG_CATEGORY(LogYcGameCheat);

namespace YcGameCheat
{
	static bool bStartInGodMode = false;
	static FAutoConsoleVariableRef CVarStartInGodMode(
		TEXT("YcGameCheat.StartInGodMode"),
		bStartInGodMode,
		// 如果为 true，则在 BeginPlay 时自动应用 God 作弊
		TEXT("If true then the God cheat will be applied on begin play"),
		ECVF_Cheat);
}

UYcCheatManager::UYcCheatManager()
{
}

void UYcCheatManager::InitCheatManager()
{
	Super::InitCheatManager();

#if WITH_EDITOR
	if (GIsEditor)
	{
		APlayerController* PC = GetOuterAPlayerController();
		// 在编辑器下，根据 DeveloperSettings 中的配置自动执行指定时机的作弊命令
		for (const FYcCheatToRun& CheatRow : GetDefault<UYcGameDeveloperSettings>()->CheatsToRun)
		{
			if (CheatRow.Phase == ECheatExecutionTime::OnCheatManagerCreated)
			{
				PC->ConsoleCommand(CheatRow.Cheat, /*bWriteToLog=*/ true);
			}
		}
	}
#endif

	if (YcGameCheat::bStartInGodMode)
	{
		God();
	}
}

void UYcCheatManager::CheatOutputText(const FString& TextToOutput)
{
#if USING_CHEAT_MANAGER
	// 输出到控制台
	if (GEngine && GEngine->GameViewport && GEngine->GameViewport->ViewportConsole)
	{
		GEngine->GameViewport->ViewportConsole->OutputText(TextToOutput);
	}

	// 输出到日志
	UE_LOG(LogYcGameCheat, Display, TEXT("%s"), *TextToOutput);
#endif // USING_CHEAT_MANAGER
}

void UYcCheatManager::Cheat(const FString& Msg)
{
	if (AYcPlayerController* YcPC = Cast<AYcPlayerController>(GetOuterAPlayerController()))
	{
		YcPC->ServerCheat(Msg.Left(128));
	}
}

void UYcCheatManager::CheatAll(const FString& Msg)
{
	if (AYcPlayerController* YcPC = Cast<AYcPlayerController>(GetOuterAPlayerController()))
	{
		YcPC->ServerCheatAll(Msg.Left(128));
	}
}

void UYcCheatManager::PlayNextGame(bool bSeamlessTravel)
{
	UYcGameSystemStatics::PlayNextGame(this, bSeamlessTravel);
}