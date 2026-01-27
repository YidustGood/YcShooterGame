// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Abilities/YcGameplayAbility.h"
#include "UObject/Object.h"
#include "YcGameplayAbility_ResetCharacter.generated.h"

class AActor;
class UObject;
struct FGameplayAbilityActorInfo;
struct FGameplayEventData;

/**
 * UYcGameplayAbility_ResetCharacter
 * 角色重置技能类
 *
 * 功能说明：
 *	用于快速将玩家角色重置回初始生成状态的Gameplay技能。
 *	该技能通过 "GameplayEvent.RequestReset" 标签自动触发激活（仅服务器端）。
 *	
 * 主要用途：
 *	- 玩家死亡后重生
 *	- 关卡重置
 *	- 游戏状态回滚
 *
 * 技术特性：
 *	- 实例化策略：每个Actor一个实例
 *	- 网络执行策略：服务器发起
 *	- 自动触发：通过GameplayEvent触发
 */
UCLASS()
class YICHENGAMEPLAY_API UYcGameplayAbility_ResetCharacter : public UYcGameplayAbility
{
	GENERATED_BODY()
public:
	/**
	 * 初始化技能的实例化策略、网络执行策略和触发条件
	 */
	UYcGameplayAbility_ResetCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	/**
	 * 激活技能
	 * 执行角色重置的核心逻辑：
	 *	1. 取消所有正在运行的技能（除了标记为SurvivesDeath的技能）
	 *	2. 调用角色的Reset方法重置状态
	 *	3. 广播重置消息通知其他系统
	 *
	 * @param Handle 技能规格句柄
	 * @param ActorInfo 技能拥有者的Actor信息
	 * @param ActivationInfo 技能激活信息
	 * @param TriggerEventData 触发事件数据（可选）
	 */
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
};

/**
 * FYcPlayerResetMessage
 * 玩家重置消息结构体
 * 
 * 用于在角色重置时广播消息，通知其他系统玩家状态已重置。
 * 包含触发重置的PlayerState对象引用，便于其他系统识别是哪个玩家被重置。
 */
USTRUCT(BlueprintType)
struct FYcPlayerResetMessage
{
	GENERATED_BODY()

	/** 拥有者的PlayerState对象，用于标识是哪个玩家触发了重置 */
	UPROPERTY(BlueprintReadOnly)
	TObjectPtr<AActor> OwnerPlayerState = nullptr;
};