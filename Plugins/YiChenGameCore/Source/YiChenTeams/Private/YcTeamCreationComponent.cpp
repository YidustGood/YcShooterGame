// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcTeamCreationComponent.h"

#include "GenericTeamAgentInterface.h"
#include "YcTeamAgentInterface.h"
#include "YcTeamPublicInfo.h"
#include "YcTeamSubsystem.h"
#include "YiChenTeams.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameFramework/PlayerState.h"
#include "GameplayCommon/ExperienceMessageTypes.h"
#include "GameplayCommon/GameplayMessageTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcTeamCreationComponent)

UYcTeamCreationComponent::UYcTeamCreationComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// 设置默认的团队公开信息类
	PublicTeamInfoClass = AYcTeamPublicInfo::StaticClass();
	// 启用使用注册子对象列表进行网络复制
	bReplicateUsingRegisteredSubObjectList = true;
}

#if WITH_EDITOR
EDataValidationResult UYcTeamCreationComponent::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	//@TODO: TEAMS: 验证所有队伍资产是否设置了相同的属性！

	return Result;
}
#endif

void UYcTeamCreationComponent::BeginPlay()
{
	Super::BeginPlay();

	// 监听经验加载完成事件
	UGameplayMessageSubsystem& Subsystem = UGameplayMessageSubsystem::Get(this);
	Subsystem.RegisterListener(YcGameplayTags::Experience_StateEvent_Loaded_HighPriority, this, &ThisClass::OnExperienceLoaded);
}

void UYcTeamCreationComponent::OnExperienceLoaded(FGameplayTag Channel, const FExperienceLoadedMessage& Message)
{
	// 创建队伍的逻辑仅在服务器端执行
#if WITH_SERVER_CODE
	if (HasAuthority())
	{
		// 先创建所有配置的队伍
		ServerCreateTeams();
		// 然后为当前已加入的玩家分配团队
		ServerAssignPlayersToTeams();
	}
#endif
}

#if WITH_SERVER_CODE

void UYcTeamCreationComponent::ServerCreateTeams()
{
	// 遍历配置的团队列表，逐个创建团队
	for (const auto& KVP : TeamsToCreate)
	{
		const int32 TeamId = KVP.Key;
		ServerCreateTeam(TeamId, KVP.Value);
	}
}

void UYcTeamCreationComponent::ServerAssignPlayersToTeams()
{
	// 为已存在的玩家分配团队
	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	for (APlayerState* PS : GameState->PlayerArray)
	{
		ServerChooseTeamForPlayer(PS);
	}
	
	// 监听新的玩家/AI加入游戏并初始化完Controller的消息, 以便为新玩家/AI分配团队ID
	UGameplayMessageSubsystem& Subsystem = UGameplayMessageSubsystem::Get(this);
	Subsystem.RegisterListener(YcGameplayTags::Gameplay_GameMode_PlayerInitialized, this, &ThisClass::OnPlayerInitialized);
}

void UYcTeamCreationComponent::ServerChooseTeamForPlayer(APlayerState* PS)
{
	// 检查PlayerState是否实现了团队代理接口
	TScriptInterface<IYcTeamAgentInterface> TeamAgent(PS);
	UYcTeamSubsystem::FindTeamAgentFromActor(PS, TeamAgent);
	if (!TeamAgent)
	{
		UE_LOG(LogYcTeamSystem, Warning, TEXT("为玩家选择队伍失败, 请确保PlayerState实现了IYcTeamAgentInterface接口!"));
		return;
	}
	
	// 如果是观众，设置为无团队
	if (PS->IsOnlyASpectator())
	{
		TeamAgent->SetGenericTeamId(FGenericTeamId::NoTeam);
	}
	else
	{
		// 为正常玩家分配到人数最少的团队，以保持团队平衡
		const FGenericTeamId TeamID = IntegerToGenericTeamId(GetLeastPopulatedTeamID());
		TeamAgent->SetGenericTeamId(TeamID);
	}
}

