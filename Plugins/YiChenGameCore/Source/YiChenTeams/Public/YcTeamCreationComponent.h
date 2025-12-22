// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Components/GameStateComponent.h"
#include "GameplayMessage/ScopedGameplayMessageListener.h"
#include "YcTeamCreationComponent.generated.h"

struct FGameplayTag;
struct FExperienceLoadedMessage;
class UYcTeamAsset;
class AYcTeamPublicInfo;
class IYcTeamAgentInterface;

/**
 * 挂载在GameState上的队伍创建及分配组件,目前的分配机制是以平衡双方人数来进行自动分配
 * 可以配置TeamsToCreate来指定游戏对局要创建的队伍信息,支持创建多个不同队伍
 */
UCLASS(ClassGroup=(YiChenTeams), meta=(BlueprintSpawnableComponent))
class YICHENTEAMS_API UYcTeamCreationComponent : public UGameStateComponent
{
	GENERATED_BODY()
public:
	UYcTeamCreationComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	//~UObject interface
#if WITH_EDITOR
	/**
	 * 验证数据有效性
	 * @param Context 数据验证上下文
	 * @return 数据验证结果
	 */
	virtual EDataValidationResult IsDataValid(class FDataValidationContext& Context) const override;
#endif
	//~End of UObject interface

	//~UActorComponent interface
	virtual void BeginPlay() override;
	//~End of UActorComponent interface

private:
	/**
	 * Experience加载完毕后的回调
	 * 将在这里创建队伍并为当前已经在对局中的玩家分配队伍ID
	 * @param Channel 消息通道
	 * @param Message 经验加载消息
	 */
	void OnExperienceLoaded(FGameplayTag Channel, const FExperienceLoadedMessage& Message);
	
protected:
	/** 要创建的团队列表（团队ID到显示资源的映射，显示资源可以留空） */
	UPROPERTY(EditDefaultsOnly, Category = Teams)
	TMap<uint8, TObjectPtr<UYcTeamAsset>> TeamsToCreate;
	
	/** 队伍公开信息类 */
	UPROPERTY(EditDefaultsOnly, Category=Teams)
	TSubclassOf<AYcTeamPublicInfo> PublicTeamInfoClass;
	
#if WITH_SERVER_CODE
protected:
	/**
	 * 在服务器端创建所有配置的队伍
	 */
	virtual void ServerCreateTeams();
	
	/**
	 * 在服务器端为当前已加入对局的玩家分配TeamId
	 */
	virtual void ServerAssignPlayersToTeams();

	/**
	 * 为指定的玩家状态设置团队ID
	 * 如果是观众将会被分配为FGenericTeamId::NoTeam，正常玩家则调用GetLeastPopulatedTeamID()获取一个TeamId分配给玩家
	 * 可以通过子组件重写这个函数定制玩家的队伍分配逻辑
	 * @param PS 要分配团队的玩家状态
	 */
	virtual void ServerChooseTeamForPlayer(APlayerState* PS);

private:
	/**
	 * 在服务器端创建单个团队
	 * @param TeamId 要创建的团队ID
	 * @param TeamAsset 团队资源资产
	 */
	void ServerCreateTeam(int32 TeamId, UYcTeamAsset* TeamAsset) const;
	
	/**
	 * 新玩家加入时的回调
	 * 由GameMode发起，从这里开始为新玩家分配队伍ID
	 * @param GameMode 游戏模式
	 * @param NewPlayer 新加入的玩家控制器
	 */
	void OnPlayerInitialized(AGameModeBase* GameMode, const AController* NewPlayer);
	
	/**
	 * 返回玩家数量最少的团队ID
	 * @return 玩家最少的TeamId，如果没有有效的Team则返回INDEX_NONE
	 */
	int32 GetLeastPopulatedTeamID() const;
#endif
};