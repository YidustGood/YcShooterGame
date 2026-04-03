// Copyright (c) 2025 YiChen. All Rights Reserved.

#include "Debug/YcDamageDebugSettings.h"
#include "HAL/IConsoleManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcDamageDebugSettings)

FYcDamageDebugSettings UYcDamageDebugSubsystem::Settings;

void UYcDamageDebugSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

#if !UE_BUILD_SHIPPING
	// 注册控制台命令（使用 ECVF_Cheat 确保在 PIE 中可用）

	// 总开关命令
	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Yc.Damage.EnableAll"),
		TEXT("Toggle all damage debug features (0/1)"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&HandleEnableAllCommand),
		ECVF_Cheat
	);

	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Yc.Damage.Debug"),
		TEXT("Toggle damage debug log (0/1)"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&HandleDebugCommand),
		ECVF_Cheat
	);

	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Yc.Damage.Visualize"),
		TEXT("Toggle damage visualization (0/1)"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&HandleVisualizationCommand),
		ECVF_Cheat
	);

	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Yc.Damage.ShowProcess"),
		TEXT("Toggle showing damage calculation process (0/1)"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&HandleShowProcessCommand),
		ECVF_Cheat
	);

	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Yc.Damage.ShowComponents"),
		TEXT("Toggle showing component execution order (0/1)"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&HandleShowComponentOrderCommand),
		ECVF_Cheat
	);

	IConsoleManager::Get().RegisterConsoleCommand(
		TEXT("Yc.Damage.Duration"),
		TEXT("Set visualization duration in seconds (default: 3.0)"),
		FConsoleCommandWithArgsDelegate::CreateStatic(&HandleDurationCommand),
		ECVF_Cheat
	);

	UE_LOG(LogTemp, Log, TEXT("YcDamageDebugSubsystem initialized. Commands: Yc.Damage.EnableAll, Yc.Damage.Debug, Yc.Damage.Visualize, etc."));
#endif
}

void UYcDamageDebugSubsystem::Deinitialize()
{
	Super::Deinitialize();

#if !UE_BUILD_SHIPPING
	// 控制台命令会自动在模块卸载时清理，无需手动注销
#endif
}

#if !UE_BUILD_SHIPPING
void UYcDamageDebugSubsystem::HandleEnableAllCommand(const TArray<FString>& Args)
{
	bool bEnable = true;
	if (Args.Num() > 0)
	{
		bEnable = FCString::Atoi(*Args[0]) != 0;
	}
	else
	{
		// 无参数时切换状态
		bEnable = !Settings.bEnableDebugLog || !Settings.bEnableVisualization;
	}

	// 设置所有开关
	Settings.bEnableDebugLog = bEnable;
	Settings.bEnableVisualization = bEnable;
	Settings.bShowCalculationProcess = bEnable;
	Settings.bShowComponentOrder = bEnable;

	UE_LOG(LogTemp, Log, TEXT("YcDamage EnableAll: %s"), bEnable ? TEXT("ON") : TEXT("OFF"));
	UE_LOG(LogTemp, Log, TEXT("  - Debug: %s"), Settings.bEnableDebugLog ? TEXT("ON") : TEXT("OFF"));
	UE_LOG(LogTemp, Log, TEXT("  - Visualize: %s"), Settings.bEnableVisualization ? TEXT("ON") : TEXT("OFF"));
	UE_LOG(LogTemp, Log, TEXT("  - ShowProcess: %s"), Settings.bShowCalculationProcess ? TEXT("ON") : TEXT("OFF"));
	UE_LOG(LogTemp, Log, TEXT("  - ShowComponents: %s"), Settings.bShowComponentOrder ? TEXT("ON") : TEXT("OFF"));
}

void UYcDamageDebugSubsystem::HandleDebugCommand(const TArray<FString>& Args)
{
	if (Args.Num() > 0)
	{
		Settings.bEnableDebugLog = FCString::Atoi(*Args[0]) != 0;
	}
	else
	{
		Settings.bEnableDebugLog = !Settings.bEnableDebugLog;
	}
	UE_LOG(LogTemp, Log, TEXT("YcDamage Debug Log: %s"), Settings.bEnableDebugLog ? TEXT("ON") : TEXT("OFF"));
}

void UYcDamageDebugSubsystem::HandleVisualizationCommand(const TArray<FString>& Args)
{
	if (Args.Num() > 0)
	{
		Settings.bEnableVisualization = FCString::Atoi(*Args[0]) != 0;
	}
	else
	{
		Settings.bEnableVisualization = !Settings.bEnableVisualization;
	}
	UE_LOG(LogTemp, Log, TEXT("YcDamage Visualization: %s"), Settings.bEnableVisualization ? TEXT("ON") : TEXT("OFF"));
}

void UYcDamageDebugSubsystem::HandleShowProcessCommand(const TArray<FString>& Args)
{
	if (Args.Num() > 0)
	{
		Settings.bShowCalculationProcess = FCString::Atoi(*Args[0]) != 0;
	}
	else
	{
		Settings.bShowCalculationProcess = !Settings.bShowCalculationProcess;
	}
	UE_LOG(LogTemp, Log, TEXT("YcDamage Show Process: %s"), Settings.bShowCalculationProcess ? TEXT("ON") : TEXT("OFF"));
}

void UYcDamageDebugSubsystem::HandleShowComponentOrderCommand(const TArray<FString>& Args)
{
	if (Args.Num() > 0)
	{
		Settings.bShowComponentOrder = FCString::Atoi(*Args[0]) != 0;
	}
	else
	{
		Settings.bShowComponentOrder = !Settings.bShowComponentOrder;
	}
	UE_LOG(LogTemp, Log, TEXT("YcDamage Show Components: %s"), Settings.bShowComponentOrder ? TEXT("ON") : TEXT("OFF"));
}

void UYcDamageDebugSubsystem::HandleDurationCommand(const TArray<FString>& Args)
{
	if (Args.Num() > 0)
	{
		Settings.VisualizationDuration = FCString::Atof(*Args[0]);
		Settings.VisualizationDuration = FMath::Max(0.1f, Settings.VisualizationDuration);
	}
	UE_LOG(LogTemp, Log, TEXT("YcDamage Visualization Duration: %.2f seconds"), Settings.VisualizationDuration);
}
#endif
