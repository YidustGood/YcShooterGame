// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Phases/YcGamePhaseAbility_Playing.h"
#include "NativeGameplayTags.h"
#include "YiChenGamePhase.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGamePhaseAbility_Playing)

UE_DEFINE_GAMEPLAY_TAG(TAG_GamePhase_Playing, "GamePhase.Playing");

UYcGamePhaseAbility_Playing::UYcGamePhaseAbility_Playing()
{
	// 必须设置阶段标签, 蓝图中也可以设置
	GamePhaseTag = TAG_GamePhase_Playing;
}

void UYcGamePhaseAbility_Playing::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
	// 这里可以执行一些游戏游玩阶段正式开始后需要处理的事情
	UE_LOG(LogYcGamePhase, Log, TEXT("GamePhase: 游戏阶段<GamePhase.Playing>开始)"))
}

void UYcGamePhaseAbility_Playing::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
	// 这里可以执行一些游戏游玩阶段结束后需要处理的事情
	UE_LOG(LogYcGamePhase, Log, TEXT("GamePhase: 游戏阶段<GamePhase.Playing>结束)"))
}