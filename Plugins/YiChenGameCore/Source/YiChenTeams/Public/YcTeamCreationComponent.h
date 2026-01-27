// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Components/GameStateComponent.h"
#include "GameplayMessage/ScopedGameplayMessageListener.h"
#include "YcTeamCreationComponent.generated.h"

struct FGameModePlayerInitializedMessage;
struct FGameplayTag;
struct FExperienceLoadedMessage;
class UYcTeamAsset;
class AYcTeamPublicInfo;
class IYcTeamAgentInterface;

/**
 * 团队分配模式枚举
 */
UENUM(BlueprintType)
enum class EYcTeamAllocationMode : uint8
{
	/** 平衡分配到预定义队伍 (适用于团队对战模式如 TDM) */
	BalancedTeams UMETA(DisplayName = "Balanced Teams"),
	
	/** 每个玩家独立队伍 (适用于个人死亡竞赛 FFA) */
	FreeForAll UMETA(DisplayName = "Free For All"),
	
	/** 自定义分配逻辑 (通过蓝图或子类实现) */
	Custom UMETA(DisplayName = "Custom")
};

/**
 * 挂载在GameState上的队伍创建及分配组件
 * 支持多种团队分配模式:
 * - BalancedTeams: 平衡分配到预定义队伍 (TDM等团队模式)
 * - FreeForAll: 每个玩家独立队伍 (个人死亡竞赛)
 * - Custom: 自定义分配逻辑
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
	/** 团队分配模式 */
	UPROPERTY(EditDefaultsOnly, Category = Teams)
	EYcTeamAllocationMode AllocationMode = EYcTeamAllocationMode::BalancedTeams;
	
	/** 要创建的团队列表（团队ID到显示资源的映射，显示资源可以留空）
	 * 仅在 BalancedTeams 模式下使用 */
	UPROPERTY(EditDefaultsOnly, Category = Teams, meta = (EditCondition = "AllocationMode == EYcTeamAllocationMode::BalancedTeams", EditConditionHides))
	TMap<uint8, TObjectPtr<UYcTeamAsset>> TeamsToCreate;
	
	/** 队伍公开信息类 */
	UPROPERTY(EditDefaultsOnly, Category=Teams)
	TSubclassOf<AYcTeamPublicInfo> PublicTeamInfoClass;
	
	/** FFA 模式起始 TeamId，避免与预定义队伍冲突
	 * 建议设置为 100 或更大的值 */
	UPROPERTY(EditDefaultsOnly, Category = Teams, meta = (EditCondition = "AllocationMode == EYcTeamAllocationMode::FreeForAll", EditConditionHides))
	int32 FFAStartingTeamId = 100;
	
	/** FFA 模式是否为每个玩家创建 TeamInfo Actor
	 * 如果 UI 不需要显示团队信息，可以关闭以节省性能 */
	UPROPERTY(EditDefaultsOnly, Category = Teams, meta = (EditCondition = "AllocationMode == EYcTeamAllocationMode::FreeForAll", EditConditionHides))
	bool bCreateTeamInfoForFFAPlayers = false;
	
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
	 * 根据 AllocationMode 的不同采用不同的分配策略:
	 * - BalancedTeams: 分配到人数最少的预定义队伍
	 * - FreeForAll: 为每个玩家分配唯一的 TeamId
	 * - Custom: 调用 BP_CustomTeamAllocation 蓝图函数
	 * 如果是观众将会被分配为FGenericTeamId::NoTeam
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
	 * 由GameMode发起消息通知，从这里开始为新玩家分配队伍ID
	 * @param Channel 消息通道
	 * @param Message 消息体, 包含GameMode和新的Controller
	 */
	void OnPlayerInitialized(FGameplayTag Channel, const FGameModePlayerInitializedMessage& Message);
	
	/**
	 * 返回玩家数量最少的团队ID
	 * 仅在 BalancedTeams 模式下使用
	 * @return 玩家最少的TeamId，如果没有有效的Team则返回INDEX_NONE
	 */
	int32 GetLeastPopulatedTeamID() const;
	
	/**
	 * 为 FFA 模式分配唯一的 TeamId
	 * @return 新分配的唯一 TeamId
	 */
	int32 AllocateFFATeamId();
	
	/** FFA 模式的 TeamId 计数器，用于生成唯一 ID */
	int32 FFATeamIdCounter = 0;
#endif
	
	/**
	 * 自定义团队分配逻辑的蓝图可重载函数
	 * 仅在 AllocationMode 为 Custom 时调用
	 * @param PS 要分配团队的玩家状态
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = Teams, meta = (DisplayName = "Custom Team Allocation"))
	void K2_CustomTeamAllocation(APlayerState* PS);
};