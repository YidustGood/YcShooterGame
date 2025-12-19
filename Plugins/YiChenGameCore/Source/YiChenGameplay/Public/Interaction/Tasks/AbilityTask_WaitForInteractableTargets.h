// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "AbilityTask_WaitForInteractableTargets.generated.h"

class IYcInteractableTarget;
struct FYcInteractionQuery;
struct FYcInteractionOption;

// 当前交互对象发生改变时的委托，广播新的交互对象上的InteractableOptions
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractableTargetsChangedEvent, const TArray<FYcInteractionOption>&, InteractableOptions);

/**
 * 扫描玩家视野焦点的可交互对象AbilityTask基类
 * 基于UWorld::LineTraceMultiByProfile提供可穿透的射线检测, 通常只需要使用首个检测到的目标
 */
UCLASS(Abstract)
class YICHENGAMEPLAY_API UAbilityTask_WaitForInteractableTargets : public UAbilityTask
{
	GENERATED_BODY()
public:
	UAbilityTask_WaitForInteractableTargets(const FObjectInitializer& ObjectInitializer);
	
	/** 交互对象发生改变的委托 */
	UPROPERTY(BlueprintAssignable)
	FInteractableTargetsChangedEvent OnInteractableTargetsChanged;
	
protected:
	/**
	 * 可穿透射线检测，填充第一个命中的 `FHitResult`
	 *
	 * @param OutHitResult	检测结果输出
	 * @param World	世界对象
	 * @param Start	射线起点
	 * @param End	射线终点
	 * @param ProfileName	碰撞配置名称
	 * @param Params	查询参数
	 */
	static void LineTrace(FHitResult& OutHitResult, const UWorld* World, const FVector& Start, const FVector& End, FName ProfileName, const FCollisionQueryParams Params);

	/**
	 * 根据玩家控制器视线，计算射线终点
	 *
	 * @param InSourceActor	源 Actor
	 * @param Params	碰撞查询参数
	 * @param TraceStart	射线起点
	 * @param MaxRange	最大射线长度
	 * @param OutTraceEnd	计算得到的射线终点
	 * @param bIgnorePitch	是否忽略 Pitch 分量
	 */
	void AimWithPlayerController(const AActor* InSourceActor, FCollisionQueryParams Params, const FVector& TraceStart, float MaxRange, FVector& OutTraceEnd, bool bIgnorePitch = false) const;

	/**
	 * 将摄像机射线裁剪到能力范围以内。
	 *
	 * @param CameraLocation	摄像机位置
	 * @param CameraDirection	摄像机前向向量
	 * @param AbilityCenter	能力中心
	 * @param AbilityRange	能力半径
	 * @param ClippedPosition	裁剪后的射线终点
	 * @return	若成功裁剪返回 true
	 */
	static bool ClipCameraRayToAbilityRange(FVector CameraLocation, FVector CameraDirection, FVector AbilityCenter, float AbilityRange, FVector& ClippedPosition);
	
	/**
	 * 更新交互对象Option数组
	 * @param InteractQuery 交互查询者的信息(玩家就是这个查询者，里面存放的玩家Actor和Controller的信息)
	 * @param InteractableTargets 射线检测到的Actor中的InteractableTargets(Actor本身可以携带，其组件也可以携带，所以是一个数组)
	 */
	void UpdateInteractableOptions(const FYcInteractionQuery& InteractQuery, const TArray<TScriptInterface<IYcInteractableTarget>>& InteractableTargets);

	FCollisionProfileName TraceProfile;
	
	bool bTraceAffectsAimPitch = true;

	/** 当前交互选项 */
	TArray<FYcInteractionOption> CurrentOptions;
	
	/** 缓存的上一次检测到的交互接口 */
	TSet<TScriptInterface<IYcInteractableTarget>> CachedInteractableTargets;
};