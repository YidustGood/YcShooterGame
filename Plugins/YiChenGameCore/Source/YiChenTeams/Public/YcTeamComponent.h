// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "YcTeamAgentInterface.h"
#include "Components/ActorComponent.h"
#include "YcTeamComponent.generated.h"

/**
 * 团队组件, 一般挂载给PlayerState, 这样玩家就拥有了团队系统所需的功能
 * 这个组件提供常规的团队接口实现, 能够满足基本的团队功能需求, 如果需要定制更复杂的团队功能可以通过直接实现IYcTeamAgentInterface接口进行定制
 */
UCLASS(ClassGroup=(YiChenTeams), meta=(BlueprintSpawnableComponent))
class YICHENTEAMS_API UYcTeamComponent : public UActorComponent, public IYcTeamAgentInterface
{
	GENERATED_BODY()
public:
	UYcTeamComponent();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	//~IYcTeamAgentInterface interface
	/**
	 * 设置玩家的通用团队ID
	 * 注意：此函数只能在服务器端调用
	 * @param NewTeamID 新的团队ID
	 */
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	
	/**
	 * 获取玩家的通用团队ID
	 * @return 当前玩家的通用团队ID
	 */
	virtual FGenericTeamId GetGenericTeamId() const override;
	
	/**
	 * 获取团队索引变更委托的指针
	 * @return 团队索引变更委托的指针
	 */
	virtual FOnYcTeamIndexChangedDelegate* GetOnTeamIndexChangedDelegate() override;
	
	/**
	 * 获取玩家的团队ID（整数形式）
	 * @return 团队ID的整数值，如果未分配团队则返回INDEX_NONE
	 */
	virtual int32 GetTeamId() const override;
	//~End of IYcCTeamAgentInterface interface
	
	/**
	 * 获取玩家所属的小队ID
	 * @return 小队ID的整数值
	 */
	UFUNCTION(BlueprintCallable)
	int32 GetSquadId() const;

	/**
	 * 获取玩家所属的团队ID（蓝图版本）
	 * @return 团队ID的整数值，如果未分配团队则返回INDEX_NONE
	 */
	UFUNCTION(BlueprintCallable, DisplayName = "GetTeamId")
	int32 K2_GetTeamId() const;

	/**
	 * 设置玩家的小队ID
	 * 注意：此函数只能在服务器端调用
	 * @param NewSquadID 新的小队ID
	 */
	UFUNCTION(BlueprintCallable)
	void SetSquadID(int32 NewSquadID);
	
protected:
	/** 团队ID复制属性变更时的回调函数 */
	UFUNCTION()
	void OnRep_MyTeamID(FGenericTeamId OldTeamID);
	
	/** 小队ID复制属性变更时的回调函数 */
	UFUNCTION()
	void OnRep_MySquadID(int32 OldSquadId);
	
private:
	/** 
	 * 常规的队伍ID，这是游戏中作为主导的TeamId
	 * PlayerController、Character等类中获取TeamId也是转发到PlayerState中来获取TeamId
	 * 使用基于推送的网络复制
	 */
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_MyTeamID)
	FGenericTeamId MyTeamID;

	/** 
	 * 小队ID，如果需要的话一个队伍中可以分为多个小队
	 * 参考战地大战场模式，盟军就是大队伍，小队则是盟军的一个子队
	 * 使用基于推送的网络复制
	 */
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing=OnRep_MySquadID)
	int32 MySquadID;
	
	/** 团队索引变更委托，当玩家的团队ID发生变化时触发 */
	UPROPERTY(BlueprintAssignable)
	FOnYcTeamIndexChangedDelegate OnTeamChangedDelegate;
};