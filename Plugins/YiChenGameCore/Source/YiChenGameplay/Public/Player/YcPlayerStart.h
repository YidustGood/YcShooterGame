// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "GameFramework/PlayerStart.h"
#include "YcPlayerStart.generated.h"

/** 玩家出生点位置占用状态 */
UENUM()
enum class EYcPlayerStartLocationOccupancy : uint8
{
	Empty,		// 空闲，无碰撞
	Partial,	// 部分占用，可以找到传送点
	Full		// 完全占用，无法使用
};

/**
 * YiChenGameCore插件提供的玩家出生点类
 * 支持标签识别、占用检测和团队分配
 */
UCLASS(Config = Game)
class YICHENGAMEPLAY_API AYcPlayerStart : public APlayerStart
{
	GENERATED_BODY()
public:
	AYcPlayerStart(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 获取此出生点的游戏标签 */
	UFUNCTION(BlueprintCallable)
	const FGameplayTagContainer& GetGameplayTags() { return StartPointTags; }

	/** 获取此出生点的位置占用状态 */
	UFUNCTION(BlueprintCallable)
	EYcPlayerStartLocationOccupancy GetLocationOccupancy(AController* const ControllerPawnToFit) const;

	/** 获取此出生点所属的团队ID */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	int32 GetTeamId() const { return TeamId; };

	/** 检查此出生点是否已被控制器占用 */
	UFUNCTION(BlueprintCallable)
	bool IsClaimed() const;

	/** 尝试为指定控制器占用此出生点，如果未被占用则占用成功 */
	UFUNCTION(BlueprintCallable)
	bool TryClaim(AController* OccupyingController);

protected:
	/** 检查此出生点是否仍被占用，如果不再占用则释放 */
	UFUNCTION(BlueprintCallable)
	void CheckUnclaimed();

	/** 占用此出生点的控制器 */
	UPROPERTY(Transient)
	TObjectPtr<AController> ClaimingController = nullptr;

	/** 检查出生点是否不再碰撞的时间间隔 */
	UPROPERTY(EditDefaultsOnly, Category = "Player Start Claiming")
	float ExpirationCheckInterval = 1.f;

	/** 用于识别此出生点的标签 */
	UPROPERTY(EditAnywhere)
	FGameplayTagContainer StartPointTags;

	/** 用于跟踪过期检查的定时器句柄 */
	FTimerHandle ExpirationTimerHandle;
	
	/** 团队ID */
	UPROPERTY(EditAnywhere)
	int32 TeamId;
};
