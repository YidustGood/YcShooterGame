// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "ModularPlayerState.h"
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
	// @TODO 基于GameplayTag且支持网络复制的StatTagStack功能, 可用来记录一些PS的数值数据
};