void UYcTeamCreationComponent::ServerCreateTeam(const int32 TeamId, UYcTeamAsset* TeamAsset) const
{
	check(HasAuthority());
	
	// 确保团队资源资产不为空
	ensure(TeamAsset != nullptr);
	const UYcTeamSubsystem& TeamSubsystem = UYcTeamSubsystem::Get(this);
	// 确保团队ID不重复
	ensureMsgf(!TeamSubsystem.DoesTeamExist(TeamId), TEXT("队伍重复创建!"));

	UWorld* World = GetWorld();
	check(World);

	// 配置Actor生成参数，确保总是能够生成
	FActorSpawnParameters SpawnInfo;
	SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	// 生成团队公开信息Actor
	AYcTeamPublicInfo* NewTeamPublicInfo = World->SpawnActor<AYcTeamPublicInfo>(PublicTeamInfoClass, SpawnInfo);
	checkf(NewTeamPublicInfo != nullptr, TEXT("Failed to create public team actor from class %s"), *GetPathNameSafe(*PublicTeamInfoClass));
	// 设置团队ID和资源资产，这会触发自动注册到子系统
	NewTeamPublicInfo->SetTeamId(TeamId);
	NewTeamPublicInfo->SetTeamAsset(TeamAsset);
	
	// @TODO 创建队伍私有信息对象
}

void UYcTeamCreationComponent::OnPlayerInitialized(FGameplayTag Channel,
	const FGameModePlayerInitializedMessage& Message)
{
	check(Message.NewPlayer);
	check(Message.NewPlayer->PlayerState);
	// 为新玩家选择并分配团队
	ServerChooseTeamForPlayer(Message.NewPlayer->PlayerState);
}

int32 UYcTeamCreationComponent::GetLeastPopulatedTeamID() const
{
	const int32 NumTeams = TeamsToCreate.Num();
	if (NumTeams == 0) return INDEX_NONE;
	
	TMap<int32, uint32> TeamMemberCounts;
	TeamMemberCounts.Reserve(NumTeams);

	for (const auto& KVP : TeamsToCreate)
	{
		const int32 TeamId = KVP.Key;
		TeamMemberCounts.Add(TeamId, 0);
	}

	// 通过GameState拿到当前所有的PlayerState然后进行各团队人数统计
	AGameStateBase* GameState = GetGameStateChecked<AGameStateBase>();
	TScriptInterface<IYcTeamAgentInterface> TeamAgent;
	for (APlayerState* PS : GameState->PlayerArray)
	{
		UYcTeamSubsystem::FindTeamAgentFromActor(PS, TeamAgent);
		if (TeamAgent == nullptr) continue;
		
		const int32 PlayerTeamID = TeamAgent->GetTeamId();

		if ((PlayerTeamID != INDEX_NONE) && !PS->IsInactive())	//  不计算未分配角色或处于离线状态的玩家
		{
			check(TeamMemberCounts.Contains(PlayerTeamID))
			TeamMemberCounts[PlayerTeamID] += 1;
		}
	}
	
	// 按照团队人数从少到多的顺序排列，然后按照团队编号进行排序。
	int32 BestTeamId = INDEX_NONE;
	uint32 BestPlayerCount = TNumericLimits<uint32>::Max();
	for (const auto& KVP : TeamMemberCounts)
	{
		const int32 TestTeamId = KVP.Key;
		const uint32 TestTeamPlayerCount = KVP.Value;

		if (TestTeamPlayerCount < BestPlayerCount)
		{
			BestTeamId = TestTeamId;
			BestPlayerCount = TestTeamPlayerCount;
		}
		else if (TestTeamPlayerCount == BestPlayerCount)
		{
			if ((TestTeamId < BestTeamId) || (BestTeamId == INDEX_NONE))
			{
				BestTeamId = TestTeamId;
				BestPlayerCount = TestTeamPlayerCount;
			}
		}
	}
	
	return BestTeamId;
}
#endif	// WITH_SERVER_CODE