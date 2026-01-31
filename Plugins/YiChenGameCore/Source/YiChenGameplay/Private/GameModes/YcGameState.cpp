// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "GameModes/YcGameState.h"

#include "YcAbilitySystemComponent.h"
#include "GameFramework/GameplayMessageSubsystem.h"
#include "GameModes/YcExperienceManagerComponent.h"
#include "GameplayCommon/YcGameVerbMessage.h"
#include "Player/YcPlayerState.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGameState)

AYcGameState::AYcGameState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	
	ExperienceManagerComponent = CreateDefaultSubobject<UYcExperienceManagerComponent>(TEXT("ExperienceManagerComponent"));
	
	AbilitySystemComponent = ObjectInitializer.CreateDefaultSubobject<UYcAbilitySystemComponent>(this, TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
}

void AYcGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, StatTags);
}

void AYcGameState::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	check(AbilitySystemComponent);
	AbilitySystemComponent->InitAbilityActorInfo(/*Owner=*/ this, /*Avatar=*/ this); // 初始化设置ASC的ActorInfo
}

UAbilitySystemComponent* AYcGameState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AYcGameState::MulticastMessageToClients_Implementation(const FYcGameVerbMessage Message)
{
	if (GetNetMode() == NM_Client)
	{
		UGameplayMessageSubsystem::Get(this).BroadcastMessage(Message.Verb, Message);
	}
}

void AYcGameState::MulticastReliableMessageToClients_Implementation(const FYcGameVerbMessage Message)
{
	MulticastMessageToClients_Implementation(Message);
}

void AYcGameState::AddStatTagStack(const FGameplayTag Tag, const int32 StackCount)
{
	StatTags.AddStack(Tag, StackCount);
}

void AYcGameState::RemoveStatTagStack(const FGameplayTag Tag, const int32 StackCount)
{
	StatTags.RemoveStack(Tag, StackCount);
}

void AYcGameState::SetStatTagStack(const FGameplayTag Tag, const int32 StackCount)
{
	// 先移除现有的堆栈（如果存在）
	const int32 CurrentCount = StatTags.GetStackCount(Tag);
	if (CurrentCount > 0)
	{
		StatTags.RemoveStack(Tag, CurrentCount);
	}
	
	// 添加新的堆栈数量
	if (StackCount > 0)
	{
		StatTags.AddStack(Tag, StackCount);
	}
}

int32 AYcGameState::GetStatTagStackCount(const FGameplayTag Tag) const
{
	return StatTags.GetStackCount(Tag);
}

bool AYcGameState::HasStatTag(const FGameplayTag Tag) const
{
	return StatTags.ContainsTag(Tag);
}

TArray<AYcPlayerState*> AYcGameState::GetPlayersSortedByTag(FGameplayTag SortTag, bool bDescending) const
{
	TArray<AYcPlayerState*> SortedPlayers;
	
	// 收集所有有效的 YcPlayerState
	for (APlayerState* PS : PlayerArray)
	{
		if (AYcPlayerState* YcPS = Cast<AYcPlayerState>(PS))
		{
			SortedPlayers.Add(YcPS);
		}
	}
	
	// 使用 C++ 标准库的快速排序
	// Lambda 表达式捕获 SortTag 和 bDescending
	SortedPlayers.Sort([SortTag, bDescending](const AYcPlayerState& A, const AYcPlayerState& B)
	{
		const int32 ScoreA = A.GetStatTagStackCount(SortTag);
		const int32 ScoreB = B.GetStatTagStackCount(SortTag);
		
		// 降序：A > B 返回 true（A 排在前面）
		// 升序：A < B 返回 true（A 排在前面）
		return bDescending ? (ScoreA > ScoreB) : (ScoreA < ScoreB);
	});
	
	return SortedPlayers;
}