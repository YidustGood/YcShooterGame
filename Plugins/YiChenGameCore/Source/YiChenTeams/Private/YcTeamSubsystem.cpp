// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcTeamSubsystem.h"

#include "AbilitySystemGlobals.h"
#include "GenericTeamAgentInterface.h"
#include "YcTeamAgentInterface.h"
#include "YcTeamInfoBase.h"
#include "YcTeamPublicInfo.h"
#include "YcTeamCheats.h"
#include "YiChenTeams.h"
#include "GameFramework/CheatManager.h"
#include "GameFramework/PlayerState.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcTeamSubsystem)



//////////////////////////////////////////////////////////////////////
// FYcTeamTrackingInfo

void FYcTeamTrackingInfo::SetTeamInfo(AYcTeamInfoBase* Info)
{
	if (AYcTeamPublicInfo* NewPublicInfo = Cast<AYcTeamPublicInfo>(Info))
	{
		// 确保公共信息为空或者是同一个对象
		ensure((PublicInfo == nullptr) || (PublicInfo == NewPublicInfo));
		PublicInfo = NewPublicInfo;

		// 检查团队资源是否发生变化
		UYcTeamAsset* OldTeamAsset = TeamAsset;
		TeamAsset = NewPublicInfo->GetTeamAsset();

		// 如果团队资源发生变化，广播变更事件
		if (OldTeamAsset != TeamAsset)
		{
			OnTeamAssetChanged.Broadcast(TeamAsset);
		}
	}
	else
	{
		checkf(false, TEXT("Expected a public or private team info but got %s"), *GetPathNameSafe(Info))
	}
}

void FYcTeamTrackingInfo::RemoveTeamInfo(AYcTeamInfoBase* Info)
{
	// 如果是要移除的公共信息，则清空引用
	if (PublicInfo == Info)
	{
		PublicInfo = nullptr;
	}
	else
	{
		ensureMsgf(false, TEXT("Expected a previously registered team info but got %s"), *GetPathNameSafe(Info));
	}
}

//////////////////////////////////////////////////////////////////////
// UYcTeamSubsystem

UYcTeamSubsystem::UYcTeamSubsystem()
{
	
}

void UYcTeamSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// 注册作弊扩展, 方便开发调试
	auto AddTeamCheats = [](UCheatManager* CheatManager)
	{
		CheatManager->AddCheatManagerExtension(NewObject<UYcTeamCheats>(CheatManager));
	};

	CheatManagerRegistrationHandle = UCheatManager::RegisterForOnCheatManagerCreated(FOnCheatManagerCreated::FDelegate::CreateLambda(AddTeamCheats));
}

void UYcTeamSubsystem::Deinitialize()
{
	// 注销作弊扩展
	UCheatManager::UnregisterFromOnCheatManagerCreated(CheatManagerRegistrationHandle);

	Super::Deinitialize();
}

UYcTeamSubsystem& UYcTeamSubsystem::Get(const UObject* WorldContextObject)
{
	// 从世界上下文对象获取世界实例
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	check(World);
	
	// 获取团队子系统实例
	UYcTeamSubsystem* Team = UWorld::GetSubsystem<UYcTeamSubsystem>(World);
	check(Team);
	
	return *Team;
}

bool UYcTeamSubsystem::RegisterTeamInfo(AYcTeamInfoBase* TeamInfo)
{
	if (!ensure(TeamInfo)) return false;
	
	// 有效则对这个队伍信息进行记录
	const int32 TeamId = TeamInfo->GetTeamId();
	if (ensure(TeamId != INDEX_NONE))
	{
		FYcTeamTrackingInfo& Entry = TeamMap.FindOrAdd(TeamId);
		Entry.SetTeamInfo(TeamInfo);

		return true;
	}

	return false;
}

