// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "ModularAIController.h"
#include "YcTeamAgentInterface.h"
#include "YcAIController.generated.h"

/**
 * YiChenGameCore插件提供的AI控制器类
 * 
 * 用于控制游戏中的AI Bot，支持队伍系统和AI感知系统
 * 继承自ModularAIController以支持模块化组件架构
 * 实现IYcTeamAgentInterface以支持队伍管理功能
 */
UCLASS(Blueprintable)
class YICHENGAMEPLAY_API AYcAIController : public AModularAIController, public IYcTeamAgentInterface
{
	GENERATED_BODY()
	
public:
	AYcAIController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~IYcTeamAgentInterface interface
	/**
	 * 设置队伍ID（AI控制器禁止直接设置）
	 * 注意：AI Bot的队伍ID由其关联的PlayerState决定，不允许直接设置
	 * @param NewTeamID 新的队伍ID
	 */
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	
	/**
	 * 获取当前队伍ID
	 * 从关联的PlayerState中获取队伍信息
	 * @return 当前队伍ID，如果没有PlayerState则返回NoTeam
	 */
	virtual FGenericTeamId GetGenericTeamId() const override;
	
	/**
	 * 获取队伍变更委托
	 * @return 队伍索引变更时触发的委托指针
	 */
	virtual FOnYcTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	
	/**
	 * 获取对其他Actor的队伍态度
	 * 根据队伍ID判断是友军还是敌军
	 * @param Other 目标Actor
	 * @return 队伍态度（友好/敌对/中立）
	 */
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	//~End of IYcTeamAgentInterface interface
	
	/**
	 * 服务器端重启控制器
	 * 用于重生AI Bot等场景
	 */
	void ServerRestartController();
	
	/**
	 * 更新AI的队伍态度
	 * 刷新AI感知组件的刺激监听器，使队伍变更生效
	 * @param AIPerception AI感知组件
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore")
	void UpdateTeamAttitude(UAIPerceptionComponent* AIPerception);
	
	/**
	 * 解除对Pawn的控制时调用
	 * 确保被解除控制的Pawn不再作为ASC的Avatar Actor
	 */
	virtual void OnUnPossess() override;
	
private:
	/**
	 * PlayerState队伍变更回调
	 * 当关联的PlayerState队伍发生变化时触发
	 * @param TeamAgent 队伍代理对象
	 * @param OldTeam 旧队伍ID
	 * @param NewTeam 新队伍ID
	 */
	UFUNCTION()
	void OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam);
	
protected:
	/**
	 * PlayerState设置或清除时调用
	 * 派生类可重写此方法以响应PlayerState变更
	 */
	virtual void OnPlayerStateChanged();

private:
	/**
	 * 广播PlayerState变更事件
	 * 处理旧PlayerState的解绑和新PlayerState的绑定
	 * 并在队伍真正发生变化时广播队伍变更事件
	 */
	void BroadcastOnPlayerStateChanged();

protected:	
	//~AController interface
	/** 初始化PlayerState时调用 */
	virtual void InitPlayerState() override;
	
	/** 清理PlayerState时调用 */
	virtual void CleanupPlayerState() override;
	
	/** PlayerState复制时调用（客户端） */
	virtual void OnRep_PlayerState() override;
	//~End of AController interface

private:
	/** 队伍变更委托 */
	UPROPERTY()
	FOnYcTeamIndexChangedDelegate OnTeamChangedDelegate;

	/** 上一次记录的PlayerState，用于检测PlayerState变更 */
	UPROPERTY()
	TObjectPtr<APlayerState> LastSeenPlayerState;
};
