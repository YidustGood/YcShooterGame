// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "YcGamePhaseSubsystem.h"

#include "GameplayAbilitySpec.h"
#include "YcAbilitySystemComponent.h"
#include "YcGamePhaseAbility.h"
#include "YiChenGamePhase.h"
#include "GameFramework/GameStateBase.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(YcGamePhaseSubsystem)

UYcGamePhaseSubsystem::UYcGamePhaseSubsystem()
{
}

UYcGamePhaseSubsystem& UYcGamePhaseSubsystem::Get(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::Assert);
	check(World);
	
	UYcGamePhaseSubsystem* Subsystem = World->GetSubsystem<UYcGamePhaseSubsystem>();
	check(Subsystem);
	return *Subsystem;
}

void UYcGamePhaseSubsystem::PostInitialize()
{
	Super::PostInitialize();
}

bool UYcGamePhaseSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return Super::ShouldCreateSubsystem(Outer);
}

bool UYcGamePhaseSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	// 仅支持Game和PIE两种World类型, 其它World类型通常来说并不需要
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

void UYcGamePhaseSubsystem::StartPhase(const TSubclassOf<UYcGamePhaseAbility> PhaseAbility, const FYcGamePhaseDelegate& PhaseEndedCallback)
{
	if (PhaseAbility == nullptr)
	{
		UE_LOG(LogYcGamePhase, Error, TEXT("开始了一个无效的PhaseAbility!"));
		return;
	}
	
	UWorld* World = GetWorld();
	UAbilitySystemComponent* GameState_ASC = World->GetGameState()->FindComponentByClass<UAbilitySystemComponent>();
	if (!ensure(GameState_ASC)) return;
	
	// 将Phase技能赋予GameState的ASC组件并激活
	FGameplayAbilitySpec PhaseSpec(PhaseAbility, 1, 0, this);
	// 授予游戏阶段技能并尝试单次激活，激活后该技能将被移除，此操作仅在服务器端有效，且技能的“网络执行策略”不能设置为“仅本地”或“本地预测”。
	FGameplayAbilitySpecHandle SpecHandle = GameState_ASC->GiveAbilityAndActivateOnce(PhaseSpec);
	FGameplayAbilitySpec* FoundSpec = GameState_ASC->FindAbilitySpecFromHandle(SpecHandle);
	
	// 如果阶段技能成功授予并成功激活, 那么就记录它, 并且绑定结束时的委托
	if (FoundSpec && FoundSpec->IsActive())
	{
		FYcGamePhaseEntry& Entry = ActivePhaseMap.FindOrAdd(SpecHandle);
		Entry.PhaseEndedCallback = PhaseEndedCallback;
	}
	else
	{
		// 如果未成功授予或激活那么直接执行回调委托
		PhaseEndedCallback.ExecuteIfBound(nullptr);
	}
}

void UYcGamePhaseSubsystem::RegisterPhaseStartedObserver(FGameplayTag PhaseTag, EPhaseTagMatchType MatchType,
	const FYcGamePhaseTagDelegate& OnPhaseStarted)
{
	FPhaseObserver Observer;
	Observer.PhaseTag = PhaseTag;
	Observer.MatchType = MatchType;
	Observer.PhaseCallback = OnPhaseStarted;
	PhaseStartObservers.Add(Observer);

	if (IsPhaseActive(PhaseTag))
	{
		OnPhaseStarted.ExecuteIfBound(PhaseTag);
	}
}

void UYcGamePhaseSubsystem::RegisterPhaseEndedObserver(FGameplayTag PhaseTag, EPhaseTagMatchType MatchType,
	const FYcGamePhaseTagDelegate& OnPhaseEnded)
{
	FPhaseObserver Observer;
	Observer.PhaseTag = PhaseTag;
	Observer.MatchType = MatchType;
	Observer.PhaseCallback = OnPhaseEnded;
	PhaseEndObservers.Add(Observer);
}

bool UYcGamePhaseSubsystem::IsPhaseActive(const FGameplayTag& PhaseTag) const
{
	for (const auto& KVP : ActivePhaseMap)
	{
		const FYcGamePhaseEntry& PhaseEntry = KVP.Value;
		if (PhaseEntry.PhaseTag.MatchesTag(PhaseTag))
		{
			return true;
		}
	}

	return false;
}

void UYcGamePhaseSubsystem::K2_StartPhase(TSubclassOf<UYcGamePhaseAbility> PhaseAbility, const FYcGamePhaseDynamicDelegate& PhaseEndedDelegate)
{
	const FYcGamePhaseDelegate EndedDelegate = FYcGamePhaseDelegate::CreateWeakLambda(const_cast<UObject*>(PhaseEndedDelegate.GetUObject()), [PhaseEndedDelegate](const UYcGamePhaseAbility* PhaseAbility) {
		PhaseEndedDelegate.ExecuteIfBound(PhaseAbility);
	});

	StartPhase(PhaseAbility, EndedDelegate);
}

void UYcGamePhaseSubsystem::K2_RegisterPhaseStartedObserver(FGameplayTag PhaseTag, EPhaseTagMatchType MatchType, FYcGamePhaseTagDynamicDelegate OnPhaseStarted)
{
	const FYcGamePhaseTagDelegate ActiveDelegate = FYcGamePhaseTagDelegate::CreateWeakLambda(OnPhaseStarted.GetUObject(), [OnPhaseStarted](const FGameplayTag& PhaseTag) {
		OnPhaseStarted.ExecuteIfBound(PhaseTag);
	});

	RegisterPhaseStartedObserver(PhaseTag, MatchType, ActiveDelegate);
}