bool UYcTeamSubsystem::UnregisterTeamInfo(AYcTeamInfoBase* TeamInfo)
{
	if (!ensure(TeamInfo))
	{
		return false;
	}

	const int32 TeamId = TeamInfo->GetTeamId();
	if (ensure(TeamId != INDEX_NONE))
	{
		FYcTeamTrackingInfo* Entry = TeamMap.Find(TeamId);

		// 如果找不到条目，可能是来自之前世界的残留Actor，忽略它
		if (Entry)
		{
			Entry->RemoveTeamInfo(TeamInfo);

			return true;
		}
	}

	return false;
}

bool UYcTeamSubsystem::DoesTeamExist(int32 TeamId) const
{
	return TeamMap.Contains(TeamId);
}

TArray<int32> UYcTeamSubsystem::GetTeamIDs() const
{
	TArray<int32> Result;
	// 从映射表中提取所有团队ID并排序
	TeamMap.GenerateKeyArray(Result);
	Result.Sort();
	return Result;
}

bool UYcTeamSubsystem::ChangeTeamForActor(AActor* ActorToChange, const int32 NewTeamId)
{
	// 将整数团队ID转换为通用团队ID
	const FGenericTeamId NewTeamID = IntegerToGenericTeamId(NewTeamId);
	
	TScriptInterface<IYcTeamAgentInterface> TeamAgent;
	// 查找Actor的团队代理接口，如果找到则设置新的团队ID
	if (FindTeamAgentFromActor(ActorToChange, TeamAgent))
	{
		TeamAgent->SetGenericTeamId(NewTeamID);
		return true;
	}
	
	return false;
}

bool UYcTeamSubsystem::FindTeamAgentFromActor(AActor* PossibleTeamActor, TScriptInterface<IYcTeamAgentInterface>& OutTeamAgent)
{
	if (PossibleTeamActor == nullptr) return false;
	
	{
		// 首先，检查Actor本身是否直接实现了队伍接口。
		TScriptInterface<IYcTeamAgentInterface> TeamAgent(PossibleTeamActor);
		if (TeamAgent)
		{
			OutTeamAgent = TeamAgent;
			return true;
		}
		
		// 其次，检查Actor组件是否实现了队伍接口。
		FindTeamAgentFromActorComponents(PossibleTeamActor, TeamAgent);
		if (TeamAgent)
		{
			OutTeamAgent = TeamAgent;
			return true;
		}
	}

	// 最后再尝试获取PlayerState, 因为推荐做法是放在PlayerState上管理玩家队伍信息
	APlayerState* PlayerState = nullptr;
	
	if (const APawn* Pawn = Cast<const APawn>(PossibleTeamActor))
	{
		PlayerState = Pawn->GetPlayerState<APlayerState>();
	}
	else if (const AController* PC = Cast<const AController>(PossibleTeamActor))
	{
		PlayerState = PC->PlayerState;
	}
	
	if (PlayerState)
	{
		TScriptInterface<IYcTeamAgentInterface> TeamAgent(PlayerState);
		if (TeamAgent)
		{
			OutTeamAgent = TeamAgent;
			return true;
		}
		
		// 检查PlayerState的组件是否实现了队伍接口。
		FindTeamAgentFromActorComponents(PlayerState, TeamAgent);
		if (TeamAgent)
		{
			OutTeamAgent = TeamAgent;
			return true;
		}
	}
	
	// 尝试从Instigator查找，例如手雷的Instigator就是抛出该手雷的玩家
	if (AActor* Instigator = PossibleTeamActor->GetInstigator())
	{
		if (ensure(Instigator != PossibleTeamActor))
		{
			return FindTeamAgentFromActor(Instigator, OutTeamAgent);
		}
	}
	
	return false;
}

bool UYcTeamSubsystem::FindTeamAgentFromActorComponents(AActor* PossibleTeamActor,
	TScriptInterface<IYcTeamAgentInterface>& OutTeamAgent)
{
	if (PossibleTeamActor == nullptr) return false;
	
	// 查找Actor的组件是否有实现了团队代理接口的
	UActorComponent* ActorComponent = PossibleTeamActor->FindComponentByInterface(UYcTeamAgentInterface::StaticClass());
	if (ActorComponent == nullptr) return false;
	
	const TScriptInterface<IYcTeamAgentInterface> TeamAgent(ActorComponent);
	if (TeamAgent)
	{
		OutTeamAgent = TeamAgent;
		return true;
	}
	
	return false;
}

