// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Interaction/Tasks/AbilityTask_GrantNearbyInteraction.h"

#include "AbilitySystemComponent.h"
#include "Interaction/YcInteractableTarget.h"
#include "Interaction/YcInteractionCvars.h"
#include "Interaction/YcInteractionStatics.h"
#include "Interaction/YcInteractionTypes.h"
#include "Physics/YcCollisionChannels.h"
#include "Engine/OverlapResult.h"



#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_GrantNearbyInteraction)

UAbilityTask_GrantNearbyInteraction::UAbilityTask_GrantNearbyInteraction(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UAbilityTask_GrantNearbyInteraction* UAbilityTask_GrantNearbyInteraction::GrantAbilitiesForNearbyInteractors(UGameplayAbility* OwningAbility, float InteractionScanRange, float InteractionScanRate)
{
	UAbilityTask_GrantNearbyInteraction* MyObj = NewAbilityTask<UAbilityTask_GrantNearbyInteraction>(OwningAbility);
	MyObj->InteractionScanRange = InteractionScanRange;
	MyObj->InteractionScanRate = InteractionScanRate;
	return MyObj;
}

void UAbilityTask_GrantNearbyInteraction::Activate()
{
	SetWaitingOnAvatar();

	UWorld* World = GetWorld();
	World->GetTimerManager().SetTimer(QueryTimerHandle, this, &ThisClass::QueryInteractables, InteractionScanRate, true);
}

void UAbilityTask_GrantNearbyInteraction::OnDestroy(bool AbilityEnded)
{
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(QueryTimerHandle);
	}

	Super::OnDestroy(AbilityEnded);
}

void UAbilityTask_GrantNearbyInteraction::QueryInteractables()
{
	UWorld* World = GetWorld();
	AActor* ActorOwner = GetAvatarActor();

	if (World == nullptr || ActorOwner == nullptr) return;
	
	FCollisionQueryParams Params(SCENE_QUERY_STAT(UAbilityTask_GrantNearbyInteraction), false);
#if !(UE_BUILD_TEST || UE_BUILD_SHIPPING)
	Params.bDebugQuery = true; // 避免再Shipping或Test中开启DebugQuery
#endif
		

	// 进行球形重叠检测获取玩家周围可交互目标接口
	TArray<FOverlapResult> OverlapResults;
	World->OverlapMultiByChannel(OUT OverlapResults, ActorOwner->GetActorLocation(), FQuat::Identity, Yc_TraceChannel_Interaction, FCollisionShape::MakeSphere(InteractionScanRange), Params);
	
	// 通过控制台命令控制是否绘制DebugSphere
#if ENABLE_DRAW_DEBUG
	if (YcConsoleVariables::CVarDrawDebugShape.GetValueOnGameThread())
	{
		FColor DebugColor = FColor::Green;
		DrawDebugSphere(World, ActorOwner->GetActorLocation(), InteractionScanRange, 16, DebugColor, false, InteractionScanRate);
	}
#endif // ENABLE_DRAW_DEBUG
	
	if (OverlapResults.Num() <= 0) return;
	
	// 从重叠结果中获取可交互目标接口
	TArray<TScriptInterface<IYcInteractableTarget>> InteractableTargets;
	UYcInteractionStatics::AppendInteractableTargetsFromOverlapResults(OverlapResults, OUT InteractableTargets);

	FYcInteractionQuery InteractionQuery;
	InteractionQuery.RequestingAvatar = ActorOwner;
	InteractionQuery.RequestingController = Cast<AController>(ActorOwner->GetOwner());

	TArray<FYcInteractionOption> Options;
	for (TScriptInterface<IYcInteractableTarget>& InteractiveTarget : InteractableTargets)
	{
		FYcInteractionOptionBuilder InteractionBuilder(InteractiveTarget, Options);
		InteractiveTarget->GatherInteractionOptions(InteractionQuery, InteractionBuilder);
	}

	// 检查这些交互选项是否需要在使用前先为玩家授予对应的能力
	for (FYcInteractionOption& Option : Options)
	{
		if (Option.InteractionAbilityToGrant)
		{
			// 向玩家的 AbilitySystem 组件授予该能力，否则无法执行交互逻辑
			FObjectKey ObjectKey(Option.InteractionAbilityToGrant);
			if (!InteractionAbilityCache.Find(ObjectKey)) // 缓存起来, 后续就不用重复授予了, 就算玩家走出了交互范围目前来说也没必要移除GA, 如果需要优化可以采用淘汰算法控制缓存总量
			{
				FGameplayAbilitySpec Spec(Option.InteractionAbilityToGrant, 1, INDEX_NONE, this);
				FGameplayAbilitySpecHandle Handle = AbilitySystemComponent->GiveAbility(Spec);
				InteractionAbilityCache.Add(ObjectKey, Handle);
			}
		}
	}
}