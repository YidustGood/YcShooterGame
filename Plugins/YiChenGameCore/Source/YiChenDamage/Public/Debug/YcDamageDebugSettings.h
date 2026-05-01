// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "YcDamageDebugSettings.generated.h"

/**
 * 伤害调试设置。
 * 控制台命令会修改这里的全局调试开关，供伤害链路各环节读取。
 */
USTRUCT(BlueprintType)
struct FYcDamageDebugSettings
{
	GENERATED_BODY()

	/** 是否启用伤害调试日志。 */
	UPROPERTY(BlueprintReadOnly)
	bool bEnableDebugLog = false;

	/** 是否启用伤害可视化。 */
	UPROPERTY(BlueprintReadOnly)
	bool bEnableVisualization = false;

	/** 是否显示伤害计算过程。 */
	UPROPERTY(BlueprintReadOnly)
	bool bShowCalculationProcess = false;

	/** 是否显示组件执行顺序。 */
	UPROPERTY(BlueprintReadOnly)
	bool bShowComponentOrder = false;

	/** 可视化持续时间，单位秒。 */
	UPROPERTY(BlueprintReadOnly)
	float VisualizationDuration = 3.0f;

	/** 可视化文字大小。 */
	UPROPERTY(BlueprintReadOnly)
	float TextSize = 1.0f;
};

/**
 * 伤害调试子系统。
 * 负责暴露全局伤害调试设置；控制台命令本身在模块级静态注册，
 * 这样 PIE 重进时不会重复注册同名命令。
 */
UCLASS()
class YICHENDAMAGE_API UYcDamageDebugSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 获取当前调试设置。 */
	static const FYcDamageDebugSettings& GetSettings() { return Settings; }

	/** 是否启用调试日志。 */
	static bool IsDebugLogEnabled() { return Settings.bEnableDebugLog; }

	/** 是否启用可视化。 */
	static bool IsVisualizationEnabled() { return Settings.bEnableVisualization; }

#if !UE_BUILD_SHIPPING
	/** 控制台命令处理函数。 */
	static void HandleEnableAllCommand(const TArray<FString>& Args);
	static void HandleDebugCommand(const TArray<FString>& Args);
	static void HandleVisualizationCommand(const TArray<FString>& Args);
	static void HandleShowProcessCommand(const TArray<FString>& Args);
	static void HandleShowComponentOrderCommand(const TArray<FString>& Args);
	static void HandleDurationCommand(const TArray<FString>& Args);
#endif

private:
	/** 全局调试设置。 */
	static FYcDamageDebugSettings Settings;
};