int32 UYcTeamSubsystem::FindTeamFromObject(UObject* TestObject)
{
	// 检查对象是否直接实现了团队代理接口
	if (const IYcTeamAgentInterface* ObjectWithTeamInterface = Cast<IYcTeamAgentInterface>(TestObject))
	{
		return GenericTeamIdToInteger(ObjectWithTeamInterface->GetGenericTeamId());
	}

	if (AActor* TestActor = Cast<AActor>(TestObject))
	{
		// 从Instigator上查找队伍信息，例如一个手雷的Instigator就是抛出该手雷的玩家
		if (const IYcTeamAgentInterface* InstigatorWithTeamInterface = Cast<IYcTeamAgentInterface>(TestActor->GetInstigator()))
		{
			return GenericTeamIdToInteger(InstigatorWithTeamInterface->GetGenericTeamId());
		}

		// TeamInfo类型的Actor实际上没有实现团队接口，需要特殊处理
		if (const AYcTeamInfoBase* TeamInfo = Cast<AYcTeamInfoBase>(TestActor))
		{
			return TeamInfo->GetTeamId();
		}

		TScriptInterface<IYcTeamAgentInterface> TeamAgent;
		// 回退到查找关联的PlayerState
		if (FindTeamAgentFromActor(TestActor, TeamAgent))
		{
			return TeamAgent->GetTeamId();
		}
	}
	UE_LOG(LogYcTeamSystem, Warning, TEXT("通过%s对象寻找队伍信息失败."), *GetNameSafe(TestObject));
	return INDEX_NONE;
}

void UYcTeamSubsystem::FindTeamFromActor(AActor* TestActor, bool& bIsPartOfTeam, int32& TeamId) const
{
	TeamId = FindTeamFromObject(TestActor);
	bIsPartOfTeam = TeamId != INDEX_NONE;
}

EYcTeamComparison UYcTeamSubsystem::CompareTeamsOut(UObject* A, UObject* B, int32& TeamIdA, int32& TeamIdB) const
{
	// 查找两个对象的团队ID
	TeamIdA = FindTeamFromObject(A);
	TeamIdB = FindTeamFromObject(B);

	// 如果任一对象无效或不属于任何团队，返回无效参数
	if ((TeamIdA == INDEX_NONE) || (TeamIdB == INDEX_NONE))
	{
		return EYcTeamComparison::InvalidArgument;
	}
	else
	{
		// 根据团队ID是否相同返回相应的比较结果
		return (TeamIdA == TeamIdB) ? EYcTeamComparison::OnSameTeam : EYcTeamComparison::DifferentTeams;
	}
}

EYcTeamComparison UYcTeamSubsystem::CompareTeams(UObject* A, UObject* B) const
{
	int32 TeamIdA;
	int32 TeamIdB;
	return CompareTeamsOut(A, B, /*out*/ TeamIdA, /*out*/ TeamIdB);
}

