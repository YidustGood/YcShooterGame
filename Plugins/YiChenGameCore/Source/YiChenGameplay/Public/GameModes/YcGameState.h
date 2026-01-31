// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "ModularGameState.h"
#include "YcGameplayTagStack.h"
#include "YcGameState.generated.h"

struct FYcGameVerbMessage;
class UYcExperienceManagerComponent;
class UYcAbilitySystemComponent;
class AYcPlayerState;

/**
 * 游戏状态类
 * 
 * 负责管理游戏体验的加载和生命周期。持有ExperienceManagerComponent组件，
 * 该组件负责Experience的加载、GameFeature插件的激活、以及GameFeatureAction的执行。
 * 作为GameState的扩展，支持模块化架构。
 */
UCLASS(Config = Game)
class YICHENGAMEPLAY_API AYcGameState : public AModularGameStateBase, public IAbilitySystemInterface
{
	GENERATED_BODY()
public:
	AYcGameState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	
	//~AActor interface
	/* 组件初始化完成后设置ASC组件的Avatar为自己 */
	virtual void PostInitializeComponents() override;
	//~End of AActor interface
	
	//~IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	//~End of IAbilitySystemInterface
	
	/**
	 * 向所有客户端发送不可靠消息（可能丢失）
	 * 仅用于可以容忍丢失的客户端通知，如击杀提示、玩家加入消息等
	 * 
	 * @param Message - 要发送的游戏消息
	 */
	UFUNCTION(NetMulticast, Unreliable, BlueprintCallable, Category = "YcGameCore|GameState")
	void MulticastMessageToClients(const FYcGameVerbMessage Message);

	/**
	 * 向所有客户端发送可靠消息（保证送达）
	 * 仅用于不能容忍丢失的重要客户端通知
	 * 
	 * @param Message - 要发送的游戏消息
	 */
	UFUNCTION(NetMulticast, Reliable, BlueprintCallable, Category = "YcGameCore|GameState")
	void MulticastReliableMessageToClients(const FYcGameVerbMessage Message);
	
private:
	/**
	 * 游戏体验管理组件
	 * 负责加载和管理当前游戏体验，包括Experience资产的加载、GameFeature插件的激活、
	 * 以及GameFeatureAction的执行。支持网络复制以同步客户端状态。
	 */
	UPROPERTY()
	TObjectPtr<UYcExperienceManagerComponent> ExperienceManagerComponent;
	
	/**
	 * 游戏级别的能力系统组件
	 * 
	 * 功能说明：
	 * 用于处理游戏全局范围的GAS功能，主要用于播放全局性的Gameplay Cues（游戏提示效果）。
	 * 
	 * 典型应用场景：
	 * - 对局阶段：通过GA来组织游戏对局的阶段
	 * - 环境状态：毒圈收缩、安全区提示、全局Buff/Debuff效果
	 * - 全局音效：游戏开始/结束音效、回合切换音效
	 * - 全局特效：天气变化、昼夜循环、全场景光效
	 * - 全局UI提示：击杀播报、任务完成通知、系统公告
	 * 
	 * 设计考量：
	 * 与PlayerState/Pawn上的ASC不同，此组件不绑定到特定玩家或角色，
	 * 而是作为游戏全局状态的一部分，用于触发影响所有玩家的视听效果。
	 */
	UPROPERTY(VisibleAnywhere, Category = "YcGameCore|GameState")
	TObjectPtr<UYcAbilitySystemComponent> AbilitySystemComponent;

public:
	/**
	 * 游戏状态标签堆栈系统
	 * 
	 * 功能说明：
	 * 基于GameplayTag的堆栈计数系统，支持网络复制，用于记录游戏全局状态的数值数据。
	 * 
	 * 典型应用场景：
	 * - 比赛配置：Tag.Match.TargetScore（目标分数）、Tag.Match.TimeLimit（时间限制）
	 * - 比赛状态：Tag.Match.RemainingTime（剩余时间）、Tag.Match.CurrentRound（当前回合）
	 * - 团队分数：Tag.Team.Red.Score、Tag.Team.Blue.Score
	 * - 全局计数：Tag.Global.TotalKills（总击杀数）、Tag.Global.PlayersAlive（存活玩家数）
	 * - 环境状态：Tag.Environment.SafeZoneRadius（安全区半径）
	 * 
	 * 优势：
	 * - 自动网络同步，客户端可以实时获取游戏状态数据
	 * - 基于Tag的灵活性，易于扩展新的状态类型
	 * - 与Phase系统配合，Phase可以设置和更新这些数据
	 */

