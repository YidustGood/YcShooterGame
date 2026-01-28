// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CommonPlayerController.h"
#include "YcTeamAgentInterface.h"
#include "YcPlayerController.generated.h"

class AYcPlayerState;
class UYcAbilitySystemComponent;

/**
 * 插件Gameplay框架提供的玩家控制器基类
 * 
 * 功能说明：
 * - 与插件其它模块系统协同工作
 * - 实现队伍系统接口（IYcTeamAgentInterface）
 * - 管理玩家输入和作弊命令
 * 
 * 队伍系统说明：
 * - Controller 实现 IYcTeamAgentInterface 接口
 * - 队伍信息从 PlayerState 中获取
 * - 性能优化：直接访问 PlayerState，避免复杂查找
 */
UCLASS(Config = Game, Meta = (ShortTooltip = "The base player controller class used by this project."))
class YICHENGAMEPLAY_API AYcPlayerController : public ACommonPlayerController, public IYcTeamAgentInterface
{
	GENERATED_BODY()
public:
	AYcPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	/**
	 * 获取当前控制器对应的 YcPlayerState
	 * @return 转换后的 AYcPlayerState 指针，如果当前 PlayerState 不是 AYcPlayerState 类型则返回 nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|PlayerController")
	AYcPlayerState* GetYcPlayerState() const;
	
	/**
	 * 获取当前控制器对应的 YcAbilitySystemComponent
	 * @return YcAbilitySystemComponent 指针，如果未找到则返回 nullptr
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|PlayerController")
	UYcAbilitySystemComponent* GetYcAbilitySystemComponent() const;

	//~IYcTeamAgentInterface interface
	/**
	 * 设置队伍ID（Controller 不允许直接设置）
	 * 注意：Controller 的队伍ID由其 PlayerState 决定
	 * @param NewTeamID 新的队伍ID
	 */
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	
	/**
	 * 获取队伍ID
	 * 性能优化：直接从 PlayerState 获取，避免复杂查找
	 * @return 当前队伍ID，如果没有 PlayerState 则返回 NoTeam
	 */
	virtual FGenericTeamId GetGenericTeamId() const override;
	
	/**
	 * 获取队伍变更委托
	 * 从 PlayerState 获取
	 * @return 队伍变更委托指针
	 */
	virtual FOnYcTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	
	/**
	 * 获取对其他 Actor 的队伍态度
	 * 根据队伍ID判断是友军还是敌军
	 * @param Other 目标Actor
	 * @return 队伍态度（友好/敌对/中立）
	 */
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	//~End of IYcTeamAgentInterface interface
	
	//~APlayerController interface
	virtual void PreProcessInput(const float DeltaTime, const bool bGamePaused) override;
	virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;
	//~End of APlayerController interface
	
	//~AController interface
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	//~End of AController interface
	
	////////// Cheat相关 //////////
	/** 在服务器上执行一条作弊命令（仅作用于当前玩家） */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerCheat(const FString& Msg);

	/** 在服务器上执行一条作弊命令（作用于所有玩家） */
	UFUNCTION(Reliable, Server, WithValidation)
	void ServerCheatAll(const FString& Msg);
	////////// ~Cheat相关 //////////
};