bool UYcTeamSubsystem::CanCauseDamage(UObject* Instigator, UObject* Target, bool bAllowDamageToSelf) const
{
	// 如果允许对自己造成伤害，检查是否为同一对象或同一团队代理
	if (bAllowDamageToSelf)
	{
		TScriptInterface<IYcTeamAgentInterface> InstigatorTeamAgent;
		TScriptInterface<IYcTeamAgentInterface> TargetTeamAgent;
		FindTeamAgentFromActor(Cast<AActor>(Instigator), InstigatorTeamAgent);
		FindTeamAgentFromActor(Cast<AActor>(Target), TargetTeamAgent);
		if ((Instigator == Target) || (InstigatorTeamAgent != nullptr && InstigatorTeamAgent == TargetTeamAgent))
		{
			return true;
		}
	}
	
	// 比较两个对象的团队关系
	int32 InstigatorTeamId;
	int32 TargetTeamId;
	const EYcTeamComparison Relationship = CompareTeamsOut(Instigator, Target, /*out*/ InstigatorTeamId, /*out*/ TargetTeamId);
	
	// 如果属于不同团队，可以造成伤害
	if (Relationship == EYcTeamComparison::DifferentTeams)
	{
		return true;
	}
	// 如果伤害发起者存在有效队伍标记，而被伤害目标没有有效队伍标记，那么可以对其造成伤害
	else if ((Relationship == EYcTeamComparison::InvalidArgument) && (InstigatorTeamId != INDEX_NONE))
	{
		// 临时允许对非团队Actor造成伤害，只要它们有技能系统组件
		// @TODO: 这是临时方案，直到训练假人有了团队分配
		return UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Cast<const AActor>(Target)) != nullptr;
	}

	return false;	
}

void UYcTeamSubsystem::AddTeamTagStack(const int32 TeamId, const FGameplayTag Tag, const int32 StackCount)
{
	auto FailureHandler = [&](const FString& ErrorMessage)
	{
		UE_LOG(LogYcTeamSystem, Error, TEXT("AddTeamTagStack(TeamId: %d, Tag: %s, StackCount: %d) %s"), TeamId, *Tag.ToString(), StackCount, *ErrorMessage);
	};

	if (const FYcTeamTrackingInfo* Entry = TeamMap.Find(TeamId))
	{
		if (Entry->PublicInfo)
		{
			if (Entry->PublicInfo->HasAuthority())
			{
				Entry->PublicInfo->TeamTags.AddStack(Tag, StackCount);
			}
			else
			{
				FailureHandler(TEXT("failed because it was called on a client"));
			}
		}
		else
		{
			FailureHandler(TEXT("failed because there is no team info spawned yet (called too early, before the experience was ready)"));
		}
	}
	else
	{
		FailureHandler(TEXT("failed because it was passed an unknown team id"));
	}
}

void UYcTeamSubsystem::RemoveTeamTagStack(const int32 TeamId, const FGameplayTag Tag, const int32 StackCount)
{
	auto FailureHandler = [&](const FString& ErrorMessage)
	{
		UE_LOG(LogYcTeamSystem, Error, TEXT("RemoveTeamTagStack(TeamId: %d, Tag: %s, StackCount: %d) %s"), TeamId, *Tag.ToString(), StackCount, *ErrorMessage);
	};

	if (const FYcTeamTrackingInfo* Entry = TeamMap.Find(TeamId))
	{
		if (Entry->PublicInfo)
		{
			if (Entry->PublicInfo->HasAuthority())
			{
				Entry->PublicInfo->TeamTags.RemoveStack(Tag, StackCount);
			}
			else
			{
				FailureHandler(TEXT("failed because it was called on a client"));
			}
		}
		else
		{
			FailureHandler(TEXT("failed because there is no team info spawned yet (called too early, before the experience was ready)"));
		}
	}
	else
	{
		FailureHandler(TEXT("failed because it was passed an unknown team id"));
	}
}

int32 UYcTeamSubsystem::GetTeamTagStackCount(const int32 TeamId, const FGameplayTag Tag) const
{
	if (const FYcTeamTrackingInfo* Entry = TeamMap.Find(TeamId))
	{
		const int32 PublicStackCount = (Entry->PublicInfo != nullptr) ? Entry->PublicInfo->TeamTags.GetStackCount(Tag) : 0;
		return PublicStackCount;
	}
	else
	{
		UE_LOG(LogYcTeamSystem, Verbose, TEXT("GetTeamTagStackCount(TeamId: %d, Tag: %s) failed because it was passed an unknown team id"), TeamId, *Tag.ToString());
		return 0;
	}
}

bool UYcTeamSubsystem::TeamHasTag(const int32 TeamId, const FGameplayTag Tag) const
{
	return GetTeamTagStackCount(TeamId, Tag) > 0;
}