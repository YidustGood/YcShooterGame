// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Components/GameStateComponent.h"
#include "YcAIBotCreationComponent.generated.h"


class UYcExperienceDefinition;
class AAIController;

/**
 * AI Bot创建组件
 * 
 * 挂载在GameState上的组件，负责在对局中创建和管理AI Bot
 * 支持通过配置、命令行参数和URL参数动态控制Bot数量
 * 
 * 使用方式：
 * 1. 在GameState蓝图中添加此组件
 * 2. 配置BotControllerClass和NumBotsToCreate
 * 3. 组件会在Experience加载完成后自动创建Bot
 */
UCLASS(Blueprintable, Abstract)
class YICHENGAMEPLAY_API UYcAIBotCreationComponent : public UGameStateComponent
{
	GENERATED_BODY()
public:
	UYcAIBotCreationComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UActorComponent interface
	virtual void BeginPlay() override;
	//~End of UActorComponent interface

private:
	/**
	 * Experience加载完成回调
	 * 在Experience加载完成后触发Bot创建流程
	 * @param Experience 已加载的Experience定义
	 */
	void OnExperienceLoaded(const UYcExperienceDefinition* Experience);
	
protected:
	/**
	 * 生成单个Bot
	 * 创建AI控制器、初始化PlayerState、重启玩家并生成Pawn
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Gameplay)
	virtual void SpawnOneBot();

	/**
	 * 移除一个Bot
	 * 随机选择一个已生成的Bot并销毁
	 */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category=Gameplay)
	virtual void RemoveOneBot();

	/**
	 * 服务器端创建Bot
	 * 根据配置、开发者设置、命令行参数和URL参数确定Bot数量并批量创建
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintAuthorityOnly, Category=Gameplay)
	void ServerCreateBots();

protected:
	/** 默认创建的AI Bot数量 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Gameplay)
	int32 NumBotsToCreate = 5;

	/** AI控制器类，用于创建Bot */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Gameplay)
	TSubclassOf<AAIController> BotControllerClass;

	/** Bot随机名称列表，用于为Bot分配名称 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category=Gameplay)
	TArray<FString> RandomBotNames;

	/** 剩余可用的Bot名称列表（运行时使用） */
	TArray<FString> RemainingBotNames;

	/** 已生成的Bot控制器列表 */
	UPROPERTY(Transient)
	TArray<TObjectPtr<AAIController>> SpawnedBotList;

#if WITH_SERVER_CODE
public:
	/** 作弊命令：添加一个Bot */
	void Cheat_AddBot() { SpawnOneBot(); }
	
	/** 作弊命令：移除一个Bot */
	void Cheat_RemoveBot() { RemoveOneBot(); }

	/**
	 * 创建Bot名称
	 * 从随机名称列表中选择，如果列表为空则生成默认名称
	 * @param PlayerIndex 玩家索引
	 * @return Bot名称
	 */
	FString CreateBotName(int32 PlayerIndex);
#endif
};
