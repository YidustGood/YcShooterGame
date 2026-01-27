// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "ModularPlayerState.h"
#include "YcGameplayTagStack.h"
#include "YcPlayerState.generated.h"

class AYcPlayerController;
class UYcAbilitySystemComponent;
class UYcPawnData;
class UYcExperienceDefinition;

/**
 * 插件Gameplay框架提供的玩家状态基类, 与插件其它模块系统协同工作
 */
UCLASS(Config = Game)
class YICHENGAMEPLAY_API AYcPlayerState : public AModularPlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:
	AYcPlayerState(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|PlayerState")
	AYcPlayerController* GetYcPlayerController() const;
	
	/** 返回YcPlayerState的AbilitySystem组件 */
	UFUNCTION(BlueprintCallable, Category = "YcGameCore|PlayerState")
	UYcAbilitySystemComponent* GetYcAbilitySystemComponent() const { return AbilitySystemComponent; }
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	/** 设置PawnData,设置成功后会将PawnData的AbilitySets授予这个Player State的ASC组件 */
	void SetPawnData(const UYcPawnData* InPawnData);
	
	template <class T>
	const T* GetPawnData() const { return Cast<T>(PawnData); }
	
	/** PawnData中技能已经授予ASC后通过游戏框架控制系统发送出去的事件名称, 用于通知其他关注这个事情的系统模块 */
	static const FName NAME_YcAbilityReady;
protected:
	//~AActor interface
	virtual void PreInitializeComponents() override;
	virtual void PostInitializeComponents() override;
	//~End of AActor interface
	
	//~APlayerState interface
	/** PlayerState首次复制到达客户端时调用 */
	virtual void ClientInitialize(AController* C) override;
	//~End of APlayerState interface
private:
	// 当游戏体验加载完成时的回调
	void OnExperienceLoaded(const UYcExperienceDefinition* CurrentExperience);
	
protected:
	UFUNCTION()
	void OnRep_PawnData();
	
	/** 玩家Pawn的数据包，当游戏体验加载完成后从体验定义实例中获取 */
	UPROPERTY(VisibleInstanceOnly, ReplicatedUsing = OnRep_PawnData)
	TObjectPtr<const UYcPawnData> PawnData;
	
private:
	/** 
	 * 玩家的能力系统组件。将其附加到PlayerState上，而非Pawn，主要基于以下设计考量：
	 * 1. 状态持久性：确保玩家的核心游戏状态（如技能、关键Buff/Debuff、属性）在一局游戏会话内持续存在，
	 *    不受Pawn死亡/重生的影响。例如，实现“重生”技能、死亡后依然持续的诅咒效果、或跨死亡的资源累积。
	 * 2. 架构清晰：PlayerState代表玩家的逻辑身份，Pawn代表其当前的物理化身。将核心能力逻辑与逻辑身份绑定，
	 *    更符合职责分离原则，便于管理玩家级别的数据。
	 * 3. 网络同步：PlayerState在服务器和客户端上都稳定存在，是进行玩家状态同步的可靠节点。
	 *
	 * 注意：此选择适用于MOBA、RPG、英雄射击等玩法。对于无需状态持久化的AI或一次性实体，
	 * 将ASC附加到其Pawn上更为合适。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "YcGameCore|PlayerState", meta = (AllowPrivateAccess = true))
	TObjectPtr<UYcAbilitySystemComponent> AbilitySystemComponent;
	
	// @TODO 断线重连数据备份与恢复机制
	// @TODO 队伍机制
	
public:
	/**
	 * 状态标签堆栈系统
	 * 
	 * 功能说明：
	 * 基于GameplayTag的堆栈计数系统，支持网络复制，用于记录玩家状态的数值数据。
	 * 
	 * 典型应用场景：
	 * - 击杀数/死亡数统计：Tag.Stats.Kills, Tag.Stats.Deaths
	 * - 连杀计数：Tag.Stats.KillStreak（可用于触发连杀奖励）
	 * - 任务进度：Tag.Quest.CollectedItems（收集类任务进度）
	 * - 货币/资源：Tag.Currency.Gold, Tag.Currency.Gems
	 * - 状态层数：Tag.Status.Buff.Strength（可叠加的增益效果层数）
	 * - 成就进度：Tag.Achievement.Progress（某个成就的完成进度）
	 * 
	 * 优势：
	 * - 自动网络同步，无需手动编写复制代码
	 * - 基于Tag的灵活性，易于扩展新的统计类型
	 * - 堆栈机制天然支持累加/递减操作
	 */

	/**
	 * 添加状态标签堆栈
	 * 为指定的GameplayTag增加堆栈数量
	 * 
	 * @param Tag 要操作的GameplayTag
	 * @param StackCount 要添加的堆栈数量（小于1时不执行任何操作）
	 * 
	 * 示例：AddStatTagStack(Tag.Stats.Kills, 1) // 击杀数+1
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	void AddStatTagStack(FGameplayTag Tag, int32 StackCount);

	/**
	 * 移除状态标签堆栈
	 * 从指定的GameplayTag减少堆栈数量
	 * 
	 * @param Tag 要操作的GameplayTag
	 * @param StackCount 要移除的堆栈数量（小于1时不执行任何操作）
	 * 
	 * 示例：RemoveStatTagStack(Tag.Currency.Gold, 100) // 消耗100金币
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Teams)
	void RemoveStatTagStack(FGameplayTag Tag, int32 StackCount);

	/**
	 * 获取状态标签堆栈数量
	 * 返回指定GameplayTag的当前堆栈计数
	 * 
	 * @param Tag 要查询的GameplayTag
	 * @return 堆栈数量（如果Tag不存在则返回0）
	 * 
	 * 示例：int32 Kills = GetStatTagStackCount(Tag.Stats.Kills) // 获取当前击杀数
	 */
	UFUNCTION(BlueprintCallable, Category=Teams)
	int32 GetStatTagStackCount(FGameplayTag Tag) const;

	/**
	 * 检查是否拥有状态标签
	 * 判断指定的GameplayTag是否存在且至少有1层堆栈
	 * 
	 * @param Tag 要检查的GameplayTag
	 * @return 如果存在至少一层堆栈则返回true，否则返回false
	 * 
	 * 示例：if (HasStatTag(Tag.Status.InCombat)) // 检查是否处于战斗状态
	 */
	UFUNCTION(BlueprintCallable, Category=Teams)
	bool HasStatTag(FGameplayTag Tag) const;
	
private:
	/**
	 * 状态标签堆栈容器
	 * 存储所有状态标签及其对应的堆栈数量，支持网络复制
	 * 
	 * 网络复制说明：
	 * - 服务器端修改会自动同步到所有客户端
	 * - 客户端只能读取，不能直接修改（通过RPC调用服务器端的Add/Remove方法）
	 */
	UPROPERTY(VisibleInstanceOnly, Replicated)
	FYcGameplayTagStackContainer StatTags;
};
