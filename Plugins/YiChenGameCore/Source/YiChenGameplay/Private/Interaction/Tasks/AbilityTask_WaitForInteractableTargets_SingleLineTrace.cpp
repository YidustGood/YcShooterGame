// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Interaction/Tasks/AbilityTask_WaitForInteractableTargets_SingleLineTrace.h"

#include "Interaction/YcInteractionCvars.h"
#include "Interaction/YcInteractionStatics.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_WaitForInteractableTargets_SingleLineTrace)

// ==================== 性能计数器声明 ====================
DECLARE_CYCLE_STAT(TEXT("PerformTrace"), STAT_YcInteraction_PerformTrace, STATGROUP_YcInteraction);

UAbilityTask_WaitForInteractableTargets_SingleLineTrace* UAbilityTask_WaitForInteractableTargets_SingleLineTrace::WaitForInteractableTargets_SingleLineTrace(UGameplayAbility* OwningAbility,
	FYcInteractionQuery InteractionQuery, FCollisionProfileName TraceProfile, FGameplayAbilityTargetingLocationInfo StartLocation, float InteractionScanLength, float InteractionScanRate)
{
	UAbilityTask_WaitForInteractableTargets_SingleLineTrace* MyObj = NewAbilityTask<UAbilityTask_WaitForInteractableTargets_SingleLineTrace>(OwningAbility);
	MyObj->InteractionScanLength = InteractionScanLength;
	MyObj->InteractionScanRate = InteractionScanRate;
	MyObj->StartLocation = StartLocation;
	MyObj->InteractionQuery = InteractionQuery;
	MyObj->TraceProfile = TraceProfile;

	return MyObj;
}

void UAbilityTask_WaitForInteractableTargets_SingleLineTrace::Activate()
{
	SetWaitingOnAvatar();

	UWorld* World = GetWorld();
	check(World);
	// 设置Timer循环调用PerformTrace()进行射线检测
	World->GetTimerManager().SetTimer(TimerHandle, this, &ThisClass::PerformTrace, InteractionScanRate, true);
}

void UAbilityTask_WaitForInteractableTargets_SingleLineTrace::OnDestroy(bool AbilityEnded)
{
	// 当被销毁时候清理射线检测Timer
	if (const UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TimerHandle);
	}

	Super::OnDestroy(AbilityEnded);
}

void UAbilityTask_WaitForInteractableTargets_SingleLineTrace::PerformTrace()
{
	YC_INTERACTION_SCOPE_CYCLE_COUNTER(PerformTrace); // 注意这里的统计包含了LineTrace的耗时
	
	AActor* AvatarActor = Ability->GetCurrentActorInfo()->AvatarActor.Get();
	if (!AvatarActor) return;

	const UWorld* World = GetWorld();
	check(World);

	// 设置忽略自己
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(AvatarActor);

	// 设置碰撞查询参数
	FCollisionQueryParams Params(SCENE_QUERY_STAT(UAbilityTask_WaitForInteractableTargets_SingleLineTrace), false);
	Params.AddIgnoredActors(ActorsToIgnore);

	const FVector TraceStart = StartLocation.GetTargetingTransform().GetLocation();
	FVector TraceEnd;
	// 计算基于Controller的射线检测终点
	AimWithPlayerController(AvatarActor, Params, TraceStart, InteractionScanLength, OUT TraceEnd);

	FHitResult OutHitResult;
	// 进行射线检测
	LineTrace(OutHitResult, World, TraceStart, TraceEnd, TraceProfile.Name, Params);

	// 从射线检测命中的Actor对象上获取InteractableTarget接口(可能存在多个, Actor本身以及Actor组件都可以实现可交互接口)
	TArray<TScriptInterface<IYcInteractableTarget>> InteractableTargets;
	UYcInteractionStatics::GetInteractableTargetsFromHitResult(OutHitResult, InteractableTargets);

	// 用获取到的InteractableTargets接口对象数组去更新Options
	UpdateInteractableOptions(InteractionQuery, InteractableTargets);

#if ENABLE_DRAW_DEBUG
	if (YcConsoleVariables::CVarDrawDebugShape.GetValueOnGameThread())
	{
		FColor DebugColor = OutHitResult.bBlockingHit ? FColor::Red : FColor::Green;
		if (OutHitResult.bBlockingHit)
		{
			DrawDebugLine(World, TraceStart, OutHitResult.Location, DebugColor, false, InteractionScanRate);
			DrawDebugSphere(World, OutHitResult.Location, 5, 16, DebugColor, false, InteractionScanRate);
		}
		else
		{
			DrawDebugLine(World, TraceStart, TraceEnd, DebugColor, false, InteractionScanRate);
		}
	}
#endif // ENABLE_DRAW_DEBUG
}