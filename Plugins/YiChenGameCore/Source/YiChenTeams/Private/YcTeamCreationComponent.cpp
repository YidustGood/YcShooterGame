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
#include "Misc/DataValidation.h"

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

	// 验证 BalancedTeams 模式下 TeamsToCreate 不为空
	if (AllocationMode == EYcTeamAllocationMode::BalancedTeams && TeamsToCreate.Num() == 0)
	{
		Context.AddError(FText::FromString(TEXT("BalancedTeams mode requires at least one team in TeamsToCreate")));
		Result = EDataValidationResult::Invalid;
	}

	// 验证 PublicTeamInfoClass 是否设置
	if (PublicTeamInfoClass == nullptr)
	{
		Context.AddWarning(FText::FromString(TEXT("PublicTeamInfoClass is not set")));
		if (Result == EDataValidationResult::Valid)
		{
			Result = EDataValidationResult::NotValidated;
		}
	}

	// 验证 FFA 模式下 FFAStartingTeamId 是否过低
	if (AllocationMode == EYcTeamAllocationMode::FreeForAll && FFAStartingTeamId < 10)
	{
		Context.AddWarning(FText::FromString(TEXT("FFAStartingTeamId < 10 may conflict with predefined teams")));
		if (Result == EDataValidationResult::Valid)
		{
			Result = EDataValidationResult::NotValidated;
		}
	}

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
	// 只在 BalancedTeams 模式下创建预定义的团队
	// FreeForAll 和 Custom 模式不需要预先创建团队
	if (AllocationMode != EYcTeamAllocationMode::BalancedTeams)
	{
		return;
	}
	
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
	
	// 观众始终分配为 NoTeam，不受分配模式影响
	if (PS->IsOnlyASpectator())
	{
		TeamAgent->SetGenericTeamId(FGenericTeamId::NoTeam);
		return;
	}
	
	// 根据分配模式选择不同的团队分配策略
	FGenericTeamId AssignedTeamId;
	
	switch (AllocationMode)
	{
		case EYcTeamAllocationMode::BalancedTeams:
		{
			// 平衡模式：分配到人数最少的预定义团队
			const int32 TeamId = GetLeastPopulatedTeamID();
			AssignedTeamId = IntegerToGenericTeamId(TeamId);
			break;
		}
		
		case EYcTeamAllocationMode::FreeForAll:
		{
			// FFA 模式：为每个玩家分配唯一的 TeamId
			const int32 TeamId = AllocateFFATeamId();
			AssignedTeamId = IntegerToGenericTeamId(TeamId);
			
			// 根据配置决定是否为 FFA 玩家创建 TeamInfo Actor
			if (bCreateTeamInfoForFFAPlayers)
			{
				ServerCreateTeam(TeamId, nullptr);
			}
			break;
		}
		
		case EYcTeamAllocationMode::Custom:
		{
			// 自定义模式：调用蓝图可重载函数处理分配逻辑
			K2_CustomTeamAllocation(PS);
			return; // 自定义逻辑负责设置 TeamId，直接返回
		}
	}
	
	// 设置分配的团队 ID
	TeamAgent->SetGenericTeamId(AssignedTeamId);
}

void UYcTeamCreationComponent::ServerCreateTeam(const int32 TeamId, UYcTeamAsset* TeamAsset) const
{
	check(HasAuthority());
	
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
	// 设置团队ID，这会触发自动注册到子系统
	NewTeamPublicInfo->SetTeamId(TeamId);
	
	// TeamAsset 可能为 nullptr（用于 FFA 模式）
	// 只有在 TeamAsset 不为空时才设置
	if (TeamAsset != nullptr)
	{
		NewTeamPublicInfo->SetTeamAsset(TeamAsset);
	}
	
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

int32 UYcTeamCreationComponent::AllocateFFATeamId()
{
	// 首次调用时，将计数器初始化为起始值
	if (FFATeamIdCounter == 0)
	{
		FFATeamIdCounter = FFAStartingTeamId;
	}
	
	// 返回当前值并递增，为下一个玩家准备
	return FFATeamIdCounter++;
}
#endif	// WITH_SERVER_CODE