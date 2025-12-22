// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "YcGameplayTagStack.h"
#include "GameFramework/Info.h"
#include "YcTeamInfoBase.generated.h"

class UYcTeamSubsystem;

/** 
 *  表示一个团队的信息数据基类
 *  会自动向YcTeamSubsystem进行注册/注销
 */
UCLASS()
class YICHENTEAMS_API AYcTeamInfoBase : public AInfo
{
	GENERATED_BODY()

public:
	AYcTeamInfoBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * 获取团队ID
	 * @return 团队ID的整数值，如果未分配团队则返回INDEX_NONE
	 */
	int32 GetTeamId() const { return TeamId; }

	//~AActor interface
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	//~End of AActor interface

protected:
	/**
	 * 尝试向队伍子系统注册这个队伍信息
	 * 仅在TeamId有效时才会进行注册
	 */
	void TryRegisterWithTeamSubsystem();
	
	/**
	 * 向子系统注册团队信息
	 * 实际经过检查后转发到这个函数进行，然后向子系统进行注册
	 * @param Subsystem 要注册到的团队子系统
	 */
	virtual void RegisterWithTeamSubsystem(UYcTeamSubsystem* Subsystem);

private:
	/**
	 * 设置团队ID
	 * 注意：此函数只能在服务器端调用，且TeamId必须为INDEX_NONE
	 * @param NewTeamId 新的团队ID
	 */
	void SetTeamId(int32 NewTeamId);

	/** 团队ID复制属性变更时的回调函数 */
	UFUNCTION()
	void OnRep_TeamId();

public:
	friend class UYcTeamCreationComponent;
	friend class UYcTeamSubsystem;

private:
	/** 
	 *  基于GameplayTag的数据存储容器, 支持网络复制, 例如用来存储团队得分信息等
	 *  在YcTeamSubsystem中提供了基于队伍ID的增删查接口
	 */
	UPROPERTY(BlueprintReadOnly, Replicated, meta = (AllowPrivateAccess = "true"))
	FYcGameplayTagStackContainer TeamTags;
	
	/** 团队ID，仅在初始时进行网络复制 */
	UPROPERTY(ReplicatedUsing=OnRep_TeamId)
	int32 TeamId;
};