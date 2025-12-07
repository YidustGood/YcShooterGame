// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "ModularGameMode.h"
#include "YcExperienceDefinition.h"
#include "YcGameMode.generated.h"

enum class ECommonUserOnlineContext : uint8;
enum class ECommonUserPrivilege : uint8;
class UCommonUserInfo;
enum class ECommonSessionOnlineMode : uint8;

/**
 * 本插件的基本游戏模式类
 * 
 * 负责游戏体验的初始化和管理，包括ExperienceId的查找加载、专用服务器的登录处理等。
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "The base game mode class used by this project."))
class YICHENGAMEPLAY_API AYcGameMode : public AModularGameModeBase
{
	GENERATED_BODY()
public:
	AYcGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	
	//~AGameModeBase interface
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	//~End of AGameModeBase interface
	
	/**
	 * 获取指定Controller对应的PawnData
	 * 优先从Controller的PlayerState中获取，如果无效则从当前加载的Experience中获取默认PawnData
	 * @param InController 要查询的Controller
	 * @return 对应的PawnData，如果未找到则返回nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|Pawn")
	const UYcPawnData* GetPawnDataForController(const AController* InController) const;
	
	///////////// Game Experience 相关 /////////////
public:
	/**
	 * 检查游戏体验是否已完全加载
	 * @return 如果体验已加载返回true，否则返回false
	 */
	bool IsExperienceLoaded() const;
protected:
	/**
	 * 寻找并分配可用的游戏体验
	 * 按优先级顺序查找Experience：URL选项 > 开发者设置 > 命令行 > 世界设置 > 默认体验
	 * 在InitGame()中通过定时器延迟到InitGame的下一帧调用，给启动设置初始化时间
	 */
	void HandleMatchAssignmentIfNotExpectingOne();

	/**
	 * 当找到可用的Experience后的处理函数
	 * 将Experience的PrimaryAssetId设置到GameState的ExperienceManagerComponent中，触发Experience的加载流程
	 * @param ExperienceId 要加载的Experience主资产ID
	 * @param ExperienceIdSource Experience的来源标记（用于日志和调试）
	 */
	void OnMatchAssignmentGiven(FPrimaryAssetId ExperienceId, const FString& ExperienceIdSource);

	/**
	 * 游戏体验加载完成后的回调函数
	 * 可用于处理Experience加载完成后的相关逻辑，如生成玩家等
	 * @param CurrentExperience 当前加载的Experience定义对象
	 */
	void OnExperienceLoaded(const UYcExperienceDefinition* CurrentExperience);
	///////////// ~Game Experience 相关 /////////////
	
	////////////// Dedicated Server 相关 /////////////
protected:
	/**
	 * 尝试进行专用服务器登录
	 * 目前仅在专用服务器模式下的默认地图上执行在线登录, 后续需要根据项目后端进行定制处理,这里这是提供个简单参考
	 * @return 如果成功启动专用服务器登录流程返回true，否则返回false
	 */
	virtual bool TryDedicatedServerLogin();
	
	/**
	 * 启动专用服务器匹配会话
	 * @param OnlineMode 在线模式（Online或LAN）
	 */
	void HostDedicatedServerMatch(ECommonSessionOnlineMode OnlineMode);
	
	/**
	 * 专用服务器用户初始化完成的回调函数
	 * 处理在线登录结果，根据登录成功或失败启动相应的服务器模式
	 * @param UserInfo 用户信息
	 * @param bSuccess 登录是否成功
	 * @param Error 错误信息
	 * @param RequestedPrivilege 请求的权限
	 * @param OnlineContext 在线上下文
	 */
	UFUNCTION()
	void OnUserInitializedForDedicatedServer(const UCommonUserInfo* UserInfo, bool bSuccess, FText Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext);
	//////////////// ~Dedicated Server 相关 /////////////
};