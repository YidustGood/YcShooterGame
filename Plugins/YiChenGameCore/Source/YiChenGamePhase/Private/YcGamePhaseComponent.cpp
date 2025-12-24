// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcGamePhaseComponent.h"
#include "YcGamePhaseSubsystem.h"
#include "YiChenGamePhase.h"
#include "GameplayCommon/GameplayMessageTypes.h"
#include "GameFramework/GameplayMessageSubsystem.h"

#include "GameplayCommon/ExperienceMessageTypes.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGamePhaseComponent)

UYcGamePhaseComponent::UYcGamePhaseComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	// 该组件本身不需要Tick，仅通过消息和阶段切换驱动逻辑
	PrimaryComponentTick.bCanEverTick = false;
	// -1 表示当前还没有任何阶段被激活
	CurrentPhaseIndex = -1;
	SetIsReplicatedByDefault(true);
}

void UYcGamePhaseComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UYcGamePhaseComponent, CurrentPhaseIndex);
}

UYcGamePhaseComponent* UYcGamePhaseComponent::GetGamePhaseComponent(AGameStateBase* GameState)
{
	if (GameState == nullptr) return nullptr;
	return GameState->FindComponentByClass<UYcGamePhaseComponent>();
}

void UYcGamePhaseComponent::BeginPlay()
{
	Super::BeginPlay();
	
	// 仅在权威端处理游戏阶段流转
	if (GetOwner() && GetOwner()->HasAuthority())
	{
		// 监听游戏体验加载完成事件，体验加载完成后才开始阶段流转
		UGameplayMessageSubsystem& MessageSubsystem = UGameplayMessageSubsystem::Get(this);
		MessageSubsystem.RegisterListener(YcGameplayTags::Experience_StateEvent_Loaded_HighPriority, this, &ThisClass::OnExperienceLoaded);
		
		// @TODO 这里在实际发送消息时一定要注意在 OnExperienceLoaded 之后，否则阶段切换会过早；
		// @TODO 也可以在收到消息时判断体验是否加载完成，若未完成则缓存起来待体验加载完成后再处理，但目前认为没有强需求。
		
		// 为每个游戏阶段创建消息监听, 以接收处理开始某个游戏阶段
		// 不过请优先使用StartNextPhase()的方式进行顺序的游戏阶段轮转, 发送消息只是提供一个向前跳跃式轮转的途径
		for (const auto& Phase : GamePhases)
		{
			MessageSubsystem.RegisterListener(Phase.GamePhaseTag, this, &ThisClass::OnGamePhaseMessage);
		}
	}
}

void UYcGamePhaseComponent::OnGamePhaseMessage(FGameplayTag Channel, const FGamePhaseMessage& Message)
{
	// 根据消息所属的阶段标签获取目标阶段下标，用于防止阶段倒退或重复激活
	// 游戏阶段在一局对局中只允许单向向前推进
	int32 TargetIndex = GetPhaseIndex(Channel);
	if (TargetIndex <= CurrentPhaseIndex)
	{
		UE_LOG(LogYcGamePhase, Error, TEXT("接收到游戏阶段消息, 但是错误的倒退或者重复的阶段激活消息"));
		return;
	};
	
	const FYcGamePhaseDefinition* Phase = GetPhaseDefinition(Channel);
	if (!Phase)
	{
		UE_LOG(LogYcGamePhase, Error, TEXT("接收到游戏阶段消息, 但是未查找到有效的游戏阶段定义消息"));
		return;
	}
	auto& PhaseSubsystem = UYcGamePhaseSubsystem::Get(this);
	PhaseSubsystem.StartPhase(Phase->PhaseAbilityClass);
	
	// 更新当前游戏阶段下标；通过发送 GameplayMessage 切换阶段时是支持从当前阶段跳跃到更后面阶段的
	CurrentPhaseIndex = TargetIndex;
}

void UYcGamePhaseComponent::OnExperienceLoaded(FGameplayTag Channel, const FExperienceLoadedMessage& Message)
{
	// 游戏体验加载完成后自动开启第一个游戏阶段（从 GamePhases[0] 开始）
	StartNextPhase();
}

void UYcGamePhaseComponent::StartNextPhase()
{
	ensure(GetOwner()->HasAuthority() == true);
	
	// 确保存在下一阶段定义，且不会越界访问
	if (GamePhases.IsEmpty() || (CurrentPhaseIndex + 1) >= GamePhases.Num()) return;
	CurrentPhaseIndex++;
	auto& PhaseSubsystem = UYcGamePhaseSubsystem::Get(this);
	PhaseSubsystem.StartPhase(GamePhases[CurrentPhaseIndex].PhaseAbilityClass);
}

FGameplayTag UYcGamePhaseComponent::GetCurrentPhase()
{
	if (GamePhases.IsEmpty() || CurrentPhaseIndex >= GamePhases.Num() || CurrentPhaseIndex < 0) return FGameplayTag();
	return GamePhases[CurrentPhaseIndex].GamePhaseTag;
}

FYcGamePhaseDefinition* UYcGamePhaseComponent::GetPhaseDefinition(const FGameplayTag& PhaseTag)
{
	// 游戏阶段通常数量较少，使用线性遍历即可满足性能需求
	for (auto& Phase : GamePhases)
	{
		if (Phase.GamePhaseTag == PhaseTag) return &Phase;
	}
	return nullptr;
}

int32 UYcGamePhaseComponent::GetPhaseIndex(const FGameplayTag& PhaseTag)
{
	// 游戏阶段通常数量较少，使用线性遍历即可满足性能需求
	for (int i = 0; i < GamePhases.Num(); i++)
	{
		if (GamePhases[i].GamePhaseTag == PhaseTag) return i;
	}
	return -1;
}