// Copyright (c) 2025 YiChen. All Rights Reserved.

#pragma once

#include "Components/GameStateComponent.h"
#include "YcPlayerSpawningManagerComponent.generated.h"


class AYcPlayerStart;
/**
 * 玩家生成管理组件
 * 负责管理玩家出生点的选择、缓存和重生逻辑
 */
UCLASS(ClassGroup=(YiChenGameplay), meta=(BlueprintSpawnableComponent))
class YICHENGAMEPLAY_API UYcPlayerSpawningManagerComponent : public UGameStateComponent
{
	GENERATED_BODY()

public:
	UYcPlayerSpawningManagerComponent(const FObjectInitializer& ObjectInitializer);

	/** UActorComponent */
	virtual void InitializeComponent() override;
	virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	/** ~UActorComponent */
	
protected:
	/** 从给定的出生点列表中获取第一个随机的未占用出生点 */
	APlayerStart* GetFirstRandomUnoccupiedPlayerStart(AController* Controller, const TArray<AYcPlayerStart*>& FoundStartPoints) const;
	
	/** 选择玩家出生点的蓝图可重写事件，返回nullptr则使用默认逻辑 */
	UFUNCTION(BlueprintNativeEvent)
	AActor* OnChoosePlayerStart(AController* Player, TArray<AYcPlayerStart*>& PlayerStarts);
	
	/** 完成玩家重生后的回调，可在子类中重写以添加自定义逻辑 */
	virtual void OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation) { }

	/** 完成玩家重生后的蓝图事件 */
	UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName=OnFinishRestartPlayer))
	void K2_OnFinishRestartPlayer(AController* Player, const FRotator& StartRotation);
	
private:
	/** 从AYcGameMode代理的调用，使每个Experience可以更轻松地自定义重生系统 */
	AActor* ChoosePlayerStart(AController* Player);
	bool ControllerCanRestart(AController* Player);
	void FinishRestartPlayer(AController* NewPlayer, const FRotator& StartRotation);
	friend class AYcGameMode;
	/** ~AYcGameMode */

	/** 缓存的玩家出生点列表 */
	UPROPERTY(Transient)
	TArray<TWeakObjectPtr<AYcPlayerStart>> CachedPlayerStarts;
	
	/** 当新关卡加入世界时的回调，用于缓存新关卡中的出生点 */
	void OnLevelAdded(ULevel* InLevel, UWorld* InWorld);
	
	/** 当Actor生成时的回调，用于缓存动态创建的出生点 */
	void HandleOnActorSpawned(AActor* SpawnedActor);

#if WITH_EDITOR
	/** 在编辑器PIE模式下查找"从此处开始游戏"的出生点 */
	APlayerStart* FindPlayFromHereStart(AController* Player);
#endif
};
