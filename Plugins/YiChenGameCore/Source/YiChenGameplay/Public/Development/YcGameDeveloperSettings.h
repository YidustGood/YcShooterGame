// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "YcGameDeveloperSettings.generated.h"

/** 作弊命令执行时机 */
UENUM()
enum class ECheatExecutionTime
{
	/** 当 CheatManager 创建时执行 */
	OnCheatManagerCreated,

	/** 当玩家控制一个 Pawn 时执行 */
	OnPlayerPawnPossession
};

/** 单条需要自动执行的作弊命令配置 */
USTRUCT()
struct FYcCheatToRun
{
	GENERATED_BODY()

	/** 执行该作弊命令的时机 */
	UPROPERTY(EditAnywhere)
	ECheatExecutionTime Phase = ECheatExecutionTime::OnPlayerPawnPossession;

	/** 要执行的作弊命令字符串（控制台命令格式） */
	UPROPERTY(EditAnywhere)
	FString Cheat;
};

/**
 * 游戏开发相关 Developer Settings / 编辑器作弊配置
 * 用于配置编辑器下体验覆盖、自动运行的作弊命令以及常用地图等开发期功能
 */
UCLASS(config=EditorPerProjectUserSettings, MinimalAPI)
class UYcGameDeveloperSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()
public:
	UYcGameDeveloperSettings();
	
	//~UDeveloperSettings interface
	/**
	 * 返回在编辑器 Project Settings 中显示的设置分类名称
	 * @return 分类名称（通常为当前工程名）
	 */
	virtual FName GetCategoryName() const override;
	//~End of UDeveloperSettings interface
	
	// 在编辑器中用于「Play In Editor」的体验覆盖（如果未设置，则使用当前地图世界设置中的默认体验）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category="YcGameCore", meta=(AllowedTypes="YcExperienceDefinition"))
	FPrimaryAssetId ExperienceOverride;
	
	/** 在「Play In Editor」期间自动执行的作弊命令列表 */
	UPROPERTY(config, EditAnywhere, Category=YcGameCore)
	TArray<FYcCheatToRun> CheatsToRun;
	
public:
#if WITH_EDITORONLY_DATA
	/** 常用地图列表，这些地图可以通过编辑器工具栏快捷访问 */
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category=Maps, meta=(AllowedClasses="/Script/Engine.World"))
	TArray<FSoftObjectPath> CommonEditorMaps;
#endif
	
#if WITH_EDITOR
	/** 由编辑器引擎调用，用于在启用体验覆盖或作弊设置时弹出提醒通知 */
	YICHENGAMEPLAY_API void OnPlayInEditorStarted() const;

private:
	void ApplySettings();
#endif

public:
	//~UObject interface
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostReloadConfig(FProperty* PropertyThatWasLoaded) override;
	virtual void PostInitProperties() override;
#endif
	//~End of UObject interface
	
	
};
