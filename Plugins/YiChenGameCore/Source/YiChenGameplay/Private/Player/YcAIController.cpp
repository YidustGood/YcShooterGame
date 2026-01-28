// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Player/YcAIController.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "YiChenGameplay.h"
#include "GameFramework/PlayerState.h"
#include "GameModes/YcGameMode.h"
#include "Perception/AIPerceptionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcAIController)

AYcAIController::AYcAIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// AI控制器需要PlayerState来管理队伍信息
	bWantsPlayerState = true;
	// 解除控制时不停止AI逻辑（允许AI在失去Pawn后继续运行）
	bStopAILogicOnUnposses = false;
}

void AYcAIController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	// 禁止直接设置Bot的TeamID，因为Bot的TeamID由其关联的PlayerState决定
	// 这样可以确保队伍信息的一致性和网络同步的正确性
	UE_LOG(LogYcGameplay, Error, TEXT("You can't set the team ID on a player bot controller (%s); it's driven by the associated player state"), *GetPathNameSafe(this));
}

FGenericTeamId AYcAIController::GetGenericTeamId() const
{
	// 从Bot控制的PlayerState中获取TeamID
	// 这样可以确保AI Bot和玩家使用相同的队伍管理机制
	if (const IYcTeamAgentInterface* PSWithTeamInterface = Cast<IYcTeamAgentInterface>(PlayerState))
	{
		return PSWithTeamInterface->GetGenericTeamId();
	}
	return FGenericTeamId::NoTeam;	
}

FOnYcTeamIndexChangedDelegate* AYcAIController::GetOnTeamIndexChangedDelegate()
{
	return &OnTeamChangedDelegate;
}

ETeamAttitude::Type AYcAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	// 获取目标Pawn
	const APawn* OtherPawn = Cast<APawn>(&Other);
	if (OtherPawn == nullptr) return ETeamAttitude::Neutral;
	
	// 获取目标的队伍代理接口
	const IYcTeamAgentInterface* TeamAgent = Cast<IYcTeamAgentInterface>(OtherPawn->GetController());
	if (TeamAgent == nullptr) return ETeamAttitude::Neutral;

	FGenericTeamId OtherTeamID = TeamAgent->GetGenericTeamId();

	// 根据队伍ID判断态度：不同队伍为敌对，相同队伍为友好
	if (OtherTeamID.GetId() != GetGenericTeamId().GetId())
	{
		return ETeamAttitude::Hostile;
	}
	else
	{
		return ETeamAttitude::Friendly;
	}
}

void AYcAIController::ServerRestartController()
{
	// 只在服务器端执行
	if (GetNetMode() == NM_Client)
	{
		return;
	}
	
	// 确保控制器处于正确的状态（无Pawn且处于非活动状态）
	ensure((GetPawn() == nullptr) && IsInState(NAME_Inactive));
	if (IsInState(NAME_Inactive) || (IsInState(NAME_Spectating)))
	{
		AYcGameMode* const GameMode = GetWorld()->GetAuthGameMode<AYcGameMode>();

		// 检查GameMode是否允许重启
		if ((GameMode == nullptr) || !GameMode->ControllerCanRestart(this))
		{
			return;
		}

		// 如果仍然控制着Pawn，先解除控制
		if (GetPawn() != nullptr)
		{
			UnPossess();
		}

		// 重新启用输入（类似ClientRestart中的代码）
		ResetIgnoreInputFlags();

		// 通过GameMode重启玩家（重新生成Pawn等）
		GameMode->RestartPlayer(this);
	}
	
}

void AYcAIController::UpdateTeamAttitude(UAIPerceptionComponent* AIPerception)
{
	// 请求更新AI感知组件的刺激监听器
	// 这会使队伍变更后的感知配置立即生效
	if (AIPerception)
	{
		AIPerception->RequestStimuliListenerUpdate();
	}
}

void AYcAIController::OnUnPossess()
{
	// 确保被解除控制的Pawn不再作为ASC的Avatar Actor
	// 这对于GAS系统的正确运作很重要，避免ASC引用已经不受控制的Pawn
	if (APawn* PawnBeingUnpossessed = GetPawn())
	{
		if (UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(PlayerState))
		{
			if (ASC->GetAvatarActor() == PawnBeingUnpossessed)
			{
				ASC->SetAvatarActor(nullptr);
			}
		}
	}
	Super::OnUnPossess();
}

void AYcAIController::OnPlayerStateChangedTeam(UObject* TeamAgent, int32 OldTeam, int32 NewTeam)
{
	// 当PlayerState的队伍发生变化时，广播队伍变更事件
	ConditionalBroadcastTeamChanged(this, IntegerToGenericTeamId(OldTeam), IntegerToGenericTeamId(NewTeam));
}

void AYcAIController::OnPlayerStateChanged()
{
	// 空实现，供派生类重写以响应PlayerState变更
	// 派生类可以在此处添加自定义逻辑，而无需挂钩所有其他事件
}

void AYcAIController::BroadcastOnPlayerStateChanged()
{
	// 通知派生类PlayerState已变更
	OnPlayerStateChanged();
	
	// 从旧的PlayerState解绑队伍变更委托
	FGenericTeamId OldTeamID = FGenericTeamId::NoTeam;
	if (LastSeenPlayerState != nullptr)
	{
		if (IYcTeamAgentInterface* PlayerStateTeamInterface = Cast<IYcTeamAgentInterface>(LastSeenPlayerState))
		{
			OldTeamID = PlayerStateTeamInterface->GetGenericTeamId();
			PlayerStateTeamInterface->GetTeamChangedDelegateChecked().RemoveAll(this);
		}
	}

	// 绑定到新的PlayerState的队伍变更委托
	FGenericTeamId NewTeamID = FGenericTeamId::NoTeam;
	if (PlayerState != nullptr)
	{
		if (IYcTeamAgentInterface* PlayerStateTeamInterface = Cast<IYcTeamAgentInterface>(PlayerState))
		{
			NewTeamID = PlayerStateTeamInterface->GetGenericTeamId();
			PlayerStateTeamInterface->GetTeamChangedDelegateChecked().AddDynamic(this, &ThisClass::OnPlayerStateChangedTeam);
		}
	}

	// 如果队伍真的发生了变化，广播队伍变更事件
	ConditionalBroadcastTeamChanged(this, OldTeamID, NewTeamID);

	// 更新记录的PlayerState
	LastSeenPlayerState = PlayerState;
}

void AYcAIController::InitPlayerState()
{
	Super::InitPlayerState();
	BroadcastOnPlayerStateChanged();
}

void AYcAIController::CleanupPlayerState()
{
	Super::CleanupPlayerState();
	BroadcastOnPlayerStateChanged();
}

void AYcAIController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	BroadcastOnPlayerStateChanged();
}