void UYcGamePhaseSubsystem::K2_RegisterPhaseEndedObserver(FGameplayTag PhaseTag, EPhaseTagMatchType MatchType, FYcGamePhaseTagDynamicDelegate OnPhaseEnded)
{
	const FYcGamePhaseTagDelegate EndedDelegate = FYcGamePhaseTagDelegate::CreateWeakLambda(OnPhaseEnded.GetUObject(), [OnPhaseEnded](const FGameplayTag& PhaseTag) {
		OnPhaseEnded.ExecuteIfBound(PhaseTag);
	});

	RegisterPhaseEndedObserver(PhaseTag, MatchType, EndedDelegate);
}

void UYcGamePhaseSubsystem::OnBeginPhase(const UYcGamePhaseAbility* PhaseAbility, const FGameplayAbilitySpecHandle PhaseAbilityHandle)
{
	const FGameplayTag IncomingPhaseTag = PhaseAbility->GetGamePhaseTag();

	UE_LOG(LogYcGamePhase, Log, TEXT("Beginning Phase '%s' (%s)"), *IncomingPhaseTag.ToString(), *GetNameSafe(PhaseAbility));

	const UWorld* World = GetWorld();
	UYcAbilitySystemComponent* GameState_ASC = World->GetGameState()->FindComponentByClass<UYcAbilitySystemComponent>();
	if (!ensure(GameState_ASC)) return;
	
	TArray<FGameplayAbilitySpec*> ActivePhases;
	for (const auto& KVP : ActivePhaseMap)
	{
		const FGameplayAbilitySpecHandle ActiveAbilityHandle = KVP.Key;
		if (FGameplayAbilitySpec* Spec = GameState_ASC->FindAbilitySpecFromHandle(ActiveAbilityHandle))
		{
			ActivePhases.Add(Spec);
		}
	}

	// 将不属于IncomingPhaseTag子标签的游戏阶段都终止
	for (const FGameplayAbilitySpec* ActivePhase : ActivePhases)
	{
		const UYcGamePhaseAbility* ActivePhaseAbility = CastChecked<UYcGamePhaseAbility>(ActivePhase->Ability);
		const FGameplayTag ActivePhaseTag = ActivePhaseAbility->GetGamePhaseTag();
			
		// 如果当前激活的阶段标签与即将激活的阶段标签匹配，则允许其继续存在
		// 即：可以有多个游戏能力关联到同一个阶段标签并同时激活
		// 例如：处于 Game.Playing 阶段时，可以启动子阶段 Game.Playing.SuddenDeath，Game.Playing 仍保持激活
		// 如果再启动 Game.Playing.ActualSuddenDeath，则会结束 Game.Playing.SuddenDeath，但 Game.Playing 依旧存在
		// 同理，如果激活 Game.GameOver，则所有 Game.Playing* 阶段都会被结束
		if (!IncomingPhaseTag.MatchesTag(ActivePhaseTag))
		{
			UE_LOG(LogYcGamePhase, Log, TEXT("\tEnding Phase '%s' (%s)"), *ActivePhaseTag.ToString(), *GetNameSafe(ActivePhaseAbility));

			FGameplayAbilitySpecHandle HandleToEnd = ActivePhase->Handle;
			GameState_ASC->CancelAbilitiesByFunc([HandleToEnd](const UYcGameplayAbility* Ability, FGameplayAbilitySpecHandle Handle) {
				return Handle == HandleToEnd;
			}, true);
		}
	}
	
	// 保存记录本次新激活的游戏阶段信息
	FYcGamePhaseEntry& Entry = ActivePhaseMap.FindOrAdd(PhaseAbilityHandle);
	Entry.PhaseTag = IncomingPhaseTag;

	// 通知此阶段的所有观察者,该阶段已开始
	for (const FPhaseObserver& Observer : PhaseStartObservers)
	{
		if (!Observer.IsMatch(IncomingPhaseTag)) continue;
		
		Observer.PhaseCallback.ExecuteIfBound(IncomingPhaseTag);
	}
}

void UYcGamePhaseSubsystem::OnEndPhase(const UYcGamePhaseAbility* PhaseAbility, const FGameplayAbilitySpecHandle PhaseAbilityHandle)
{
	const FGameplayTag EndedPhaseTag = PhaseAbility->GetGamePhaseTag();
	UE_LOG(LogYcGamePhase, Log, TEXT("Ended Phase '%s' (%s)"), *EndedPhaseTag.ToString(), *GetNameSafe(PhaseAbility));

	const FYcGamePhaseEntry& Entry = ActivePhaseMap.FindChecked(PhaseAbilityHandle);
	Entry.PhaseEndedCallback.ExecuteIfBound(PhaseAbility);

	ActivePhaseMap.Remove(PhaseAbilityHandle);

	// 通知此阶段的所有观察者，该阶段已结束
	for (const FPhaseObserver& Observer : PhaseEndObservers)
	{
		if (!Observer.IsMatch(EndedPhaseTag)) continue;
		
		Observer.PhaseCallback.ExecuteIfBound(EndedPhaseTag);
	}
}

bool UYcGamePhaseSubsystem::FPhaseObserver::IsMatch(const FGameplayTag& ComparePhaseTag) const
{
	switch(MatchType)
	{
	case EPhaseTagMatchType::ExactMatch:
		return ComparePhaseTag == PhaseTag;
	case EPhaseTagMatchType::PartialMatch:
		return ComparePhaseTag.MatchesTag(PhaseTag);
	}

	return false;
}