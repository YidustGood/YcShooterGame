// Copyright (c) 2025 YiChen. All Rights Reserved.


#include "Interaction/Tasks/AbilityTask_WaitForInteractableTargets.h"

#include "AbilitySystemComponent.h"
#include "Interaction/YcInteractableTarget.h"
#include "Interaction/YcInteractionTypes.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_WaitForInteractableTargets)

// ==================== 性能计数器声明 ====================
DECLARE_CYCLE_STAT(TEXT("LineTrace"), STAT_YcInteraction_LineTrace, STATGROUP_YcInteraction);
DECLARE_CYCLE_STAT(TEXT("UpdateInteractableOptions"), STAT_YcInteraction_UpdateInteractableOptions, STATGROUP_YcInteraction);

UAbilityTask_WaitForInteractableTargets::UAbilityTask_WaitForInteractableTargets(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UAbilityTask_WaitForInteractableTargets::LineTrace(FHitResult& OutHitResult, const UWorld* World, const FVector& Start, const FVector& End, FName ProfileName,
														const FCollisionQueryParams Params)
{
	YC_INTERACTION_SCOPE_CYCLE_COUNTER(LineTrace);
	
	check(World);

	OutHitResult = FHitResult();
	TArray<FHitResult> HitResults;
	// @TODO 考虑到底有没有必要做Multi类型的射线检测, 目前业务需求单射线检测就足以
	World->LineTraceMultiByProfile(HitResults, Start, End, ProfileName, Params);

	OutHitResult.TraceStart = Start;
	OutHitResult.TraceEnd = End;

	if (HitResults.Num() > 0)
	{
		OutHitResult = HitResults[0];
	}
}

void UAbilityTask_WaitForInteractableTargets::AimWithPlayerController(const AActor* InSourceActor, FCollisionQueryParams Params, const FVector& TraceStart, float MaxRange, FVector& OutTraceEnd,
                                                                      bool bIgnorePitch) const
{
	if (!Ability) // Server and launching client only
	{
		return;
	}

	//@TODO: 是否需要支持AIController呢？目前仅支持PlayerController
	APlayerController* PC = Ability->GetCurrentActorInfo()->PlayerController.Get();
	if (!IsValid(PC)) return;

	// 获得玩家视角(相机的位置和旋转)
	FVector ViewStart;
	FRotator ViewRot;
	PC->GetPlayerViewPoint(ViewStart, ViewRot);

	const FVector ViewDir = ViewRot.Vector(); // 获得玩家视角的方向向量

	FVector ViewEnd = ViewStart + (ViewDir * MaxRange); // 射线的结束点 = 视角起点 + (视角方向 * 射线最大长度)

	// 计算相机裁剪与技能范围位置
	ClipCameraRayToAbilityRange(ViewStart, ViewDir, TraceStart, MaxRange, ViewEnd);

	FHitResult HitResult;
	LineTrace(HitResult, InSourceActor->GetWorld(), ViewStart, ViewEnd, TraceProfile.Name, Params);

	const bool bUseTraceResult = HitResult.bBlockingHit && (FVector::DistSquared(TraceStart, HitResult.Location) <= (MaxRange * MaxRange));

	const FVector AdjustedEnd = (bUseTraceResult) ? HitResult.Location : ViewEnd;

	FVector AdjustedAimDir = (AdjustedEnd - TraceStart).GetSafeNormal();
	if (AdjustedAimDir.IsZero())
	{
		AdjustedAimDir = ViewDir;
	}

	if (!bTraceAffectsAimPitch && bUseTraceResult)
	{
		FVector OriginalAimDir = (ViewEnd - TraceStart).GetSafeNormal();

		if (!OriginalAimDir.IsZero())
		{
			// 转换为角度并使用原始俯仰角
			const FRotator OriginalAimRot = OriginalAimDir.Rotation();

			FRotator AdjustedAimRot = AdjustedAimDir.Rotation();
			AdjustedAimRot.Pitch = OriginalAimRot.Pitch;

			AdjustedAimDir = AdjustedAimRot.Vector();
		}
	}

	OutTraceEnd = TraceStart + (AdjustedAimDir * MaxRange);
}

bool UAbilityTask_WaitForInteractableTargets::ClipCameraRayToAbilityRange(FVector CameraLocation, FVector CameraDirection, FVector AbilityCenter, float AbilityRange, FVector& ClippedPosition)
{
	FVector CameraToCenter = AbilityCenter - CameraLocation;
	float DotToCenter = FVector::DotProduct(CameraToCenter, CameraDirection);
	if (DotToCenter >= 0)
	{
		float DistanceSquared = CameraToCenter.SizeSquared() - (DotToCenter * DotToCenter);
		float RadiusSquared = (AbilityRange * AbilityRange);
		if (DistanceSquared <= RadiusSquared)
		{
			float DistanceFromCamera = FMath::Sqrt(RadiusSquared - DistanceSquared);
			float DistanceAlongRay = DotToCenter + DistanceFromCamera;
			ClippedPosition = CameraLocation + (DistanceAlongRay * CameraDirection);
			return true;
		}
	}
	return false;
}


void UAbilityTask_WaitForInteractableTargets::UpdateInteractableOptions(const FYcInteractionQuery& InteractQuery, const TArray<TScriptInterface<IYcInteractableTarget>>& InteractableTargets)
{
	YC_INTERACTION_SCOPE_CYCLE_COUNTER(UpdateInteractableOptions);

	// 这里不再仅仅在“焦点目标切换”时更新，而是每次扫描都重建一份最新的Options，
	// 以支持交互物在保持聚焦期间运行时动态切换“可交互 / 不可交互”状态。
	TSet<TScriptInterface<IYcInteractableTarget>> NewInteractableTargetSet;
	TArray<FYcInteractionOption> NewOptions;

	// 遍历本次扫描到的交互目标
	for (const TScriptInterface<IYcInteractableTarget>& InteractiveTarget : InteractableTargets)
	{
		NewInteractableTargetSet.Add(InteractiveTarget);

		// 仅当目标是本次新进入焦点时，才触发一次聚焦开始回调
		if (!CachedInteractableTargets.Contains(InteractiveTarget))
		{
			InteractiveTarget->OnPlayerFocusBegin(InteractQuery);
		}
		
		// 存储从交互对象获取到的Options
		TArray<FYcInteractionOption> TempOptions;
		// 使用交互目标接口实例化InteractionBuilder, 这会设置InteractionBuilder的Options引用到TempOptions
		FYcInteractionOptionBuilder InteractionBuilder(InteractiveTarget, TempOptions);
		// 调用交互目标接口获取其对象拥有的Option
		InteractiveTarget->GatherInteractionOptions(InteractQuery, InteractionBuilder);

		// 处理交互物体的Options中的能力GA
		for (FYcInteractionOption& Option : TempOptions)
		{
			// 如果当前配置为“不可交互且隐藏提示”，Gather阶段通常已经过滤掉；
			// 这里再次做保护，防止外部自定义实现遗漏该判断。
			if (!Option.ShouldShowPrompt())
			{
				continue;
			}

			FGameplayAbilitySpec* InteractionAbilitySpec = nullptr;

			// 如果目标拥有 AbilitySystem 且GA句柄有效，则在目标上触发该能力
			if (Option.TargetAbilitySystem && Option.TargetInteractionAbilityHandle.IsValid())
			{
				// Find the spec
				InteractionAbilitySpec = Option.TargetAbilitySystem->FindAbilitySpecFromHandle(Option.TargetInteractionAbilityHandle);
			}
			// 如果配置了交互能力，则在玩家自身 ASC 上激活
			else if (Option.InteractionAbilityToGrant)
			{
				// Find the spec，这里的ASC是调用这个Task的GA的拥有者的ASC组件（在这里其实就是玩家的ASC组件）
				InteractionAbilitySpec = AbilitySystemComponent->FindAbilitySpecFromClass(Option.InteractionAbilityToGrant);

				if (InteractionAbilitySpec)
				{
					Option.TargetAbilitySystem = AbilitySystemComponent.Get();
					Option.TargetInteractionAbilityHandle = InteractionAbilitySpec->Handle;
				}
			}

			// 对于“不可交互但显示禁用提示”的Option，不再要求能力当前可激活，
			// 这样UI依然可以显示出来，但后续真正执行交互时会被阻止。
			if (!Option.IsInteractable())
			{
				NewOptions.Add(Option);
				continue;
			}

			if (InteractionAbilitySpec)
			{
				// 过滤掉当前由于条件不足无法激活的选项
				if (InteractionAbilitySpec->Ability->CanActivateAbility(InteractionAbilitySpec->Handle, AbilitySystemComponent->AbilityActorInfo.Get()))
				{
					NewOptions.Add(Option);
				}
			}
		}
	}
	
	// 对上一次缓存中的交互目标进行检查，找出本次已经失焦的目标并触发失焦回调
	for (const TScriptInterface<IYcInteractableTarget>& Target : CachedInteractableTargets)
	{
		if (!NewInteractableTargetSet.Contains(Target))
		{
			Target->OnPlayerFocusEnd(InteractQuery);
		}
	}

	// 判断交互目标集合是否发生变化
	bool bTargetsChanged = CachedInteractableTargets.Num() != NewInteractableTargetSet.Num();
	if (!bTargetsChanged)
	{
		for (const TScriptInterface<IYcInteractableTarget>& Target : NewInteractableTargetSet)
		{
			if (!CachedInteractableTargets.Contains(Target))
			{
				bTargetsChanged = true;
				break;
			}
		}
	}

	// 即使焦点目标没有变化，只要Option内容变化（例如禁用态文案、状态Tag、显示策略变化），
	// 也需要广播出去以刷新UI。
	const bool bOptionsChanged = CurrentOptions != NewOptions;
	
	// 重新缓存本次交互目标
	CachedInteractableTargets = MoveTemp(NewInteractableTargetSet);
	
	// 如发生变化则更新CurrentOptions并进行广播
	if (bTargetsChanged || bOptionsChanged)
	{
		CurrentOptions = NewOptions;
		OnInteractableTargetsChanged.Broadcast(CurrentOptions);
	}
}