	/**
	 * 添加游戏状态标签堆栈
	 * 为指定的GameplayTag增加堆栈数量
	 * 
	 * @param Tag 要操作的GameplayTag
	 * @param StackCount 要添加的堆栈数量（小于1时不执行任何操作）
	 * 
	 * 示例：AddStatTagStack(Tag.Match.TargetScore, 25) // 设置目标分数为25
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "YcGameCore|GameState")
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	/**
	 * 移除游戏状态标签堆栈
	 * 从指定的GameplayTag减少堆栈数量
	 * 
	 * @param Tag 要操作的GameplayTag
	 * @param StackCount 要移除的堆栈数量（小于1时不执行任何操作）
	 * 
	 * 示例：RemoveStatTagStack(Tag.Match.RemainingTime, 1) // 剩余时间减1秒
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "YcGameCore|GameState")
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	/**
	 * 设置游戏状态标签堆栈
	 * 直接设置指定GameplayTag的堆栈数量（覆盖原有值）
	 * 
	 * @param Tag 要操作的GameplayTag
	 * @param StackCount 要设置的堆栈数量
	 * 
	 * 示例：SetStatTagStack(Tag.Match.RemainingTime, 600) // 设置剩余时间为600秒
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "YcGameCore|GameState")
	void SetStatTagStack(FGameplayTag Tag, int32 StackCount);

	/**
	 * 获取游戏状态标签堆栈数量
	 * 返回指定GameplayTag的当前堆栈计数
	 * 
	 * @param Tag 要查询的GameplayTag
	 * @return 堆栈数量（如果Tag不存在则返回0）
	 * 
	 * 示例：int32 TargetScore = GetStatTagStackCount(Tag.Match.TargetScore) // 获取目标分数
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|GameState")
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	/**
	 * 检查是否拥有游戏状态标签
	 * 判断指定的GameplayTag是否存在且至少有1层堆栈
	 * 
	 * @param Tag 要检查的GameplayTag
	 * @return 如果存在至少一层堆栈则返回true，否则返回false
	 * 
	 * 示例：if (HasStatTag(Tag.Match.Overtime)) // 检查是否处于加时赛
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|GameState")
	bool HasStatTag(FGameplayTag Tag) const;

	/**
	 * 获取按指定标签排序的玩家列表
	 * 根据玩家 PlayerState 中指定 GameplayTag 的堆栈数量进行排序
	 * 
	 * @param SortTag 用于排序的 GameplayTag（如击杀数、分数等）
	 * @param bDescending 是否降序排列（true=从高到低，false=从低到高）
	 * @return 排序后的玩家 PlayerState 数组
	 * 
	 * 性能说明：
	 * - 使用 C++ 标准库的快速排序算法（O(n log n)）
	 * - 适合频繁调用的场景（如计分板更新）
	 * 
	 * 示例：
	 * - 按击杀数排序：GetPlayersSortedByTag(Tag.Score.Eliminations, true)
	 * - 按死亡数排序：GetPlayersSortedByTag(Tag.Score.Deaths, false)
	 * - 按助攻数排序：GetPlayersSortedByTag(Tag.Score.Assists, true)
	 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|GameState")
	TArray<AYcPlayerState*> GetPlayersSortedByTag(FGameplayTag SortTag, bool bDescending = true) const;
	
private:
	/**
	 * 游戏状态标签堆栈容器
	 * 存储所有游戏状态标签及其对应的堆栈数量，支持网络复制
	 * 
	 * 网络复制说明：
	 * - 服务器端修改会自动同步到所有客户端
	 * - 客户端只能读取，不能直接修改（通过RPC调用服务器端的Add/Remove/Set方法）
	 */
	UPROPERTY(VisibleInstanceOnly, Replicated)
	FYcGameplayTagStackContainer StatTags;
};