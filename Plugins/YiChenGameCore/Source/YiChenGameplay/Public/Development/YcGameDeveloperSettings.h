// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Engine/DeveloperSettingsBackedByCVars.h"
#include "YcGameDeveloperSettings.generated.h"

/** 作弊命令执行时机 */
UENUM()
enum class ECheatExecutionTime
{
	/** 当CheatManager创建时执行 */
	OnCheatManagerCreated,

	/** 当玩家控制Pawn时执行 */
	OnPlayerPawnPossession
};

/** 单条自动执行的作弊命令配置 */
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
 * 游戏开发者设置
 * 
 * 用于配置编辑器下的开发辅助功能，包括：
 * - Experience覆盖（快速测试不同游戏模式）
 * - 自动执行的作弊命令（加速开发迭代）
 * - 游戏流程控制（跳过等待阶段等）
 * - Bot配置（数量、攻击行为等）
 * - 常用地图快捷访问
 * 
 * 配置位置：Project Settings -> [项目名称] -> Game Developer Settings
 */
UCLASS(config=EditorPerProjectUserSettings, MinimalAPI)
class UYcGameDeveloperSettings : public UDeveloperSettingsBackedByCVars
{
	GENERATED_BODY()
public:
	UYcGameDeveloperSettings();
	
	//~UDeveloperSettings interface
	/**
	 * 返回在编辑器Project Settings中显示的设置分类名称
	 * @return 分类名称（使用当前工程名）
	 */
	virtual FName GetCategoryName() const override;
	//~End of UDeveloperSettings interface
	
	/**
	 * PIE模式下的Experience覆盖
	 * 如果设置，将使用此Experience而不是地图世界设置中的默认Experience
	 * 用于快速测试不同的游戏模式而无需修改地图配置
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category="YcGameCore", meta=(AllowedTypes="YcExperienceDefinition"))
	FPrimaryAssetId ExperienceOverride;
	
	/**
	 * PIE期间自动执行的作弊命令列表
	 * 可以在CheatManager创建时或玩家控制Pawn时自动执行
	 * 用于快速设置测试环境（如无敌、无限弹药等）
	 */
	UPROPERTY(config, EditAnywhere, Category=YcGameCore)
	TArray<FYcCheatToRun> CheatsToRun;
	
	/**
	 * PIE模式下是否执行完整游戏流程
	 * 
	 * true: 执行完整流程（包括等待玩家、加载界面、游戏阶段切换等）
	 * false: 跳过等待阶段，直接进入游戏玩法（加快测试迭代速度）
	 * 
	 * 用途：
	 * - 开发期间设为false可快速测试玩法
	 * - 正式测试时设为true验证完整流程
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=SC)
	bool bTestFullGameFlowInPIE = false;
	
	/** 是否覆盖Bot数量配置 */
	UPROPERTY(BlueprintReadOnly, config, Category=SCBots)
	bool bOverrideBotCount = false;

	/**
	 * 覆盖的Bot数量
	 * 仅在bOverrideBotCount为true时生效
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=SCBots, meta=(EditCondition=bOverrideBotCount))
	int32 OverrideNumPlayerBotsToSpawn = 0;

	/**
	 * 是否允许Bot攻击
	 * 仅在编辑器模式下可控制，用于测试时禁用Bot攻击行为
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, config, Category=SCBots)
	bool bAllowPlayerBotsToAttack = true;
	
public:
#if WITH_EDITORONLY_DATA
	/**
	 * 常用地图列表
	 * 这些地图可以通过编辑器工具栏快捷访问，方便快速切换测试地图
	 */
	UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category=Maps, meta=(AllowedClasses="/Script/Engine.World"))
	TArray<FSoftObjectPath> CommonEditorMaps;
#endif
	
#if WITH_EDITOR
	/**
	 * PIE启动时调用
	 * 当启用Experience覆盖或作弊设置时弹出提醒通知
	 */
	YICHENGAMEPLAY_API void OnPlayInEditorStarted() const;

private:
	/**
	 * 应用开发者设置
	 * 在配置变更时调用，用于同步控制台变量等
	 */
	void ApplySettings();
#endif

public:
	//~UObject interface
#if WITH_EDITOR
	/**
	 * 属性在编辑器中被修改时调用
	 * 立即应用新的配置
	 */
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	
	/**
	 * 配置文件重新加载后调用
	 * 重新应用配置
	 */
	virtual void PostReloadConfig(FProperty* PropertyThatWasLoaded) override;
	
	/**
	 * 对象属性初始化完成后调用
	 * 应用初始配置
	 */
	virtual void PostInitProperties() override;
#endif
	//~End of UObject interface
	
public:
	/**
	 * 是否应该跳过游戏流程直接进入玩法
	 * 
	 * @return true表示跳过等待阶段直接开始游戏，false表示执行完整游戏流程
	 * 
	 * 用途：供游戏代码查询当前是否处于快速测试模式
	 * 注意：仅在编辑器PIE模式下生效
	 */
	UFUNCTION(BlueprintCallable, Category="YcGameCore")
	static bool ShouldSkipDirectlyToGameplay();
};
